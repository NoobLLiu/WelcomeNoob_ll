#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace welcome_noob {

// 全局状态管理
class GlobalState {
public:
    static GlobalState& getInstance();

    // ===== name_to_xuid 映射 =====
    bool loadNameMap(const std::string& path);
    void saveNameMap();
    [[nodiscard]] std::string xuidByName(const std::string& name) const;
    void setNameXuid(const std::string& name, const std::string& xuid);

    // ===== actionbar 循环标志 =====
    void setLoopActive(const std::string& xuid, bool active);
    [[nodiscard]] bool isLoopActive(const std::string& xuid) const;

    // 禁用拷贝
    GlobalState(const GlobalState&) = delete;
    GlobalState& operator=(const GlobalState&) = delete;

private:
    GlobalState() = default;

    nlohmann::json mNameMap;
    std::string mNameMapPath;
    std::unordered_map<std::string, bool> mActiveLoops;
};

} // namespace welcome_noob
