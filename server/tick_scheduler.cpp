#include "tick_scheduler.h"

#include <stdexcept>

TickScheduler::TickScheduler(std::chrono::milliseconds tick_interval):
        tick_interval(tick_interval) {
    if (tick_interval.count() <= 0) {
        throw std::invalid_argument("El intervalo de actualizacion debe ser positivo");
    }
}

std::size_t TickScheduler::advance(std::chrono::steady_clock::duration elapsed) {
    if (elapsed < std::chrono::steady_clock::duration::zero()) {
        throw std::invalid_argument("El tiempo transcurrido no puede ser negativo");
    }

    if (elapsed > std::chrono::steady_clock::duration::max() - accumulated) {
        throw std::overflow_error("El tiempo acumulado supera el rango representable");
    }

    accumulated += elapsed;

    const auto pending_steps = accumulated / tick_interval;
    accumulated = accumulated % tick_interval;

    if (pending_steps >= MAX_STEPS_PER_CYCLE) {
        return MAX_STEPS_PER_CYCLE;
    }

    return static_cast<std::size_t>(pending_steps);
}

std::chrono::steady_clock::duration TickScheduler::get_wait_time() const {
    return tick_interval - accumulated;
}
