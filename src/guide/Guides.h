#pragma once

#include <string>

class Player;

namespace welcome_noob {

struct StepConfig;

// cmd_detect 类型步骤引导
class CmdDetectGuide {
public:
    static void start(Player& player, const StepConfig& step);
};

// manual_submit 类型步骤引导
class ManualSubmitGuide {
public:
    static void start(Player& player, const StepConfig& step);

    // 检查玩家背包铁锭数量 >= 8
    [[nodiscard]] static bool checkIronIngot(Player& player, int required = 8);
};

// external_report 类型步骤引导
class ExternalReportGuide {
public:
    static void start(Player& player, const StepConfig& step);
};

// camera_tour 类型步骤引导
class CameraTourGuide {
public:
    static void start(Player& player, const StepConfig& step);
};

} // namespace welcome_noob
