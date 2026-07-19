#include "form/Forms.h"

#include "config/ConfigManager.h"
#include "data/PlayerDataStore.h"
#include "guide/Guides.h"
#include "guide/StepGuide.h"
#include "state/GlobalState.h"
#include "util/Scheduler.h"
#include "util/Text.h"

#include "ll/api/form/CustomForm.h"
#include "ll/api/form/SimpleForm.h"
#include "ll/api/service/Bedrock.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"

#include <cstdint>
#include <string>
#include <vector>

namespace welcome_noob::Forms {

// ============================================================
// 表单 1：欢迎表单（对应原 JS showWelcomeForm L331-368）
// ============================================================
void showWelcomeForm(Player& player) {
    const std::string xuid = player.getXuid();

    ll::form::SimpleForm form("§e§l欢迎来到服务器", "§f是否开始新手教程？");
    form.appendButton("§a开始教程");
    form.appendButton("§c跳过教程");

    form.sendTo(player, [xuid](Player& p, int selected, ll::form::FormCancelReason) {
        if (selected < 0) {
            // 关闭/取消：2 秒后重弹欢迎表单
            Scheduler::after(40, [xuid]() {
                if (auto* pl = Text::resolvePlayer(xuid)) {
                    showWelcomeForm(*pl);
                }
            });
            return;
        }

        auto data = PlayerDataStore::getInstance().get(xuid);

        if (selected == 0) {
            // 开始教程
            data.status = "in_progress";
            const auto& steps = ConfigManager::getInstance().getSteps();
            std::string stepKey;
            if (!steps.empty()) {
                stepKey = steps.front().key;
                data.currentStep = stepKey;
            }
            PlayerDataStore::getInstance().set(xuid, data);
            StepGuide::updateNoobTag(p);

            // 1 秒后推送第一步引导
            Scheduler::after(20, [xuid, stepKey]() {
                if (stepKey.empty()) return;
                auto* pl = Text::resolvePlayer(xuid);
                if (pl == nullptr) return;
                const auto* step = ConfigManager::getInstance().getStep(stepKey);
                if (step != nullptr) {
                    StepGuide::startStepGuide(*pl, *step);
                }
            });
        } else if (selected == 1) {
            // 跳过教程
            data.status = "skipped";
            PlayerDataStore::getInstance().set(xuid, data);
            StepGuide::updateNoobTag(p);
            Text::sendChat(p, "§e你已跳过新手教程。随时输入 /noob 重新开始。");
        }
    });
}

// ============================================================
// 表单 2：教程菜单（对应原 JS showTutorialMenu L374-514）
// ============================================================
void showTutorialMenu(Player& player) {
    const std::string xuid = player.getXuid();
    auto data = PlayerDataStore::getInstance().get(xuid);
    const auto& steps = ConfigManager::getInstance().getSteps();
    const bool isOp = player.isOperator();

    // 构建表单内容：状态摘要 + 步骤列表
    std::string content;
    if (data.status == "completed") {
        content += "§a§l恭喜！你已完成所有新手教程！§r\n\n";
    } else if (data.status == "skipped") {
        content += "§c你跳过了教程，随时可以重新开始§r\n\n";
    }
    for (size_t i = 0; i < steps.size(); ++i) {
        const auto& s = steps[i];
        const bool isCompleted = data.isStepCompleted(s.key);
        const bool isCurrent = data.currentStep.has_value() && *data.currentStep == s.key;
        const char* icon = isCompleted ? "§a✔" : (isCurrent ? "§e▶" : "§7○");
        content += std::string(icon) + " " + std::to_string(i + 1) + ". " + s.title + "§r\n";
    }

    ll::form::SimpleForm form("§e§l新手教程");
    form.setContent(content);

    // 动态构建按钮列表，buttonActions[i] 对应按钮 i 的动作字符串
    std::vector<std::string> buttonActions;

    if (data.status == "in_progress") {
        // 继续当前步骤（仅当存在 currentStep）
        if (data.currentStep.has_value()) {
            form.appendButton("§a继续当前步骤");
            buttonActions.push_back("continue");
        }

        // manual_submit 类型步骤的提交按钮
        const StepConfig* currentStep = nullptr;
        if (data.currentStep.has_value()) {
            currentStep = ConfigManager::getInstance().getStep(*data.currentStep);
        }
        if (currentStep != nullptr && currentStep->type == "manual_submit") {
            form.appendButton(currentStep->getSubmitButton());
            buttonActions.push_back("submit_manual");
        }

        // 跳过教程
        form.appendButton("§c跳过教程");
        buttonActions.push_back("skip");

        // OP 专用：强制完成当前步骤
        if (isOp) {
            form.appendButton("§6[OP] 完成当前步骤");
            buttonActions.push_back("op_complete_step");
        }
    } else if (data.status == "not_started") {
        form.appendButton("§a开始教程");
        buttonActions.push_back("restart");
        form.appendButton("§c跳过教程");
        buttonActions.push_back("skip");
    } else {
        // completed 或 skipped：重新开始（OP 与普通玩家合并为同一按钮）
        form.appendButton("§a重新开始教程");
        buttonActions.push_back("restart");
    }

    // 始终：关闭
    form.appendButton("§7关闭");
    buttonActions.push_back("close");

    form.sendTo(player, [buttonActions, xuid](Player& p, int selected, ll::form::FormCancelReason) {
        if (selected < 0) return; // 关闭表单
        if (static_cast<size_t>(selected) >= buttonActions.size()) return;

        const auto& action = buttonActions[static_cast<size_t>(selected)];
        auto data = PlayerDataStore::getInstance().get(xuid);

        if (action == "continue") {
            if (data.currentStep.has_value()) {
                const auto* step = ConfigManager::getInstance().getStep(*data.currentStep);
                if (step != nullptr) {
                    StepGuide::startStepGuide(p, *step);
                }
            }
        } else if (action == "submit_manual") {
            if (data.currentStep.has_value()) {
                if (ManualSubmitGuide::checkIronIngot(p, 8)) {
                    StepGuide::onStepComplete(p, *data.currentStep);
                } else {
                    Text::sendChat(p, "§c你没有足够的铁锭！需要8个。");
                }
            }
        } else if (action == "skip") {
            data.status = "skipped";
            data.currentStep.reset();
            PlayerDataStore::getInstance().set(xuid, data);
            StepGuide::updateNoobTag(p);
            Text::sendChat(p, "§e你已跳过新手教程。");
        } else if (action == "op_complete_step") {
            if (data.currentStep.has_value()) {
                StepGuide::onStepComplete(p, *data.currentStep);
            }
        } else if (action == "restart") {
            PlayerDataStore::getInstance().resetPlayer(xuid);
            StepGuide::updateNoobTag(p);
            showWelcomeForm(p);
        }
        // "close" 或未知动作：不处理
    });
}

// ============================================================
// 表单 3：管理员面板（对应原 JS showAdminForm L520-664）
// ============================================================
void showAdminForm(Player& player) {
    ll::form::CustomForm form("§e§l管理员管理面板");
    form.appendInput("target", "目标玩家名", "");
    form.appendDropdown("action", "操作类型", {"重置教程", "标记完成", "标记跳过"});

    // 构建步骤选项（空 vector 会导致 dropdown 崩溃，需保证至少 1 项）
    std::vector<std::string> stepKeys;
    for (const auto& s : ConfigManager::getInstance().getSteps()) {
        stepKeys.push_back(s.key);
    }
    if (stepKeys.empty()) {
        stepKeys.emplace_back("(无步骤)");
    }
    form.appendDropdown("step", "步骤", stepKeys);

    form.sendTo(player, [stepKeys](Player& p, ll::form::CustomFormResult const& result, ll::form::FormCancelReason) {
        if (!result) return; // 玩家取消

        // 解析目标玩家名
        auto targetIt = result->find("target");
        if (targetIt == result->end()) return;
        const auto* pTarget = std::get_if<std::string>(&targetIt->second);
        if (pTarget == nullptr) return;
        std::string target = *pTarget;

        if (target.empty()) {
            Text::sendChat(p, "§c请输入目标玩家名");
            return;
        }

        // 解析操作类型索引
        auto actionIt = result->find("action");
        if (actionIt == result->end()) return;
        const auto* pActionIdx = std::get_if<uint64_t>(&actionIt->second);
        if (pActionIdx == nullptr) return;
        uint64_t actionIdx = *pActionIdx;

        // 解析步骤索引
        auto stepIt = result->find("step");
        if (stepIt == result->end()) return;
        const auto* pStepIdx = std::get_if<uint64_t>(&stepIt->second);
        if (pStepIdx == nullptr) return;
        uint64_t stepIdx = *pStepIdx;

        // 通过 GlobalState 查找目标玩家 xuid
        std::string targetXuid = GlobalState::getInstance().xuidByName(target);
        if (targetXuid.empty()) {
            Text::sendChat(p, "§c未找到玩家");
            return;
        }

        // 执行对应操作
        if (actionIdx == 0) {
            // 重置教程
            PlayerDataStore::getInstance().resetPlayer(targetXuid);
        } else if (actionIdx == 1) {
            // 标记完成（完成指定步骤）
            if (stepIdx < stepKeys.size()) {
                PlayerDataStore::getInstance().completeStep(targetXuid, stepKeys[stepIdx]);
            }
        } else if (actionIdx == 2) {
            // 标记跳过
            auto data = PlayerDataStore::getInstance().get(targetXuid);
            data.status = "skipped";
            data.currentStep.reset();
            PlayerDataStore::getInstance().set(targetXuid, data);
        }

        Text::sendChat(p, "§a操作成功");

        // 2 秒后给目标玩家弹欢迎表单（若在线）
        Scheduler::after(40, [targetXuid]() {
            if (auto* pl = Text::resolvePlayer(targetXuid)) {
                showWelcomeForm(*pl);
            }
        });
    });
}

} // namespace welcome_noob::Forms
