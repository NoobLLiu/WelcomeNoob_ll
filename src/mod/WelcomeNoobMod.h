#pragma once

#include "ll/api/mod/NativeMod.h"

namespace welcome_noob {

class WelcomeNoobMod {
public:
    static WelcomeNoobMod& getInstance();

    WelcomeNoobMod() : mSelf(*ll::mod::NativeMod::current()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    bool load();
    bool enable();
    bool disable();

    // unload() 用于支持 ll reload 热重载：停止运行时状态 + 取消事件订阅 + 保存数据
    // 注意：命令注册依赖 ServerCommandRegisterEvent，只在服务器启动时触发一次，
    // 热重载时命令不会重新注册（旧命令仍保留），这是 LL 的限制
    bool unload();

private:
    ll::mod::NativeMod& mSelf;
};

} // namespace welcome_noob
