#pragma once

#include <cstddef>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

inline constexpr uint32_t PROTOCOL_VERSION = 1;
inline constexpr uint32_t DEFAULT_CAPACITY = 4096;

static_assert(ATOMIC_INT_LOCK_FREE  == 2, "uint32_t atomic must be lock-free");
static_assert(ATOMIC_LLONG_LOCK_FREE == 2, "uint64_t atomic must be lock-free");

struct QueueHeader {
    std::atomic<uint32_t> initialized;
    uint32_t              protocol_version;
    uint32_t              capacity;
    std::atomic<uint64_t> head;
    std::atomic<uint64_t> tail;
    std::atomic<uint64_t> used;
};

struct MessageHeader {
    uint32_t type;
    uint32_t length;
};

inline void write_ring(std::byte* buf, const uint32_t cap, const uint64_t offset, const void* data, const uint32_t size) {
    const auto pos = static_cast<uint32_t>(offset % cap);
    const uint32_t first = std::min(size, cap - pos);
    std::memcpy(buf + pos, data, first);
    if (first < size)
        std::memcpy(buf, static_cast<const std::byte*>(data) + first, size - first);
}

inline void read_ring(const std::byte* buf, const uint32_t cap, const uint64_t offset, void* out, const uint32_t size) {
    const auto pos = static_cast<uint32_t>(offset % cap);
    const uint32_t first = std::min(size, cap - pos);
    std::memcpy(out, buf + pos, first);
    if (first < size)
        std::memcpy(static_cast<std::byte*>(out) + first, buf, size - first);
}

inline size_t shm_total_size(const uint32_t capacity) {
    return sizeof(QueueHeader) + capacity;
}

struct Message {
    uint32_t             type{};
    std::vector<uint8_t> data;
};

class ProducerNode {
public:
    explicit ProducerNode(const std::string& name, const uint32_t capacity = DEFAULT_CAPACITY)
        : name_(name), capacity_(capacity)
    {
        shm_unlink(name.c_str());

        const size_t size = shm_total_size(capacity);

        const int fd = shm_open(name.c_str(), O_CREAT | O_RDWR, 0666);
        if (fd == -1)
            throw std::runtime_error("shm_open: " + std::string(strerror(errno)));

        if (ftruncate(fd, static_cast<off_t>(size)) == -1) {
            close(fd);
            shm_unlink(name.c_str());
            throw std::runtime_error("ftruncate: " + std::string(strerror(errno)));
        }

        ptr_ = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);
        if (ptr_ == MAP_FAILED) {
            shm_unlink(name.c_str());
            throw std::runtime_error("mmap: " + std::string(strerror(errno)));
        }

        std::memset(ptr_, 0, size);

        hdr_ = static_cast<QueueHeader*>(ptr_);
        hdr_->protocol_version = PROTOCOL_VERSION;
        hdr_->capacity         = capacity;
        hdr_->head.store(0, std::memory_order_relaxed);
        hdr_->tail.store(0, std::memory_order_relaxed);
        hdr_->used.store(0, std::memory_order_relaxed);

        buf_ = reinterpret_cast<std::byte*>(hdr_ + 1);

        hdr_->initialized.store(1, std::memory_order_release);
    }

    ~ProducerNode() {
        if (ptr_ && ptr_ != MAP_FAILED)
            munmap(ptr_, shm_total_size(capacity_));
        shm_unlink(name_.c_str());
    }

    bool send(const uint32_t type, const void* data, const uint32_t length) const {
        const uint32_t required = sizeof(MessageHeader) + length;
        if (required > capacity_)
            return false;

        uint64_t head;
        while (true) {
            head = hdr_->head.load(std::memory_order_relaxed);

            if (const uint64_t tail = hdr_->tail.load(std::memory_order_acquire); head + required - tail > capacity_)
                return false;

            if (hdr_->head.compare_exchange_weak(head, head + required, std::memory_order_relaxed))
                break;
        }

        const MessageHeader mh{type, length};
        write_ring(buf_, capacity_, head, &mh, sizeof(mh));
        write_ring(buf_, capacity_, head + sizeof(mh), data, length);

        uint64_t expected = head;
        while (!hdr_->used.compare_exchange_weak(expected, head + required,
                                                  std::memory_order_release,
                                                  std::memory_order_relaxed))
            expected = head;

        return true;
    }

private:
    std::string   name_;
    uint32_t      capacity_;
    void*         ptr_ = nullptr;
    QueueHeader*  hdr_ = nullptr;
    std::byte*    buf_ = nullptr;
};

class ConsumerNode {
public:
    explicit ConsumerNode(const std::string& name) : name_(name) {
        int fd = -1;
        while (fd == -1) {
            fd = shm_open(name.c_str(), O_RDWR, 0666);
            if (fd == -1) {
                if (errno == ENOENT) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    continue;
                }
                throw std::runtime_error("shm_open: " + std::string(strerror(errno)));
            }
        }

        void* hdr_map = mmap(nullptr, sizeof(QueueHeader), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (hdr_map == MAP_FAILED) {
            close(fd);
            throw std::runtime_error("mmap header: " + std::string(strerror(errno)));
        }

        const auto* hdr = static_cast<QueueHeader*>(hdr_map);

        while (hdr->initialized.load(std::memory_order_acquire) == 0)
            std::this_thread::yield();

        if (hdr->protocol_version != PROTOCOL_VERSION) {
            munmap(hdr_map, sizeof(QueueHeader));
            close(fd);
            throw std::runtime_error(
                "Protocol version mismatch: expected " +
                std::to_string(PROTOCOL_VERSION) + ", got " +
                std::to_string(hdr->protocol_version));
        }

        capacity_ = hdr->capacity;
        munmap(hdr_map, sizeof(QueueHeader));

        const size_t size = shm_total_size(capacity_);
        ptr_ = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);
        if (ptr_ == MAP_FAILED)
            throw std::runtime_error("mmap full: " + std::string(strerror(errno)));

        hdr_ = static_cast<QueueHeader*>(ptr_);
        buf_ = reinterpret_cast<std::byte*>(hdr_ + 1);
    }

    ~ConsumerNode() {
        if (ptr_ && ptr_ != MAP_FAILED)
            munmap(ptr_, shm_total_size(capacity_));
    }

    [[nodiscard]] std::optional<Message> try_receive(const uint32_t type_filter = 0) const {
        while (true) {
            const uint64_t tail = hdr_->tail.load(std::memory_order_relaxed);
            const uint64_t used = hdr_->used.load(std::memory_order_acquire);

            if (tail == used)
                return std::nullopt;

            MessageHeader mh{};
            read_ring(buf_, capacity_, tail, &mh, sizeof(mh));

            const uint32_t total = sizeof(MessageHeader) + mh.length;
            if (tail + total > used)
                return std::nullopt;

            if (type_filter != 0 && mh.type != type_filter) {
                hdr_->tail.store(tail + total, std::memory_order_relaxed);
                continue;
            }

            Message msg;
            msg.type = mh.type;
            msg.data.resize(mh.length);
            read_ring(buf_, capacity_, tail + sizeof(MessageHeader), msg.data.data(), mh.length);

            hdr_->tail.store(tail + total, std::memory_order_relaxed);
            return msg;
        }
    }

    [[nodiscard]] std::optional<Message> receive(
        const uint32_t type_filter = 0,
        const std::chrono::milliseconds timeout = std::chrono::milliseconds{0}) const
    {
        const bool has_deadline = (timeout.count() > 0);
        const auto deadline     = std::chrono::steady_clock::now() + timeout;

        while (true) {
            if (auto msg = try_receive(type_filter))
                return msg;

            if (has_deadline && std::chrono::steady_clock::now() >= deadline)
                return std::nullopt;

            std::this_thread::yield();
        }
    }

private:
    std::string   name_;
    uint32_t      capacity_ = 0;
    void*         ptr_      = nullptr;
    QueueHeader*  hdr_      = nullptr;
    std::byte*    buf_      = nullptr;
};
