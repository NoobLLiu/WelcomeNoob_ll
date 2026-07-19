#include "guide/StepGuide.h"

#include "config/ConfigManager.h"
#include "config/StepConfig.h"
#include "data/PlayerData.h"
#include "data/PlayerDataStore.h"
#include "guide/Guides.h"
#include "hud/HudManager.h"
#include "util/Scheduler.h"
#include "util/Text.h"

#include "ll/api/service/Bedrock.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"

#include <string>

namespace welcome_noob {

void StepGuide::onStepComplete(Player& player, const std::string& stepKey) {
    std::string xuid = player.getXuid();
    if (xuid.empty()) return;

    // 1. 立即标记完成
    PlayerDataStore::getInstance().completeStep(xuid, stepKey);

    // 2. 显示成功消息
    if (const auto* step = ConfigManager::getInstance().getStep(stepKey)) {
        auto msg = step->getSuccessMessage();
        if (!msg.empty()) {
            Text::sendChat(player, msg);
        }
    }

    // 3. 停止 HUD 循环并清空屏幕
    HudManager::getInstance().stop(xuid);
    Text::clearHud(player);

    // 4. 3 秒后推进到下一步
    Scheduler::after(60, [xuid]() {
        StepGuide::advanceToNextStep(xuid);
    });
}

void StepGuide::advanceToNextStep(const std::string& xuid) {
    if (xuid.empty()) return;

    // 1. 获取玩家数据
    auto data = PlayerDataStore::getInstance().get(xuid);

    // 2. 找出第一个未完成的步骤
    const StepConfig* nextStep = nullptr;
    for (const auto& s : ConfigManager::getInstance().getSteps()) {
        if (!data.isStepCompleted(s.key)) {
            nextStep = &s;
            break;
        }
    }

    // 3. 没有未完成步骤 -> 全部完成
    if (!nextStep) {
        data.status      = "completed";
        data.currentStep = std::nullopt;
        PlayerDataStore::getInstance().set(xuid, data);

        auto* level = ll::service::bedrock::getLevel().as_ptr();
        if (level) {
            if (auto* player = level->getPlayerByXuid(xuid)) {
                updateNoobTag(*player);
                Text::sendChat(*player, "§a[新手教程] §f恭喜你完成了所有新手教程！");
            }
        }
        return;
    }

    // 4. 推进到下一步
    data.currentStep = nextStep->key;
    data.status      = "in_progress";
    PlayerDataStore::getInstance().set(xuid, data);

    auto* level = ll::service::bedrock::getLevel().as_ptr();
    if (!level) return;
    if (auto* player = level->getPlayerByXuid(xuid)) {
        startStepGuide(*player, *nextStep);
    }
}

void StepGuide::startStepGuide(Player& player, const StepConfig& step) {
    // 1. 显示步骤信息
    Text::sendChat(player, "§e§l===== §f" + step.title + " §e=====");
    Text::sendChat(player, step.description);
    Text::sendChat(player, "§a提示：§f" + step.hint);

    // 2. 启动 HUD 循环
    HudManager::getInstance().start(player.getXuid(), step);

    // 3. 根据 type 分发到具体引导
    if (step.type == "cmd_detect") {
        CmdDetectGuide::start(player, step);
    } else if (step.type == "manual_submit") {
        ManualSubmitGuide::start(player, step);
    } else if (step.type == "external_report") {
        ExternalReportGuide::start(player, step);
    } else if (step.type == "camera_tour") {
        CameraTourGuide::start(player, step);
    } else {
        // 未知类型：仅记录日志
    }
}

void StepGuide::updateNoobTag(Player& player) {
    std::string xuid = player.getXuid();
    if (xuid.empty()) return;

    auto data = PlayerDataStore::getInstance().get(xuid);
    const bool shouldHave = (data.status == "in_progress");
    const bool has        = player.hasTag("noob");

    if (shouldHave && !has) {
        player.addTag("noob");
    } else if (!shouldHave && has) {
        player.removeTag("noob");
    }
}

} // namespace welcome_noob
