#include "config/StepConfig.h"

namespace welcome_noob {

void from_json(const nlohmann::json& j, SceneConfig& s) {
    s.delay = j.value("delay", 0);
    s.camera = j.value("camera", std::vector<std::string>{});
    s.message = j.value("message", std::string{});
    s.messageDelay = j.value("messageDelay", 0);
}

void from_json(const nlohmann::json& j, StepConfig& s) {
    s.key = j.value("key", std::string{});
    s.type = j.value("type", std::string{});
    s.title = j.value("title", std::string{});
    s.description = j.value("description", std::string{});
    s.hint = j.value("hint", std::string{});
    s.subtitle = j.value("subtitle", std::string{});
    s.actionbar = j.value("actionbar", std::string{});
    if (j.contains("config") && j["config"].is_object()) {
        s.config = j["config"];
    } else {
        s.config = nlohmann::json::object();
    }
}

void from_json(const nlohmann::json& j, WelcomeConfig& w) {
    w.steps = j.value("steps", std::vector<StepConfig>{});
}

std::vector<std::string> StepConfig::getCommands() const {
    return config.value("commands", std::vector<std::string>{});
}

std::string StepConfig::getSuccessMessage() const {
    return config.value("successMessage", std::string{});
}

std::string StepConfig::getSubmitButton() const {
    return config.value("submitButton", std::string{"§e提交"});
}

std::vector<std::string> StepConfig::getStartCommands() const {
    return config.value("startCommands", std::vector<std::string>{});
}

std::vector<std::string> StepConfig::getEndCommands() const {
    return config.value("endCommands", std::vector<std::string>{});
}

std::vector<SceneConfig> StepConfig::getScenes() const {
    if (!config.contains("scenes") || !config["scenes"].is_array()) return {};
    return config["scenes"].get<std::vector<SceneConfig>>();
}

std::string StepConfig::getCompleteHeader() const {
    return config.value("completeHeader", std::string{});
}

std::string StepConfig::getCompleteMessage() const {
    return config.value("completeMessage", std::string{});
}

std::string StepConfig::getCompleteFooter() const {
    return config.value("completeFooter", std::string{});
}

} // namespace welcome_noob
