#include "data/PlayerDataStore.h"

#include <fstream>
#include <iostream>

namespace welcome_noob {

PlayerDataStore& PlayerDataStore::getInstance() {
    static PlayerDataStore instance;
    return instance;
}

bool PlayerDataStore::load(const std::string& path) {
    mPath = path;
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        mData = nlohmann::json::object();
        save();
        return true;
    }
    try {
        ifs >> mData;
        if (!mData.is_object()) mData = nlohmann::json::object();
    } catch (const std::exception&) {
        mData = nlohmann::json::object();
    }
    return true;
}

void PlayerDataStore::save() {
    std::ofstream ofs(mPath);
    if (ofs.is_open()) {
        ofs << mData.dump(2);
    }
}

PlayerData PlayerDataStore::get(const std::string& xuid) const {
    if (xuid.empty() || !mData.is_object() || !mData.contains(xuid)) {
        return PlayerData::getDefault();
    }
    try {
        return mData[xuid].get<PlayerData>();
    } catch (const std::exception&) {
        return PlayerData::getDefault();
    }
}

void PlayerDataStore::set(const std::string& xuid, const PlayerData& data) {
    // 确保 mData 是 object，避免 null json 上用 operator[] 的未定义行为
    if (!mData.is_object()) mData = nlohmann::json::object();
    // 显式构造 json 后移动赋值，避免 mData[xuid] = data 的复杂隐式转换链
    nlohmann::json j;
    to_json(j, data);
    mData[xuid] = std::move(j);
    save();
}

bool PlayerDataStore::has(const std::string& xuid) const {
    return !xuid.empty() && mData.contains(xuid);
}

void PlayerDataStore::completeStep(const std::string& xuid, const std::string& stepKey) {
    auto data = get(xuid);
    if (!data.isStepCompleted(stepKey)) {
        data.completedSteps.push_back(stepKey);
    }
    set(xuid, data);
}

void PlayerDataStore::resetPlayer(const std::string& xuid) {
    set(xuid, PlayerData::getDefault());
}

std::string PlayerDataStore::getStatus(const std::string& xuid) const {
    return get(xuid).status;
}

} // namespace welcome_noob
