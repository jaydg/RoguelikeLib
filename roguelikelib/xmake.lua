target("permissive_fov")
    set_kind("static")
    set_languages("c99")
    add_includedirs("extern")
    add_files("extern/permissive-fov.cc")

target("RoguelikeLib")
    set_kind("static")
    set_languages("c++20")
    add_deps("permissive_fov")

    -- {public = true} ensures the module can be imported
    add_files("*.cppm", {public = true})
    add_files("mapgenerators/*.cppm", {public = true})
