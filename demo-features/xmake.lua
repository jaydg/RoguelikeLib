set_project("demo-features")
add_rules("mode.debug", "mode.release")

-- include the library
includes("../roguelikelib")
includes("../rtui")

target("demo-features")
    set_kind("binary")
    set_languages("c++20")
    set_warnings("all", "extra")
    add_files("demo-features.cpp")
    add_deps("RTUI")
