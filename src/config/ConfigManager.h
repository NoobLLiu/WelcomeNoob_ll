#pragma once

#include "config/StepConfig.h"

#include <optional>
#include <string>

namespace welcome_noob {

class ConfigManager {
public:
    static ConfigManager& getInstance();

    // 加载配置文件。path 形如 "<modDataDir>/config.json"
    bool load(const std::string& path);

    [[nodiscard]] const WelcomeConfig& getConfig() const { return mConfig; }
    [[nodiscard]] const std::vector<StepConfig>& getSteps() const { return mConfig.steps; }
    [[nodiscard]] const StepConfig* getStep(const std::string& key) const;

private:
    ConfigManager() = default;
    WelcomeConfig mConfig;
    bool mLoaded = false;
};

} // namespace welcome_noob
