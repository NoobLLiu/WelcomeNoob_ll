#include "util/Text.h"

#include "ll/api/service/Bedrock.h"

#include "mc/server/commands/CommandOrigin.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"

namespace welcome_noob::Text {

void sendChat(Player& p, const std::string& msg) {
    p.sendMessage(msg);
}

void sendActionbar(Player& p, const std::string& msg) {
    // 对应 LSE tell(msg, 4) - actionbar
    // 通过 setTitle 实现：setTitle(text, type, ...)，type=3 为 Actionbar
    if (msg.empty()) {
        p.setTitle("", TitleType::Actionbar);
    } else {
        p.setTitle(msg, TitleType::Actionbar);
    }
}

void sendSubtitle(Player& p, const std::string& msg) {
    // 对应 LSE tell(msg, 5) - subtitle
    // type=1 为 Subtitle，需先 setTitle 主标题为空
    p.setTitle("", TitleType::Title);
    if (!msg.empty()) {
        p.setTitle(msg, TitleType::Subtitle);
    }
}

void clearHud(Player& p) {
    p.setTitle("", TitleType::Actionbar);
    p.setTitle("", TitleType::Subtitle);
}

std::string replaceVars(const std::string& cmd, const std::string& playerName) {
    std::string result = cmd;
    const std::string placeholder = "${name}";
    size_t pos = 0;
    while ((pos = result.find(placeholder, pos)) != std::string::npos) {
        result.replace(pos, placeholder.length(), playerName);
        pos += playerName.length();
    }
    return result;
}

Player* resolvePlayer(const std::string& xuid) {
    if (xuid.empty()) return nullptr;
    auto* level = ll::service::bedrock::getLevel().as_ptr();
    if (!level) return nullptr;
    return level->getPlayerByXuid(xuid);
}

} // namespace welcome_noob::Text
