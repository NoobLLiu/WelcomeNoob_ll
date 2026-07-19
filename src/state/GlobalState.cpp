#include "state/GlobalState.h"

#include <fstream>

namespace welcome_noob {

GlobalState& GlobalState::getInstance() {
    static GlobalState instance;
    return instance;
}

bool GlobalState::loadNameMap(const std::string& path) {
    mNameMapPath = path;
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        mNameMap = nlohmann::json::object();
        saveNameMap();
        return true;
    }
    try {
        ifs >> mNameMap;
        if (!mNameMap.is_object()) mNameMap = nlohmann::json::object();
    } catch (const std::exception&) {
        mNameMap = nlohmann::json::object();
    }
    return true;
}

void GlobalState::saveNameMap() {
    std::ofstream ofs(mNameMapPath);
    if (ofs.is_open()) {
        ofs << mNameMap.dump(2);
    }
}

std::string GlobalState::xuidByName(const std::string& name) const {
    if (name.empty() || !mNameMap.contains(name)) return {};
    return mNameMap[name].get<std::string>();
}

void GlobalState::setNameXuid(const std::string& name, const std::string& xuid) {
    mNameMap[name] = xuid;
    saveNameMap();
}

void GlobalState::setLoopActive(const std::string& xuid, bool active) {
    mActiveLoops[xuid] = active;
}

bool GlobalState::isLoopActive(const std::string& xuid) const {
    auto it = mActiveLoops.find(xuid);
    return it != mActiveLoops.end() && it->second;
}

} // namespace welcome_noob
