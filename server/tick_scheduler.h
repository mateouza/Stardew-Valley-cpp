#ifndef TICK_SCHEDULER_H
#define TICK_SCHEDULER_H

#include <chrono>
#include <cstddef>

class TickScheduler {
private:
    static constexpr std::chrono::steady_clock::duration::rep MAX_STEPS_PER_CYCLE = 5;

    std::chrono::steady_clock::duration tick_interval;
    std::chrono::steady_clock::duration accumulated{};

public:
    explicit TickScheduler(std::chrono::milliseconds tick_interval);

    std::size_t advance(std::chrono::steady_clock::duration elapsed);

    std::chrono::steady_clock::duration get_wait_time() const;
};

#endif
