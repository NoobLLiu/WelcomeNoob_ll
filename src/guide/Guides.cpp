#include "guide/Guides.h"

#include "config/StepConfig.h"
#include "guide/StepGuide.h"
#include "util/Scheduler.h"
#include "util/Text.h"

#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/service/Bedrock.h"

#include "mc/server/commands/CommandContext.h"
#include "mc/server/commands/PlayerCommandOrigin.h"
#include "mc/world/Container.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/actor/player/Inventory.h"
#include "mc/world/actor/player/PlayerInventory.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/Level.h"

#include <memory>
#include <string>

namespace welcome_noob {

namespace {

// 以玩家身份执行一条命令。
void runCommandAsPlayer(Player& player, const std::string& cmd) {
    if (cmd.empty()) return;
    auto origin = std::make_unique<PlayerCommandOrigin>(
        player.getLevel(),
        player.getOrCreateUniqueID()
    );
    ll::command::CommandRegistrar::getServerInstance().executeCommand(cmd, *origin);
}

} // namespace

// ============================================================
// CmdDetectGuide
// ============================================================
void CmdDetectGuide::start(Player& player, const StepConfig& /*step*/) {
    // 实际完成靠 ExecutingCommandEvent 监听触发 StepGuide::onStepComplete
    Text::sendChat(player, "§e请在聊天栏输入命令完成此步骤");
}

// ============================================================
// ManualSubmitGuide
// ============================================================
void ManualSubmitGuide::start(Player& player, const StepConfig& /*step*/) {
    // 实际完成靠表单回调中的铁锭检测
    Text::sendChat(player, "§e请打开 /noob 菜单点击提交按钮完成此步骤");
}

bool ManualSubmitGuide::checkIronIngot(Player& player, int required) {
    auto& inventory = player.getInventory();
    int   count     = inventory.getItemCount([](ItemStack const& item) {
        return !item.isNull() && item.getName() == "minecraft:iron_ingot";
    });
    return count >= required;
}

// ============================================================
// ExternalReportGuide
// ============================================================
void ExternalReportGuide::start(Player& player, const StepConfig& /*step*/) {
    Text::sendChat(player, "§e此步骤需要其他插件上报完成。完成后将自动推进。");
}

// ============================================================
// CameraTourGuide
// ============================================================
void CameraTourGuide::start(Player& player, const StepConfig& step) {
    const std::string xuid = player.getXuid();
    const std::string name = player.getRealName();
    if (xuid.empty()) return;

    // 1. 立即执行 startCommands
    for (const auto& cmd : step.getStartCommands()) {
        runCommandAsPlayer(player, Text::replaceVars(cmd, name));
    }

    // 2. 按累加延迟调度每个场景
    int totalDelay = 0;
    for (const auto& scene : step.getScenes()) {
        const int sceneStartTicks = totalDelay / 50;        // ms -> ticks
        const int msgDelayTicks   = scene.messageDelay / 50;

        // 场景开始：播放相机命令，随后按 messageDelay 显示字幕
        Scheduler::after(sceneStartTicks, [xuid, name, scene, msgDelayTicks]() {
            Player* player = Text::resolvePlayer(xuid);
            if (!player) return;
            for (const auto& cmd : scene.camera) {
                runCommandAsPlayer(*player, Text::replaceVars(cmd, name));
            }
            if (!scene.message.empty() && msgDelayTicks > 0) {
                Scheduler::after(msgDelayTicks, [xuid, scene]() {
                    Player* player = Text::resolvePlayer(xuid);
                    if (!player) return;
                    Text::sendChat(*player, scene.message);
                });
            } else if (!scene.message.empty()) {
                Text::sendChat(*player, scene.message);
            }
        });

        totalDelay += scene.delay;
    }

    // 3. 全部播放完毕后：执行 endCommands + 显示完成消息 + 触发 onStepComplete
    const int endTicks = totalDelay / 50;
    Scheduler::after(endTicks, [xuid, name, step]() {
        Player* player = Text::resolvePlayer(xuid);
        if (!player) return;

        for (const auto& cmd : step.getEndCommands()) {
            runCommandAsPlayer(*player, Text::replaceVars(cmd, name));
        }

        const auto header = step.getCompleteHeader();
        const auto message = step.getCompleteMessage();
        const auto footer = step.getCompleteFooter();
        if (!header.empty()) Text::sendChat(*player, header);
        if (!message.empty()) Text::sendChat(*player, message);
        if (!footer.empty()) Text::sendChat(*player, footer);

        StepGuide::onStepComplete(*player, step.key);
    });
}

} // namespace welcome_noob
