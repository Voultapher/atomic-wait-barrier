#include <atomic>
#include <chrono>
#include <cstdio>
#include <future>
#include <thread>
#include <vector>

#include <atomic_wait.hpp>

class Barrier {
public:
  explicit Barrier(const int32_t count) noexcept {
    _count.store(count, std::memory_order_release);
  }

  auto wait() -> void {
    const int32_t new_count{_count.fetch_sub(1, std::memory_order_acq_rel) - 1};

    for (
      int32_t local_count{new_count};
      local_count > 0;
      local_count = _count.load(std::memory_order_relaxed)
    ) {
      // Efficiently wait for changes to this variable.
      atomic_wait(_count, local_count, std::memory_order_relaxed);
      //std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }

    // If the wait condition is not met eg every wait is triggered,
    // notify all other potentially still waiting barriers.
    if (new_count < 1) {
      atomic_notify_all(_count);
    }
  }

private:
  std::atomic<int32_t> _count = ATOMIC_VAR_INIT(0);
};


std::atomic<int32_t> GLOBAL_COUNT = ATOMIC_VAR_INIT(0);

auto observe(const int32_t barrier_count) -> void {
  int32_t last_local_count{GLOBAL_COUNT.load(std::memory_order_relaxed)};

  for (
    int32_t local_count{last_local_count};
    local_count < barrier_count;
    local_count = GLOBAL_COUNT.load(std::memory_order_relaxed)
  ) {
    if (local_count != last_local_count) {
      std::printf(
        "Saw global change from: %d -> %d\n",
        last_local_count,
        local_count
      );

      last_local_count = local_count;
    }
  }
};

auto test_barrier(const int32_t barrier_count) -> void {
  GLOBAL_COUNT.store(0, std::memory_order_release);

  Barrier barrier{barrier_count};

  auto wait_and_update = [&barrier](const std::chrono::milliseconds sleep_for) -> int32_t {
    std::this_thread::sleep_for(sleep_for);

    static_cast<void>(GLOBAL_COUNT.fetch_add(1, std::memory_order_release));

    const auto start = std::chrono::high_resolution_clock::now();
    barrier.wait();

    const auto end = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> diff = end - start;

    std::printf("Waited %f for barrier\n", diff.count());

    return GLOBAL_COUNT.load(std::memory_order_acquire);
  };

  auto observe_future = std::async(std::launch::async, observe, barrier_count);

  std::vector<std::future<int32_t>> update_futures{};
  update_futures.reserve(barrier_count);

  for (int32_t i = 0; i < barrier_count; ++i) {
    const std::chrono::milliseconds sl{(barrier_count - i) * 2};
    update_futures.push_back(std::async(std::launch::async, wait_and_update, sl));
  }

  for (auto& future : update_futures) {
    std::printf("Value of wait and update job: %d\n", future.get());
  }

  observe_future.get();
}

int32_t main()
{
  //test_barrier(1);
  //test_barrier(2);
  //test_barrier(4);
  test_barrier(64);

  return 0;
}
