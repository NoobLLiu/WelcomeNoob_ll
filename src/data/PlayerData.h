#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace welcome_noob {

// 玩家教程进度数据
struct PlayerData {
    std::string status = "not_started";        // not_started | in_progress | completed | skipped
    std::optional<std::string> currentStep;    // 当前进行中的步骤 key
    std::vector<std::string> completedSteps;   // 已完成步骤的 key 数组
    int joinCount = 0;

    [[nodiscard]] static PlayerData getDefault();
    [[nodiscard]] bool isStepCompleted(const std::string& stepKey) const;
};

void from_json(const nlohmann::json& j, PlayerData& p);
void to_json(nlohmann::json& j, const PlayerData& p);

} // namespace welcome_noob
