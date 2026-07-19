#pragma once

namespace welcome_noob {

// 命令注册（对应原 JS 的 mc.newCommand + onServerStarted）
class Commands {
public:
    // 注册 /noob 和 /noobapi 命令
    static void registerAll();
};

} // namespace welcome_noob
