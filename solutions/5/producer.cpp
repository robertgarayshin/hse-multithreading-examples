#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "mpsc_queue.h"

static constexpr char     QUEUE_NAME[]    = "/mpsc_queue";
static constexpr uint32_t QUEUE_CAPACITY  = 8192;

enum class MsgType : uint32_t {
    Heartbeat = 1,
    Data      = 2,
    Command   = 3,
};

struct HeartbeatPayload {
    uint64_t timestamp_ms;
    uint32_t producer_id;
};

struct DataPayloadHeader {
    uint32_t producer_id;
    uint32_t sequence;
};

void producer_thread(const ProducerNode& queue, const uint32_t id, const uint32_t count) {
    using namespace std::chrono;

    for (uint32_t i = 0; i < count; ++i) {
        switch (i % 3) {
        case 0: {
            HeartbeatPayload p{};
            p.timestamp_ms =
                duration_cast<milliseconds>(
                    system_clock::now().time_since_epoch())
                    .count();
            p.producer_id = id;
            queue.send(static_cast<uint32_t>(MsgType::Heartbeat), &p, sizeof(p));
            break;
        }
        case 1: {
            DataPayloadHeader hdr{};
            hdr.producer_id = id;
            hdr.sequence    = i;
            std::string text = "hello from producer "
                             + std::to_string(id) + ", seq " + std::to_string(i);
            std::vector<uint8_t> buf(sizeof(hdr) + text.size());
            std::memcpy(buf.data(), &hdr, sizeof(hdr));
            std::memcpy(buf.data() + sizeof(hdr), text.data(), text.size());
            queue.send(static_cast<uint32_t>(MsgType::Data), buf.data(), buf.size());
            break;
        }
        default: {
            constexpr char cmd[] = "PING";
            queue.send(static_cast<uint32_t>(MsgType::Command), cmd, strlen(cmd));
            break;
        }
        }

        std::this_thread::sleep_for(milliseconds(10));
    }

    std::cout << "[producer " << id << "] finished (" << count << " messages)\n";
}

int main() {
    try {
        ProducerNode queue(QUEUE_NAME, QUEUE_CAPACITY);
        std::cout << "[producer] queue created: " << QUEUE_NAME
                  << "  capacity=" << QUEUE_CAPACITY << "\n";

        constexpr uint32_t NUM_THREADS      = 3;
        constexpr uint32_t MSGS_PER_THREAD  = 12;

        std::cout << "[producer] spawning " << NUM_THREADS
                  << " producer threads, " << MSGS_PER_THREAD
                  << " messages each\n";

        std::vector<std::thread> threads;
        threads.reserve(NUM_THREADS);
        for (uint32_t i = 0; i < NUM_THREADS; ++i)
            threads.emplace_back(producer_thread, std::ref(queue), i, MSGS_PER_THREAD);

        for (auto& t : threads)
            t.join();

        std::cout << "[producer] all done, total="
                  << NUM_THREADS * MSGS_PER_THREAD << " messages sent\n";

        std::this_thread::sleep_for(std::chrono::seconds(3));

    } catch (const std::exception& e) {
        std::cerr << "producer error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
