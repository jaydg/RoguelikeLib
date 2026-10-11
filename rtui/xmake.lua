target("RTUI")
    set_kind("static")
    set_languages("c++20")
    add_deps("RoguelikeLib", { public = true })

    add_files("**.cppm", { public = true })
