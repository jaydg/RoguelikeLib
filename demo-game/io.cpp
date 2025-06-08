// Simple IO using Curses
#include <curses.h>

void IOInit()
{
    initscr();
    keypad(stdscr, TRUE);
    nodelay(stdscr, FALSE);
    cbreak();
    noecho();
    curs_set(0);
    leaveok(stdscr, true);

    if(has_colors()) {
        start_color();
    }
}

void IOPrintChar(int x, int y, char to_print)
{
    mvprintw(y, x, "%c", to_print);
}

void IOPrintString(int x, int y, char *to_print)
{
    mvprintw(y, x, "%s", to_print);
}

void IOPrintValue(int x, int y, int to_print)
{
    mvprintw(y, x, "%d", to_print);
}

void IORefresh()
{
    refresh();
}

int IOGetKey()
{
    return getch();
}
