#include "event/EventListener.h"

#include "config/ConfigManager.h"
#include "data/PlayerDataStore.h"
#include "form/Forms.h"
#include "guide/StepGuide.h"
#include "hud/HudManager.h"
#include "state/GlobalState.h"
#include "util/Scheduler.h"
#include "util/Text.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/Listener.h"
#include "ll/api/event/command/ExecutingCommandEvent.h"
#include "ll/api/event/player/PlayerDisconnectEvent.h"
#include "ll/api/event/player/PlayerJoinEvent.h"
#include "ll/api/service/Bedrock.h"

#include "mc/server/commands/CommandOrigin.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace welcome_noob {

// 存储所有已注册监听器句柄，用于 unregisterAll 清理
static std::vector<std::shared_ptr<ll::event::ListenerBase>> gListeners;

void EventListener::registerAll() {
    // ============================================================
    // 1. PlayerJoinEvent —— 玩家加入
    // ============================================================
    auto joinListener = ll::event::EventBus::getInstance().emplaceListener<
        ll::event::player::PlayerJoinEvent>(
        [](ll::event::player::PlayerJoinEvent& ev) {
            auto& player = ev.self();
            std::string xuid = player.getXuid();
            if (xuid.empty()) return;

            // 更新 name -> xuid 映射
            GlobalState::getInstance().setNameXuid(player.getName(), xuid);

            // 更新加入次数
            auto data = PlayerDataStore::getInstance().get(xuid);
            data.joinCount++;
            PlayerDataStore::getInstance().set(xuid, data);

            // 延迟 0.5 秒弹出欢迎表单（等待客户端完全初始化）
            Scheduler::after(10, [xuid]() {
                auto* level = ll::service::bedrock::getLevel().as_ptr();
                if (!level) return;
                auto* player = level->getPlayerByXuid(xuid);
                if (!player) return;

                auto data = PlayerDataStore::getInstance().get(xuid);

                if (data.status == "not_started") {
                    // 首次加入：弹出欢迎表单
                    Forms::showWelcomeForm(*player);
                } else if (data.status == "in_progress") {
                    // 已有进度，恢复
                    StepGuide::updateNoobTag(*player);
                    if (data.currentStep.has_value()) {
                        const auto* step = ConfigManager::getInstance().getStep(*data.currentStep);
                        if (step) {
                            StepGuide::startStepGuide(*player, *step);
                        }
                    }
                }
            });
        },
        ll::event::EventPriority::Normal,
        ll::mod::NativeMod::current()
    );
    if (joinListener) {
        gListeners.push_back(std::move(joinListener));
    }

    // ============================================================
    // 2. ExecutingCommandEvent —— 命令执行检测（cmd_detect 步骤）
    // ============================================================
    auto cmdListener = ll::event::EventBus::getInstance().emplaceListener<
        ll::event::command::ExecutingCommandEvent>(
        [](ll::event::command::ExecutingCommandEvent& ev) {
            // 仅处理玩家执行的命令
            auto& origin = ev.context().getOrigin();
            if (origin.getOriginType() != CommandOriginType::Player) {
                return;
            }
            auto* entity = origin.getEntity();
            if (!entity || !entity->isPlayer()) return;
            auto& player = *static_cast<Player*>(entity);

            // 检查玩家是否有进行中的 cmd_detect 步骤
            std::string xuid = player.getXuid();
            if (xuid.empty()) return;

            auto data = PlayerDataStore::getInstance().get(xuid);
            if (data.status != "in_progress" || !data.currentStep.has_value()) {
                return;
            }

            const auto* currentStep = ConfigManager::getInstance().getStep(*data.currentStep);
            if (!currentStep || currentStep->type != "cmd_detect") {
                return;
            }

            // 获取当前执行的命令名（getCommand() 返回如 "tpr" 或 "tpr arg"）
            std::string fullCmd = ev.context().getCommand();
            size_t spacePos = fullCmd.find(' ');
            std::string execCmd = (spacePos != std::string::npos) ?
                fullCmd.substr(0, spacePos) : fullCmd;

            // 检查是否匹配步骤要求的任一命令
            const auto& targetCmds = currentStep->getCommands();
            if (targetCmds.empty()) return;

            bool match = false;
            for (const auto& cmd : targetCmds) {
                if (execCmd == cmd) {
                    match = true;
                    break;
                }
            }

            if (match) {
                StepGuide::onStepComplete(player, currentStep->key);
            }
        },
        ll::event::EventPriority::Normal,
        ll::mod::NativeMod::current()
    );
    if (cmdListener) {
        gListeners.push_back(std::move(cmdListener));
    }

    // ============================================================
    // 3. PlayerDisconnectEvent —— 玩家断线
    // ============================================================
    auto disconnectListener = ll::event::EventBus::getInstance().emplaceListener<
        ll::event::player::PlayerDisconnectEvent>(
        [](ll::event::player::PlayerDisconnectEvent& ev) {
            std::string xuid = ev.self().getXuid();
            if (xuid.empty()) return;
            HudManager::getInstance().stop(xuid);
        },
        ll::event::EventPriority::Normal,
        ll::mod::NativeMod::current()
    );
    if (disconnectListener) {
        gListeners.push_back(std::move(disconnectListener));
    }
}

void EventListener::unregisterAll() {
    for (auto& listener : gListeners) {
        if (listener) {
            ll::event::EventBus::getInstance().removeListener(listener);
        }
    }
    gListeners.clear();
}

} // namespace welcome_noob
