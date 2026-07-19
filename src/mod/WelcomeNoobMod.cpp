#include "mod/WelcomeNoobMod.h"

#include "command/Commands.h"
#include "config/ConfigManager.h"
#include "data/PlayerDataStore.h"
#include "event/EventListener.h"
#include "hud/HudManager.h"
#include "state/GlobalState.h"

#include "ll/api/mod/RegisterHelper.h"

#include <filesystem>

namespace welcome_noob {

WelcomeNoobMod& WelcomeNoobMod::getInstance() {
    static WelcomeNoobMod instance;
    return instance;
}

bool WelcomeNoobMod::load() {
    auto& mod = getSelf();
    auto& logger = mod.getLogger();
    logger.info("WelcomeNoob loading...");

    // 确保数据目录存在
    auto dataDir = mod.getDataDir();
    std::filesystem::create_directories(dataDir);

    // 加载配置
    auto configPath = dataDir / "config.json";
    if (!ConfigManager::getInstance().load(configPath.string())) {
        logger.error("Failed to load config from {}, using empty steps. Please copy config.json to this path.",
            configPath.string());
    } else {
        const auto& steps = ConfigManager::getInstance().getSteps();
        logger.info("Loaded {} steps from config", steps.size());
        if (steps.empty()) {
            logger.error("config.json loaded but 'steps' is empty! Tutorial menu will show no steps.");
        }
    }

    // 加载玩家数据存储
    auto playerDataPath = dataDir / "players.json";
    PlayerDataStore::getInstance().load(playerDataPath.string());

    // 加载 name_to_xuid 映射
    auto nameMapPath = dataDir / "name_to_xuid.json";
    GlobalState::getInstance().loadNameMap(nameMapPath.string());

    // 注册事件监听
    EventListener::registerAll();

    // 注册命令（/noob, /noobapi）
    Commands::registerAll();

    return true;
}

bool WelcomeNoobMod::enable() {
    getSelf().getLogger().info("WelcomeNoob enabled");
    return true;
}

bool WelcomeNoobMod::disable() {
    getSelf().getLogger().info("WelcomeNoob disabling...");

    // 停止所有 HUD 循环
    HudManager::getInstance().stopAll();

    // 取消事件订阅
    EventListener::unregisterAll();

    return true;
}

} // namespace welcome_noob

LL_REGISTER_MOD(welcome_noob::WelcomeNoobMod, welcome_noob::WelcomeNoobMod::getInstance());
