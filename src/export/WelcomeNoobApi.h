#pragma once
#include <string>

#ifdef WELCOMENOOB_EXPORT
#define WN_API __declspec(dllexport)
#else
#define WN_API __declspec(dllimport)
#endif

namespace welcome_noob {

// 供外部插件（如 WelcomeNoobLegacyRemoteCallApi）调用的 C++ API
// 这些函数封装了 WelcomeNoob 的核心功能，使桥接插件能通过 RemoteCall::exportAs 暴露给 LSE

// 获取玩家教程状态：'not_started' | 'in_progress' | 'completed' | 'skipped'
WN_API std::string getPlayerStatus(const std::string& xuid);

// 上报步骤完成（external_report 类型步骤）
// 校验：参数非空 + 玩家 in_progress + stepKey 存在
// 在线 -> onStepComplete（停 HUD、推进下一步）；离线 -> 仅标记完成
WN_API bool reportStepComplete(const std::string& xuid, const std::string& stepKey);

// 重置玩家教程进度
WN_API bool resetPlayer(const std::string& xuid);

} // namespace welcome_noob
