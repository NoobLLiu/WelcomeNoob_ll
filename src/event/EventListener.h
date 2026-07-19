#pragma once

namespace welcome_noob {

// 事件监听注册/注销（对应原 JS 的 mc.listen）
class EventListener {
public:
    // 在 load() 中调用，订阅所有事件
    static void registerAll();

    // 在 disable() 中调用，取消订阅
    static void unregisterAll();
};

} // namespace welcome_noob
