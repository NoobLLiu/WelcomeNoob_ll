#pragma once

#include "config/StepConfig.h"

#include <string>

class Player;

namespace welcome_noob {

// HUD 循环推送管理（对应原 JS 的 startActionbarLoop / stopActionbarLoop）
// 每 1 秒（20 ticks）刷新 actionbar + subtitle
class HudManager {
public:
    static HudManager& getInstance();

    // 启动某玩家的 HUD 循环
    void start(const std::string& xuid, const StepConfig& step);

    // 停止某玩家的 HUD 循环
    void stop(const std::string& xuid);

    // 停止所有玩家的 HUD 循环
    void stopAll();

private:
    HudManager() = default;

    // 内部循环
    void loop(const std::string& xuid, StepConfig step);
};

} // namespace welcome_noob
