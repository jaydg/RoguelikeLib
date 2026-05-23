// Simple IO using Notcurses

#include "io.h"
#include <notcurses/notcurses.h>
#include <cstdio>
#include <cstdlib>

// internal state variables for notcurses
static struct notcurses* nc_ctx = nullptr;
static struct ncplane* std_plane = nullptr;

void IOInit()
{
    notcurses_options opts = {0};
    // supress showing the version when starting
    opts.flags = NCOPTION_SUPPRESS_BANNERS;

    // Initialize notcurses
    nc_ctx = notcurses_init(&opts, stdout);
    if (nc_ctx == nullptr) {
        fprintf(stderr, "Error initializing Notcurses!\n");
        exit(EXIT_FAILURE);
    }

    std_plane = notcurses_stdplane(nc_ctx);
    notcurses_cursor_disable(nc_ctx);
}

void IOShutdown()
{
    if (nc_ctx != nullptr) {
        notcurses_stop(nc_ctx);
        nc_ctx = nullptr;
        std_plane = nullptr;
    }
}

void IOPrintChar(int x, int y, char to_print)
{
    ncplane_putchar_yx(std_plane, y, x, to_print);
}

void IOPrintString(int x, int y, const char* to_print)
{
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
