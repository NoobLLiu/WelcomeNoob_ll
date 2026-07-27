#include "event/EventListener.h"

#include "config/ConfigManager.h"
#include "config/StepConfig.h"
#include "data/PlayerDataStore.h"
#include "form/Forms.h"
#include "guide/StepGuide.h"
#include "hud/HudManager.h"
#include "state/GlobalState.h"
#include "util/Scheduler.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/Listener.h"
#include "ll/api/event/command/ExecuteCommandEvent.h"
#include "ll/api/event/player/PlayerConnectEvent.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/service/Bedrock.h"

#include "mc/server/ServerPlayer.h"
#include "mc/server/commands/CommandContext.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <string>
#include <vector>

namespace welcome_noob {

// 存储所有已注册监听器句柄，用于 unregisterAll 清理
static std::vector<std::shared_ptr<ll::event::ListenerBase>> gListeners;

// Hook: ServerPlayer::disconnect —— 替代 PlayerDisconnectEvent
LL_AUTO_INSTANCE_HOOK_IMPL(
    PlayerDisconnectHook,
    ServerPlayer,
    HookPriority::Normal,
    ll::memory::SaveNone,
    &ServerPlayer::disconnect,
    void
) {
    std::string xuid = this->getXuid();
    if (!xuid.empty()) {
        HudManager::getInstance().stop(xuid);
    }
    origin();
}

void EventListener::registerAll() {
    // ============================================================
    // 1. PlayerConnectEvent —— 玩家连接
    //    (LL 26.10.14 无 PlayerJoinEvent，用 PlayerConnectEvent 替代)
    // ============================================================
    auto connectListener = ll::event::EventBus::getInstance().emplaceListener<
        ll::event::player::PlayerConnectEvent>(
        std::function<void(ll::event::player::PlayerConnectEvent&)>(
            [](ll::event::player::PlayerConnectEvent& ev) {
                auto& player = ev.self();
                std::string xuid = player.getXuid();
                if (xuid.empty()) return;

                // 更新 name -> xuid 映射
                GlobalState::getInstance().setNameXuid(player.getRealName(), xuid);

                // 更新加入次数
                auto data = PlayerDataStore::getInstance().get(xuid);
                data.joinCount++;
                PlayerDataStore::getInstance().set(xuid, data);

                // 延迟 1 秒弹出欢迎表单（PlayerConnectEvent 触发时玩家尚未完全加载）
                Scheduler::after(20, [xuid]() {
                    auto* level = ll::service::bedrock::getLevel().as_ptr();
                    if (!level) return;
                    auto* player = level->getPlayerByXuid(xuid);
                    if (!player) return;

                    auto data = PlayerDataStore::getInstance().get(xuid);

                    if (data.status == "not_started") {
                        Forms::showWelcomeForm(*player);
                    } else if (data.status == "in_progress") {
                        StepGuide::updateNoobTag(*player);
                        if (data.currentStep.has_value()) {
                            const auto* step = ConfigManager::getInstance().getStep(*data.currentStep);
                            if (step) {
                                StepGuide::startStepGuide(*player, *step);
                            }
                        }
                    }
                });
            }
        ),
        ll::event::EventPriority::Normal,
        ll::mod::NativeMod::current()
    );
    if (connectListener) {
        gListeners.push_back(std::move(connectListener));
    }

    // ============================================================
    // 2. ExecutingCommandEvent -- 命令执行（cmd_detect 类型步骤检测）
    //    对应原 LSE: mc.listen('onPlayerCmd', (player, cmd) => {...})
    //    监听所有命令执行，检查是否匹配玩家当前 cmd_detect 步骤的 commands 列表
    // ============================================================
    auto cmdListener = ll::event::EventBus::getInstance().emplaceListener<
        ll::event::command::ExecutingCommandEvent>(
        std::function<void(ll::event::command::ExecutingCommandEvent&)>(
            [](ll::event::command::ExecutingCommandEvent& ev) {
                // 1. 取命令发送者，仅处理玩家执行的命令
                auto& ctx = ev.commandContext();
                auto* entity = ctx.mOrigin->getEntity();
                if (!entity || !entity->isPlayer()) return;
                auto& player = *static_cast<Player*>(entity);

                std::string xuid = player.getXuid();
                if (xuid.empty()) return;

                // 2. 玩家必须在教程进行中
                auto data = PlayerDataStore::getInstance().get(xuid);
                if (data.status != "in_progress") return;
                if (!data.currentStep.has_value()) return;

                // 3. 当前步骤必须是 cmd_detect 类型
                const auto* step = ConfigManager::getInstance().getStep(*data.currentStep);
                if (!step || step->type != "cmd_detect") return;

                // 4. 取命令字符串，去除前导 '/'，转小写
                std::string cmd = ctx.mCommand;
                if (!cmd.empty() && cmd[0] == '/') cmd = cmd.substr(1);
                // 转小写
                std::string cmdLower = cmd;
                std::transform(cmdLower.begin(), cmdLower.end(), cmdLower.begin(),
                    [](unsigned char c) { return std::tolower(c); });

                // 5. 与步骤配置的 commands 列表比对
                //    匹配规则（与原 LSE 一致）：完全匹配 或 以 target + ' ' 开头
                auto targetCmds = step->getCommands();
                for (const auto& target : targetCmds) {
                    std::string targetLower = target;
                    std::transform(targetLower.begin(), targetLower.end(), targetLower.begin(),
                        [](unsigned char c) { return std::tolower(c); });

                    if (cmdLower == targetLower ||
                        cmdLower.compare(0, targetLower.size() + 1, targetLower + " ") == 0) {
                        // 匹配成功：触发步骤完成（不取消命令）
                        StepGuide::onStepComplete(player, step->key);
                        return;
                    }
                }
            }
        ),
        ll::event::EventPriority::Normal,
        ll::mod::NativeMod::current()
    );
    if (cmdListener) {
        gListeners.push_back(std::move(cmdListener));
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
