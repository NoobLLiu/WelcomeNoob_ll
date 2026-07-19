#include "data/PlayerData.h"

namespace welcome_noob {

PlayerData PlayerData::getDefault() {
    PlayerData d;
    d.status = "not_started";
    d.currentStep = std::nullopt;
    d.completedSteps = {};
    d.joinCount = 0;
    return d;
}

bool PlayerData::isStepCompleted(const std::string& stepKey) const {
    for (const auto& s : completedSteps) {
        if (s == stepKey) return true;
    }
    return false;
}

void from_json(const nlohmann::json& j, PlayerData& p) {
    p.status = j.value("status", std::string{"not_started"});
    if (j.contains("currentStep") && !j["currentStep"].is_null()) {
        p.currentStep = j["currentStep"].get<std::string>();
    } else {
        p.currentStep = std::nullopt;
    }
    p.completedSteps = j.value("completedSteps", std::vector<std::string>{});
    p.joinCount = j.value("joinCount", 0);
}

void to_json(nlohmann::json& j, const PlayerData& p) {
    // 显式构造 currentStep 的 json 值，避免三元表达式类型歧义导致的潜在 UB
    nlohmann::json stepJson = p.currentStep.has_value()
        ? nlohmann::json(*p.currentStep)
        : nlohmann::json(nullptr);
    j = nlohmann::json{
        {"status", p.status},
        {"currentStep", std::move(stepJson)},
        {"completedSteps", p.completedSteps},
        {"joinCount", p.joinCount}
    };
}

} // namespace welcome_noob
