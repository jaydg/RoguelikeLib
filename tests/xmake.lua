set_project("roguelikelib-tests")
add_rules("mode.debug", "mode.release")

-- the libraries
includes("../roguelikelib")
includes("../rtui")

-- One binary with every test, and each test a run of it by its name, as
-- main.cpp lists them: `xmake test` runs them all, side by side, and tells
-- of each; `xmake test roguelikelib-tests/Screen` runs one
target("roguelikelib-tests")
    set_kind("binary")
    set_default(false)
    set_languages("c++20")
    set_warnings("all", "extra")
    add_files("*.cppm", "*.cpp")
    add_deps("RTUI")

    on_load(function (target)
        local main = io.readfile(path.join(os.scriptdir(), "main.cpp"))

        for name in main:gmatch('{"(%w+)", Test%w+}') do
            target:add("tests", name, {runargs = name})
        end
    end)
