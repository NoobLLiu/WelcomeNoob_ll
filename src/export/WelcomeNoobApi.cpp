#include "export/WelcomeNoobApi.h"

#include "data/PlayerDataStore.h"
#include "config/ConfigManager.h"
#include "guide/StepGuide.h"
#include "hud/HudManager.h"

#include "ll/api/service/Bedrock.h"
#include "mc/world/level/Level.h"

namespace welcome_noob {

WN_API std::string getPlayerStatus(const std::string& xuid) {
    return PlayerDataStore::getInstance().getStatus(xuid);
}

WN_API bool reportStepComplete(const std::string& xuid, const std::string& stepKey) {
    if (xuid.empty() || stepKey.empty()) return false;
    auto data = PlayerDataStore::getInstance().get(xuid);
    if (data.status != "in_progress") return false;
    if (!ConfigManager::getInstance().getStep(stepKey)) return false;
    auto* level = ll::service::bedrock::getLevel().as_ptr();
    if (level) {
        if (auto* player = level->getPlayerByXuid(xuid)) {
            StepGuide::onStepComplete(*player, stepKey);
            return true;
        }
    }
    PlayerDataStore::getInstance().completeStep(xuid, stepKey);
    return true;
}

WN_API bool resetPlayer(const std::string& xuid) {
    if (xuid.empty()) return false;
    HudManager::getInstance().stop(xuid);
    PlayerDataStore::getInstance().resetPlayer(xuid);
    return true;
}

} // namespace welcome_noob
