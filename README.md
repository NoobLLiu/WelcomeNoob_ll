### 注意
本项目由AI编写。

### 简介
基于 LeviLamina 的新手引导插件，支持多步骤引导、HUD 显示、相机巡游、自定义命令检测等。

### 安装
1. 将 WelcomeNoob.dll 放入 `plugins/` 目录
2. 将 config.json 放入 `plugins/WelcomeNoob/` 目录
3. 重启服务器"


--

# LeviLamina Mod Template

Mod Template for LeviLamina

## Usage

For detailed instructions, see the [LeviLamina Documentation](https://lamina.levimc.org/developer_guides/tutorials/create_your_first_mod/)

1. Generate a new repository from this template
2. Clone the new repository
3. Change the mod name and the expected LeviLamina version in `xmake.lua`
4. Add your code.
5. Run `xmake f -y -p windows -a x64 -m release` in the root of the repository
6. Run `xmake` to build the mod.

After a successful build, you will find mod in `bin/`

## Contributing

Ask questions by creating an issue.

PRs accepted.

## License

CC0-1.0 © LeviMC(LiteLDev)
