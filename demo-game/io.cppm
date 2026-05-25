// Simple IO using Notcurses

module;

#include <notcurses/notcurses.h>

export module demo_game.io;

import std;

// internal state variables for notcurses
static struct notcurses* nc_ctx = nullptr;
static struct ncplane* std_plane = nullptr;

export {

constexpr std::uint32_t IO_default_fg = 0xEEEEEE;

void IOInit()
{
    notcurses_options opts = {
        // supress showing the version when starting
        .flags = NCOPTION_SUPPRESS_BANNERS
    };

    // Initialize notcurses
    nc_ctx = notcurses_init(&opts, stdout);
    if (nc_ctx == nullptr) {
        fprintf(stderr, "Error initializing Notcurses!\n");
        exit(EXIT_FAILURE);
    }

    notcurses_cursor_disable(nc_ctx);
    std_plane = notcurses_stdplane(nc_ctx);
}

void IOShutdown()
{
    if (nc_ctx != nullptr) {
        notcurses_stop(nc_ctx);
        nc_ctx = nullptr;
        std_plane = nullptr;
    }
}

void IOPrintChar(int x, int y, char to_print, std::optional<std::uint32_t> rgb_fg = std::nullopt)
{
    std::uint32_t fg = rgb_fg.has_value() ? rgb_fg.value() : IO_default_fg;

    nccell c = NCCELL_TRIVIAL_INITIALIZER;
    nccell_load(std_plane, &c, &to_print);
    nccell_set_fg_rgb(&c, fg);
    ncplane_putc_yx(std_plane, y, x, &c);
    nccell_release(std_plane, &c);
}


void IOPrintString(int x, int y, const char* to_print, std::optional<std::uint32_t> rgb_fg = std::nullopt)
{
    std::uint32_t fg = rgb_fg.has_value() ? rgb_fg.value() : IO_default_fg;

    ncplane_set_fg_rgb(std_plane, fg);
    ncplane_putstr_yx(std_plane, y, x, to_print);
}

void IOPrintValue(int x, int y, int to_print)
{
    ncplane_printf_yx(std_plane, y, x, "%d", to_print);
}

void IORefresh()
{
    notcurses_render(nc_ctx);
}

int IOGetKey()
{
    ncinput ni;
    // blocks until any event (keyboard, mouse, window resize)
    uint32_t id = notcurses_get_blocking(nc_ctx, &ni);

    // Return the character code. When missing, notcurses returns (uint32_t)-1
    return static_cast<int>(id);
}

} // export
