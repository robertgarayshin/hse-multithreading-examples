#include <chrono>
#include <iostream>
#include <string>

#include "mpsc_queue.h"

static constexpr char QUEUE_NAME[] = "/mpsc_queue";

enum class MsgType : uint32_t {
	Heartbeat = 1,
	Data = 2,
	Command = 3,
};

struct HeartbeatPayload {
	uint64_t timestamp_ms;
	uint32_t producer_id;
};

struct DataPayloadHeader {
	uint32_t producer_id;
	uint32_t sequence;
};

static void print_message(const Message &msg) {
	std::cout << "[consumer] type=" << msg.type
			<< " len=" << msg.data.size();

	switch (static_cast<MsgType>(msg.type)) {
		case MsgType::Heartbeat:
			if (msg.data.size() >= sizeof(HeartbeatPayload)) {
				HeartbeatPayload p{};
				std::memcpy(&p, msg.data.data(), sizeof(p));
				std::cout << " | heartbeat from producer=" << p.producer_id
						<< " timestamp=" << p.timestamp_ms << " ms";
			}
			break;
		case MsgType::Data:
			if (msg.data.size() >= sizeof(DataPayloadHeader)) {
				DataPayloadHeader p{};
				std::memcpy(&p, msg.data.data(), sizeof(p));
				const std::string text(reinterpret_cast<const char*>(msg.data.data() + sizeof(p)),
				                 msg.data.size() - sizeof(p));
				std::cout << " | data from producer=" << p.producer_id
						<< " seq=" << p.sequence
						<< " \"" << text << "\"";
			}
			break;
		case MsgType::Command: {
			std::cout << " | cmd=\""
					<< std::string(reinterpret_cast<const char *>(msg.data.data()),
					               msg.data.size())
					<< "\"";
			break;
		}
		default:
			std::cout << " | (unknown type)";
	}

	std::cout << "\n";
}

int main(const int argc, char *argv[]) {
	uint32_t type_filter = 0;
	if (argc > 1) {
		type_filter = static_cast<uint32_t>(std::stoul(argv[1]));
		std::cout << "[consumer] filter: type=" << type_filter << " only\n";
	} else {
		std::cout << "[consumer] filter: all types\n";
	}

	try {
		std::cout << "[consumer] waiting for queue " << QUEUE_NAME << " ...\n";
		const ConsumerNode queue(QUEUE_NAME);
		std::cout << "[consumer] connected\n\n";

		uint32_t received = 0;

		while (true) {
			constexpr auto deadline = std::chrono::seconds{2};
			auto msg =
					queue.receive(type_filter, deadline);

			if (!msg) {
				std::cout << "\n[consumer] "<< deadline << " idle — producer finished\n";
				break;
			}

			++received;
			print_message(*msg);
		}

		std::cout << "[consumer] total received: " << received << "\n";
	} catch (const std::exception &e) {
		std::cerr << "consumer error: " << e.what() << "\n";
		return 1;
	}
	return 0;
}
