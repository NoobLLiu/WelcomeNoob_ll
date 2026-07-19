#include "hud/HudManager.h"

#include "config/StepConfig.h"
#include "data/PlayerDataStore.h"
#include "state/GlobalState.h"
#include "util/Scheduler.h"
#include "util/Text.h"

#include "ll/api/service/Bedrock.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"

#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace welcome_noob {

namespace {

// 跟踪当前已启动 HUD 循环的玩家 xuid 集合，用于 stopAll 遍历
std::mutex                      gActiveMutex;
std::unordered_set<std::string> gActiveXuids;

void markActive(const std::string& xuid) {
    std::lock_guard lock(gActiveMutex);
    gActiveXuids.insert(xuid);
}

void markInactive(const std::string& xuid) {
    std::lock_guard lock(gActiveMutex);
    gActiveXuids.erase(xuid);
}

} // namespace

HudManager& HudManager::getInstance() {
    static HudManager instance;
    return instance;
}

void HudManager::start(const std::string& xuid, const StepConfig& step) {
    if (xuid.empty()) return;
    GlobalState::getInstance().setLoopActive(xuid, true);
    markActive(xuid);
    loop(xuid, step);
}

void HudManager::loop(const std::string& xuid, StepConfig step) {
    // 1. 检查循环标志
    if (!GlobalState::getInstance().isLoopActive(xuid)) {
        return;
    }

    // 2. 获取关卡与玩家
    auto* level = ll::service::bedrock::getLevel().as_ptr();
    if (!level) {
        stop(xuid);
        return;
    }
    Player* player = level->getPlayerByXuid(xuid);
    if (!player) {
        stop(xuid);
        return;
    }

    // 3. 检查玩家进度状态：若已不再进行中或已切换到其他步骤，则停止本循环
    auto data = PlayerDataStore::getInstance().get(xuid);
    if (data.status != "in_progress" || !data.currentStep || *data.currentStep != step.key) {
        stop(xuid);
        return;
    }

    // 4. 推送 subtitle 与 actionbar
    Text::sendSubtitle(*player, "§e§l" + step.subtitle);
    Text::sendActionbar(*player, step.actionbar);

    // 5. 1 秒后再次刷新（20 ticks = 1 秒）
    Scheduler::after(20, [xuid, step = std::move(step)]() {
        getInstance().loop(xuid, step);
    });
}

void HudManager::stop(const std::string& xuid) {
    GlobalState::getInstance().setLoopActive(xuid, false);
    markInactive(xuid);
}

void HudManager::stopAll() {
    std::vector<std::string> snapshot;
    {
        std::lock_guard lock(gActiveMutex);
        snapshot.assign(gActiveXuids.begin(), gActiveXuids.end());
    }
    for (const auto& xuid : snapshot) {
        stop(xuid);
    }
}

} // namespace welcome_noob
