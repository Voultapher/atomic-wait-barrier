#include <atomic_wait.hpp>

#include <mutex>
#include <thread>
#include <vector>
#include <iostream>
#include <chrono>

struct mutex {
	void lock() noexcept {
		while (1 == l.exchange(1, std::memory_order_acquire))
			atomic_wait(l, 1, std::memory_order_relaxed);
	}
	void unlock() noexcept {
		l.store(0, std::memory_order_release);
		atomic_notify_one(l);
	}
	std::atomic<int> l = ATOMIC_VAR_INIT(0);
};

struct ticket_mutex {
	void lock() noexcept {
        auto const my = in.fetch_add(1, std::memory_order_acquire);
        while(1) {
            auto const now = out.load(std::memory_order_acquire);
            if(now == my)
                return;
            atomic_wait(out, now, std::memory_order_relaxed);
        }
	}
	void unlock() noexcept {
		out.fetch_add(1, std::memory_order_release);
		atomic_notify_all(out);
	}
	alignas(64) std::atomic<int> in = ATOMIC_VAR_INIT(0);
    alignas(64) std::atomic<int> out = ATOMIC_VAR_INIT(0);
};

template <class F>
void test(char const* name, int threads, F && f) {

	auto const t1 = std::chrono::steady_clock::now();

	int sections = 1 << 20;

	std::vector<std::thread> ts(threads);
	for (auto& t : ts)
		t = std::thread([=]() {
            f(sections / threads);
        });

	for (auto& t : ts)
		t.join();

	auto const t2 = std::chrono::steady_clock::now();

	double const d = double(std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count());
	std::cout << name << " : " << d / sections << "ns per section.\n";
}

int main() {

	mutex m;
    auto f = [&](int n) {
		for (int i = 0; i < n; ++i) {
			m.lock();
			m.unlock();
		}
	};

	test("1", 1, f);
	test("2", 2, f);
	test("128", 128, f);

	ticket_mutex t;
    auto g = [&](int n) {
		for (int i = 0; i < n; ++i) {
			t.lock();
			t.unlock();
		}
	};

	test("1", 1, g);
	test("2", 2, g);
	test("max", std::thread::hardware_concurrency(), g);

	return 0;
}
