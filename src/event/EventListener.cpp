#include "event/EventListener.h"

#include "config/ConfigManager.h"
#include "data/PlayerDataStore.h"
#include "form/Forms.h"
#include "guide/StepGuide.h"
#include "hud/HudManager.h"
#include "state/GlobalState.h"
#include "util/Scheduler.h"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/Listener.h"
#include "ll/api/event/player/PlayerConnectEvent.h"
#include "ll/api/service/Bedrock.h"

#include "mc/server/ServerPlayer.h"
#include "mc/world/level/Level.h"

#include <memory>
#include <string>
#include <vector>

namespace welcome_noob {

// 存储所有已注册监听器句柄，用于 unregisterAll 清理
static std::vector<std::shared_ptr<ll::event::ListenerBase>> gListeners;

// Hook: ServerPlayer::disconnect —— 替代 PlayerDisconnectEvent
LL_AUTO_TYPE_INSTANCE_HOOK(
    PlayerDisconnectHook,
    ll::memory::HookPriority::Normal,
    ServerPlayer,
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
                GlobalState::getInstance().setNameXuid(player.getName(), xuid);

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
