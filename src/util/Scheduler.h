#pragma once

#include <functional>

namespace welcome_noob {

// 定时器封装（对应 LSE 的 setTimeout/clearTimeout/setInterval/clearInterval）
// 基于 ll::thread::ServerThreadExecutor
class Scheduler {
public:
    // 延迟执行（ticks 为游戏刻，20 ticks = 1 秒）
    static void after(int ticks, std::function<void()> cb);

    // 立即在服务端线程执行
    static void now(std::function<void()> cb);
};

} // namespace welcome_noob
