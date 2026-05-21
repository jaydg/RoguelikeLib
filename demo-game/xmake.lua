set_project("demo-game")
add_rules("mode.debug", "mode.release")

add_requires(
    "notcurses",
    {
        system = true,
        configs = { pkg_config = true }
    }
)

-- include the library
includes("../roguelikelib")

target("demo-game")
    set_kind("binary")
    set_languages("c++20")
    add_files("*.cpp")
    add_packages("notcurses")
    add_deps("RoguelikeLib")
