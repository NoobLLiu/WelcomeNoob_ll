#include "util/Scheduler.h"

#include "ll/api/thread/ServerThreadExecutor.h"

#include <chrono>

namespace welcome_noob {

void Scheduler::after(int ticks, std::function<void()> cb) {
    if (ticks <= 0) {
        now(std::move(cb));
        return;
    }
    // LL 26.10.14: executeAfter 第二参数为 Duration (std::chrono::nanoseconds)
    // 1 game tick = 50ms = 50'000'000 ns
    ll::thread::ServerThreadExecutor::getDefault().executeAfter(
        std::move(cb),
        std::chrono::nanoseconds(ticks * 50'000'000LL)
    );
}

void Scheduler::now(std::function<void()> cb) {
    ll::thread::ServerThreadExecutor::getDefault().execute(std::move(cb));
}

} // namespace welcome_noob
