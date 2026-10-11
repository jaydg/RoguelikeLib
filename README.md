Roguelike Lib
=============

Roguelike Library is portable open-source library written in C++. It consist
of set of classes that can be used in all roguelike games. Classes are
categorized to fullfill tasks of random map generation, pathfinding,
counting field of view and making up names.

This project was was originally developed by Jakub Debski between 2006 and
2007. It was originally hosted on [SourceForge](https://sf.net/p/roguelikelib/)
and abandoned many years ago.

Licensed under the BSD License.

Terminal UI
-----------

Reusable terminal UI code lives in `rtui` (Roguelike Terminal UI), separate
from the core algorithms in `roguelikelib`. Include both directories in
Xmake and depend on the `RTUI` target, which publicly depends on
`RoguelikeLib`.

The UI modules are `rtui.screen`, `rtui.windows`, `rtui.mouse`,
`rtui.utf8`, `rtui.terminal`, and `rtui.map`; terminal colours are provided by `stc` in
`rtui/extern`. C++ types and functions retain their `RL` namespace.
Import `rtui.map` and call `RL::PrintMap(map)` to print a map (previously
`CMap::PrintMap()`). Import `rtui.utf8` explicitly when encoding glyphs;
`rl.tile` no longer re-exports it. The core library can be used without
terminal UI dependencies.

`rtui.terminal` provides the POSIX terminal layer adapted from Roguevalley:
`RL::CTerminal` owns raw input mode, the alternate screen, cursor visibility,
mouse reporting, and restoration on destruction, exit, or termination signals.
Only one terminal object can be active at a time, and stdin and stdout must
both be terminals. `ReadKey(timeout_ms)` returns decoded Unicode keys, mouse
reports, resize events, or closure; `getSize()` returns columns and rows.
`DecodeKey()` can also decode buffered input without opening a terminal.

```cpp
import rtui.terminal;
import rtui.screen;

// Inside the application's main function:
RL::CTerminal terminal;
RL::CScreen screen(terminal.getSize());
screen.Print(0, 0, "Hello from RTUI");
terminal.Write(screen.Compose(true));
const auto key = terminal.ReadKey();
```

Credits
-------

Besides the original work of Jakub Debski, the library contains the work of
others, as listed in [AUTHORS](AUTHORS):

  * Kusigrosz wrote the delve map generator (`rl.mapgenerators.delve`), the
    winding path generator, and the name generator (`rl.names`), which makes
    up names that sound like those it is given, a list of English names, for
    instance. All of them were placed in the public
    domain; the original descriptions are kept at the top of each module.
  * Adam Milazzo wrote the field of view algorithm (`rl.fov`).
  * illyigan wrote the terminal colours code (stc).

Demos
-----

There are two demos included with this library: a demo game and a feature
demo. Both can be built using [Xmake](https://xmake.io/):

```sh
cd demo-game
xmake config -m debug
xmake build
```

for the demo game and the following commands for the feature demo:

```sh
cd demo-features
xmake config
xmake build
```

You can run the generated executables with `xmake run`.
