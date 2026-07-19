add_rules("mode.debug", "mode.release")

add_repositories("levimc-repo https://github.com/LiteLDev/xmake-repo.git")

option("target_type")
    set_default("server")
    set_showmenu(true)
    set_values("server", "client")
option_end()

add_requires("levilamina 26.10.14", {configs = {target_type = get_config("target_type")}})
add_requires("levibuildscript")
add_requires("nlohmann_json", {configs = {shared = false}})

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

target("WelcomeNoob")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker")
    if is_plat("windows") then
        add_defines("NOMINMAX", "UNICODE")
        set_exceptions("none") -- To avoid conflicts with /EHa.
        add_cxflags( "/EHa", "/utf-8", "/W4", "/w44265", "/w44289", "/w44296", "/w45263", "/w44738", "/w45204")
        add_cxflags(
            "/EHs",
            "-Wno-microsoft-cast",
            "-Wno-invalid-offsetof",
            "-Wno-c++2b-extensions",
            "-Wno-microsoft-include",
            "-Wno-overloaded-virtual",
            "-Wno-ignored-qualifiers",
            "-Wno-missing-field-initializers",
            "-Wno-potentially-evaluated-expression",
            "-Wno-pragma-system-header-outside-header",
            {tools = {"clang_cl"}}
        )
        set_toolchains("msvc")
    end
    add_packages("levilamina", "nlohmann_json")
    set_kind("shared")
    set_languages("c++20")
    set_symbols("debug")
    add_headerfiles("src/**.h")
    add_files("src/**.cpp")
    add_includedirs("src")
    -- 打包时把 config.json 复制到 bin/<ModName>/，让 CI 产物包含默认配置
    after_build(function(target)
        import("core.project.config")
        local modName = target:name()
        local bindir = path.join(os.projectdir(), "bin", modName)
        local srcConfig = path.join(os.projectdir(), "config.json")
        if os.isfile(srcConfig) then
            os.cp(srcConfig, path.join(bindir, "config.json"))
            cprint("${bright green}[WelcomeNoob]: ${reset}config.json copied to " .. bindir)
        end
    end)
    if is_config("target_type", "server") then
    --  add_includedirs("src-server")
    --  add_files("src-server/**.cpp")
    else
    --  add_includedirs("src-client")
    --  add_files("src-client/**.cpp")
    end
