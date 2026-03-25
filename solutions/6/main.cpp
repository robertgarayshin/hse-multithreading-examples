#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "thread_pool.hpp"

static void section(const char *title) {
	std::cout << "\n=== " << title << " ===\n";
}

int main() {
	section("ThreadPool — basic results"); {
		ThreadPool pool(4);

		auto f_add = pool.Submit([](const int a, const int b) { return a + b; }, 3, 4);
		auto f_slow = pool.Submit([] {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			return 42;
		});
		auto f_str = pool.Submit([](std::string s) { return s + " world"; },
		                         std::string("hello"));

		std::cout << "3 + 4          = " << f_add.Get() << "\n";
		std::cout << "async result   = " << f_slow.Get() << "\n";
		std::cout << "string concat  = " << f_str.Get() << "\n";
	}

	section("ThreadPool — void tasks"); {
		ThreadPool pool(2);

		int counter = 0;
		std::mutex mtx;

		std::vector<Future<void> > futures;
		futures.reserve(8);
		for (int i = 0; i < 8; ++i) {
			futures.push_back(pool.Submit([&counter, &mtx] {
				std::lock_guard lk(mtx);
				++counter;
			}));
		}
		for (auto &f: futures) f.Get();

		std::cout << "counter after 8 void tasks = " << counter << "\n";
	}

	section("ThreadPool — exception propagation"); {
		ThreadPool pool(2);

		auto f = pool.Submit([]() -> int {
			throw std::runtime_error("task error");
		});

		try {
			f.Get();
		} catch (const std::exception &e) {
			std::cout << "caught: " << e.what() << "\n";
		}
	}

	return 0;
}
