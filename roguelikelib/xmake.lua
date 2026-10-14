target("RoguelikeLib")
    set_kind("static")
    set_languages("c++20")
    set_warnings("all", "extra")

    -- {public = true} ensures the module can be imported
    add_files("**.cppm", { public = true })
    add_files("mapgenerators/*.cppm", { public = true })
