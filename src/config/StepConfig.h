#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace welcome_noob {

// 相机导览场景配置
struct SceneConfig {
    int delay = 0;                          // 场景持续时间（毫秒）
    std::vector<std::string> camera;       // 相机命令序列
    std::string message;                   // 场景字幕
    int messageDelay = 0;                   // 字幕延迟（毫秒）
};

// 步骤配置
struct StepConfig {
    std::string key;
    std::string type;        // cmd_detect | manual_submit | external_report | camera_tour
    std::string title;
    std::string description;
    std::string hint;
    std::string subtitle;
    std::string actionbar;
    nlohmann::json config;  // 按 type 不同形状的子配置

    // 便捷访问器（内部从 config json 取值）
    [[nodiscard]] std::vector<std::string> getCommands() const;
    [[nodiscard]] std::string getSuccessMessage() const;
    [[nodiscard]] std::string getSubmitButton() const;
    [[nodiscard]] std::vector<std::string> getStartCommands() const;
    [[nodiscard]] std::vector<std::string> getEndCommands() const;
    [[nodiscard]] std::vector<SceneConfig> getScenes() const;
    [[nodiscard]] std::string getCompleteHeader() const;
    [[nodiscard]] std::string getCompleteMessage() const;
    [[nodiscard]] std::string getCompleteFooter() const;
};

// 顶层配置
struct WelcomeConfig {
    std::vector<StepConfig> steps;
};

// 从 JSON 反序列化（手动解析，因为字段动态）
void from_json(const nlohmann::json& j, SceneConfig& s);
void from_json(const nlohmann::json& j, StepConfig& s);
void from_json(const nlohmann::json& j, WelcomeConfig& w);

} // namespace welcome_noob
