#pragma once

#include <string>
#include <vector>

class Player;

namespace welcome_noob {

namespace Forms {

// 欢迎表单（对应 showWelcomeForm）
void showWelcomeForm(Player& player);

// 教程菜单（对应 showTutorialMenu）
void showTutorialMenu(Player& player);

// 管理员表单（对应 showAdminForm）
void showAdminForm(Player& player);

} // namespace Forms

} // namespace welcome_noob
