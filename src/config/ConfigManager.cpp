#include "config/ConfigManager.h"

#include "ll/api/mod/NativeMod.h"

#include <fstream>
#include <sstream>

namespace welcome_noob {

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::load(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        return false;
    }
    try {
        nlohmann::json j;
        ifs >> j;
        mConfig = j.get<WelcomeConfig>();
        mLoaded = true;
        return true;
    } catch (const std::exception& e) {
        ll::mod::NativeMod::current()->getLogger().error(
            "Failed to parse config file: {}", e.what()
        );
        return false;
    }
}

const StepConfig* ConfigManager::getStep(const std::string& key) const {
    for (const auto& s : mConfig.steps) {
        if (s.key == key) return &s;
    }
    return nullptr;
}

} // namespace welcome_noob
