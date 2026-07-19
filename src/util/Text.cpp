#include "util/Text.h"

#include "ll/api/service/Bedrock.h"

#include "mc/network/packet/SetTitlePacket.h"
#include "mc/network/packet/SetTitlePacketPayload.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"

namespace welcome_noob::Text {

void sendChat(Player& p, const std::string& msg) {
    p.sendMessage(msg);
}

void sendActionbar(Player& p, const std::string& msg) {
    // 通过 SetTitlePacket::TitleType::Actionbar 实现
    // 使用 direct-initialization + guaranteed copy elision 避免 SetTitlePacketPayload 的拷贝构造
    SetTitlePacket packet(SetTitlePacketPayload(
        SetTitlePacketPayload::TitleType::Actionbar,
        msg.empty() ? std::string{} : std::string{msg},
        std::nullopt
    ));
    p.sendNetworkPacket(packet);
}

void sendSubtitle(Player& p, const std::string& msg) {
    // 先发送空 Title，再发送 Subtitle（Minecraft 协议要求 Subtitle 需先有 Title）
    {
        SetTitlePacket packet(SetTitlePacketPayload(
            SetTitlePacketPayload::TitleType::Title,
            std::string{},
            std::nullopt
        ));
        p.sendNetworkPacket(packet);
    }
    if (!msg.empty()) {
        SetTitlePacket packet(SetTitlePacketPayload(
            SetTitlePacketPayload::TitleType::Subtitle,
            std::string{msg},
            std::nullopt
        ));
        p.sendNetworkPacket(packet);
    }
}

void clearHud(Player& p) {
    SetTitlePacket packet(SetTitlePacketPayload(
        SetTitlePacketPayload::TitleType::Clear,
        std::string{},
        std::nullopt
    ));
    p.sendNetworkPacket(packet);
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
