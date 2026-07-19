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

    // 确保模组根目录存在（config.json 放在这里）
    auto modDir = mod.getModDir();
    std::error_code ec;
    std::filesystem::create_directories(modDir, ec);

    // 加载配置（config.json 放在模组根目录，与原 LSE 一致：plugins/WelcomeNoob/config.json）
    // 注意：getDataDir() 返回 plugins/WelcomeNoob/data/，getConfigDir() 返回 plugins/WelcomeNoob/config/
    //       两者都不是 config.json 应在的位置，所以用 getModDir()
    auto configPath = modDir / "config.json";
    bool configExists = std::filesystem::exists(configPath, ec);
    logger.info("Config path: {} (exists: {})", configPath.string(), configExists);
    if (!ConfigManager::getInstance().load(configPath.string())) {
        logger.error("Failed to load config from {}. File exists: {}. Using empty steps.",
            configPath.string(), configExists);
        logger.error("Please copy config.json to: {}", configPath.string());
    } else {
        const auto& steps = ConfigManager::getInstance().getSteps();
        logger.info("Loaded {} steps from config", steps.size());
        if (steps.empty()) {
            logger.error("config.json loaded but 'steps' is empty! Tutorial menu will show no steps.");
        }
    }

    // 加载玩家数据存储（players.json 放在模组根目录）
    auto playerDataPath = modDir / "players.json";
    PlayerDataStore::getInstance().load(playerDataPath.string());

    // 加载 name_to_xuid 映射（放在模组根目录）
    auto nameMapPath = modDir / "name_to_xuid.json";
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

bool WelcomeNoobMod::unload() {
    auto& logger = getSelf().getLogger();
    logger.info("WelcomeNoob unloading for hot reload...");

    // 1. 停止所有运行时状态
    HudManager::getInstance().stopAll();

    // 2. 取消所有事件订阅（含 PlayerConnectEvent、ExecutingCommandEvent、ServerCommandRegisterEvent 等）
    EventListener::unregisterAll();

    // 3. 保存持久化数据
    PlayerDataStore::getInstance().save();
    GlobalState::getInstance().saveNameMap();

    logger.info("WelcomeNoob unloaded. Hot reload will re-load config.json and re-register events.");
    return true;
}

} // namespace welcome_noob

LL_REGISTER_MOD(welcome_noob::WelcomeNoobMod, welcome_noob::WelcomeNoobMod::getInstance());
