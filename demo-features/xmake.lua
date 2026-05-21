set_project("demo-features")
add_rules("mode.debug", "mode.release")

-- include the library
includes("../roguelikelib")

target("demo-features")
    set_kind("binary")
    set_languages("c++20")
    add_files("demo-features.cpp")
    add_deps("RoguelikeLib")
