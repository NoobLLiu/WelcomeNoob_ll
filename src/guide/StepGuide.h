#pragma once

#include <string>

class Player;

namespace welcome_noob {

struct StepConfig;

// 步骤完成核心逻辑（对应原 JS 的 onStepComplete / advanceToNextStep / completeStep）
class StepGuide {
public:
    // 玩家完成某步骤（对应 onStepComplete）
    static void onStepComplete(Player& player, const std::string& stepKey);

    // 推进到下一步（对应 advanceToNextStep）
    static void advanceToNextStep(const std::string& xuid);

    // 开始某步骤的引导（对应 startStepGuide）
    static void startStepGuide(Player& player, const StepConfig& step);

    // 更新 noob tag（对应 updateNoobTag）
    static void updateNoobTag(Player& player);
};

} // namespace welcome_noob
