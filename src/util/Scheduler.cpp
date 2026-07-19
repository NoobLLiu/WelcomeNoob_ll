#include "util/Scheduler.h"

#include "ll/api/thread/ServerThreadExecutor.h"

namespace welcome_noob {

void Scheduler::after(int ticks, std::function<void()> cb) {
    if (ticks <= 0) {
        now(std::move(cb));
        return;
    }
    ll::thread::ServerThreadExecutor::getDefault().executeAfter(std::move(cb), ticks);
}

void Scheduler::now(std::function<void()> cb) {
    ll::thread::ServerThreadExecutor::getDefault().execute(std::move(cb));
}

} // namespace welcome_noob
