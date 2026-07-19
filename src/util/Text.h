#pragma once

#include <string>

class Player;

namespace welcome_noob {

// 消息发送封装（对应 LSE 的 player.tell(msg, type)）
namespace Text {

// 普通聊天消息
void sendChat(Player& p, const std::string& msg);

// actionbar（对应 LSE tell(msg, 4)）
void sendActionbar(Player& p, const std::string& msg);

// subtitle（对应 LSE tell(msg, 5)）
void sendSubtitle(Player& p, const std::string& msg);

// 清空 actionbar 和 subtitle
void clearHud(Player& p);

// 字符串变量替换：将 cmd 中的 ${name} 替换为 playerName
[[nodiscard]] std::string replaceVars(const std::string& cmd, const std::string& playerName);

// 通过 xuid 查找在线玩家，离线返回 nullptr。
// 用于 Scheduler::after 等延迟回调中安全获取 Player 指针。
[[nodiscard]] Player* resolvePlayer(const std::string& xuid);

} // namespace Text

} // namespace welcome_noob
