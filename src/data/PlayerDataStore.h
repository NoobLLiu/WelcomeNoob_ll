#pragma once

#include "data/PlayerData.h"

#include <optional>
#include <string>

namespace welcome_noob {

// 封装玩家数据持久化存储（基于单个 JSON 文件，简单可靠）
class PlayerDataStore {
public:
    static PlayerDataStore& getInstance();

    // 初始化：加载或创建存储文件
    bool load(const std::string& path);

    // 保存到磁盘
    void save();

    // 获取玩家数据（无记录则返回默认）
    [[nodiscard]] PlayerData get(const std::string& xuid) const;

    // 设置玩家数据
    void set(const std::string& xuid, const PlayerData& data);

    // 是否存在记录
    [[nodiscard]] bool has(const std::string& xuid) const;

    // 标记步骤完成
    void completeStep(const std::string& xuid, const std::string& stepKey);

    // 重置玩家进度
    void resetPlayer(const std::string& xuid);

    // 获取玩家状态字符串
    [[nodiscard]] std::string getStatus(const std::string& xuid) const;

private:
    PlayerDataStore() = default;
    nlohmann::json mData;       // { xuid: PlayerData-obj, ... }
    std::string mPath;
};

} // namespace welcome_noob
