#include "command/Commands.h"

#include "config/ConfigManager.h"
#include "data/PlayerDataStore.h"
#include "form/Forms.h"
#include "guide/StepGuide.h"
#include "state/GlobalState.h"
#include "util/Scheduler.h"
#include "util/Text.h"

#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/Listener.h"
#include "ll/api/event/command/ServerCommandRegisterEvent.h"
#include "ll/api/service/Bedrock.h"

#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"

#include <string>

namespace welcome_noob {

void Commands::registerAll() {
    ll::event::EventBus::getInstance().emplaceListener<
        ll::event::command::ServerCommandRegisterEvent>(
        std::function<void(ll::event::command::ServerCommandRegisterEvent&)>(
            [](ll::event::command::ServerCommandRegisterEvent&) {
                // ============================================
                // 命令 1：/noob —— 打开教程菜单
                // ============================================
                auto& noobCmd = ll::command::CommandRegistrar::getServerInstance()
                                    .getOrCreateCommand(
                                        "noob",
                                        "打开新手教程菜单",
                                        CommandPermissionLevel::Any
                                    );
                noobCmd.overload().execute(
                    [](CommandOrigin const& origin, CommandOutput& output) {
                        auto* entity = origin.getEntity();
                        if (!entity || !entity->isPlayer()) {
                            output.error("该命令只能由玩家执行");
                            return;
                        }
                        auto& player = *static_cast<Player*>(entity);
                        Forms::showTutorialMenu(player);
                        output.success();
                    }
                );

                // ============================================
                // 命令 2：/noobapi —— 管理/上报 API
                // ============================================
                auto& noobApiCmd = ll::command::CommandRegistrar::getServerInstance()
                                       .getOrCreateCommand(
                                           "noobapi",
                                           "新手教程API：reset/complete/skip/report",
                                           CommandPermissionLevel::Any
                                       );

                // --- /noobapi reset <玩家名> ---
                {
                    struct ResetParams {
                        std::string playerName;
                    };
                    noobApiCmd.overload<ResetParams>()
                        .text("reset")
                        .required("playerName")
                        .execute([](CommandOrigin const& origin, CommandOutput& output,
                                    ResetParams const& params) {
                            if (origin.getPermissionsLevel() < CommandPermissionLevel::GameDirectors) {
                                output.error("权限不足");
                                return;
                            }
                            auto targetXuid = GlobalState::getInstance().xuidByName(params.playerName);
                            if (targetXuid.empty()) {
                                output.error("未找到玩家: " + params.playerName);
                                return;
                            }
                            PlayerDataStore::getInstance().resetPlayer(targetXuid);
                            output.success("已重置玩家 " + params.playerName + " 的教程进度");
                            Scheduler::after(20, [targetXuid]() {
                                auto* level = ll::service::bedrock::getLevel().as_ptr();
                                if (!level) return;
                                auto* player = level->getPlayerByXuid(targetXuid);
                                if (player) {
                                    StepGuide::updateNoobTag(*player);
                                    Forms::showWelcomeForm(*player);
                                }
                            });
                        });
                }

                // --- /noobapi complete <玩家名> [步骤key] ---
                {
                    struct CompleteParams {
                        std::string playerName;
                        std::string stepKey;
                    };
                    noobApiCmd.overload<CompleteParams>()
                        .text("complete")
                        .required("playerName")
                        .optional("stepKey")
                        .execute([](CommandOrigin const& origin, CommandOutput& output,
                                    CompleteParams const& params) {
                            if (origin.getPermissionsLevel() < CommandPermissionLevel::GameDirectors) {
                                output.error("权限不足");
                                return;
                            }
                            auto targetXuid = GlobalState::getInstance().xuidByName(params.playerName);
                            if (targetXuid.empty()) {
                                output.error("未找到玩家: " + params.playerName);
                                return;
                            }
                            // 确定要完成的步骤 key
                            std::string key = params.stepKey;
                            if (key.empty()) {
                                auto data = PlayerDataStore::getInstance().get(targetXuid);
                                key = data.currentStep.value_or("");
                            }
                            if (key.empty()) {
                                output.error("未指定步骤且玩家当前无进行中步骤");
                                return;
                            }
                            PlayerDataStore::getInstance().completeStep(targetXuid, key);
                            output.success("已标记步骤完成: " + key);

                            auto* level = ll::service::bedrock::getLevel().as_ptr();
                            if (level) {
                                if (auto* player = level->getPlayerByXuid(targetXuid)) {
                                    StepGuide::advanceToNextStep(targetXuid);
                                }
                            }
                        });
                }

                // --- /noobapi skip <玩家名> ---
                {
                    struct SkipParams {
                        std::string playerName;
                    };
                    noobApiCmd.overload<SkipParams>()
                        .text("skip")
                        .required("playerName")
                        .execute([](CommandOrigin const& origin, CommandOutput& output,
                                    SkipParams const& params) {
                            if (origin.getPermissionsLevel() < CommandPermissionLevel::GameDirectors) {
                                output.error("权限不足");
                                return;
                            }
                            auto targetXuid = GlobalState::getInstance().xuidByName(params.playerName);
                            if (targetXuid.empty()) {
                                output.error("未找到玩家: " + params.playerName);
                                return;
                            }
                            auto data = PlayerDataStore::getInstance().get(targetXuid);
                            data.status = "skipped";
                            data.currentStep.reset();
                            PlayerDataStore::getInstance().set(targetXuid, data);
                            output.success("已跳过玩家 " + params.playerName + " 的教程");

                            auto* level = ll::service::bedrock::getLevel().as_ptr();
                            if (level) {
                                if (auto* player = level->getPlayerByXuid(targetXuid)) {
                                    StepGuide::updateNoobTag(*player);
                                }
                            }
                        });
                }

                // --- /noobapi report <玩家名> <步骤key> （供外部插件调用） ---
                {
                    struct ReportParams {
                        std::string playerName;
                        std::string stepKey;
                    };
                    noobApiCmd.overload<ReportParams>()
                        .text("report")
                        .required("playerName")
                        .required("stepKey")
                        .execute([](CommandOrigin const& origin, CommandOutput& output,
                                    ReportParams const& params) {
                            if (origin.getPermissionsLevel() < CommandPermissionLevel::GameDirectors) {
                                output.error("权限不足");
                                return;
                            }
                            auto targetXuid = GlobalState::getInstance().xuidByName(params.playerName);
                            if (targetXuid.empty()) {
                                output.error("未找到玩家: " + params.playerName);
                                return;
                            }
                            // 检查该步骤是否为 external_report 类型
                            const auto* step = ConfigManager::getInstance().getStep(params.stepKey);
                            if (!step || step->type != "external_report") {
                                output.error("该步骤不存在或非 external_report 类型: " + params.stepKey);
                                return;
                            }
                            auto* level = ll::service::bedrock::getLevel().as_ptr();
                            if (!level) {
                                output.error("服务未就绪");
                                return;
                            }
                            auto* player = level->getPlayerByXuid(targetXuid);
                            if (!player) {
                                output.error("玩家不在线");
                                return;
                            }
                            StepGuide::onStepComplete(*player, params.stepKey);
                            output.success("已上报步骤完成: " + params.stepKey);
                        });
                }
            }
        ),
        ll::event::EventPriority::Normal,
        ll::mod::NativeMod::current()
    );
}

} // namespace welcome_noob
