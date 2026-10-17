# Map Viewport & Camera {#rtui_map_viewport}

The `rtui.map` module bridges RoguelikeLib map data structures (`RL::CMap`) with RTUI terminal screen buffers (`RL::CScreen`).

---

## Drawing Maps on Screen (`RL::DrawMap`)

`RL::DrawMap` blits tiles from a `CMap` directly into a rectangular screen viewport:

```cpp
void DrawMap(CScreen& screen,
             const CMap& map,
             const Rect& area,
             Position origin = Position(0, 0));
```

### Features & Performance Optimizations
- **Origin Offset**: Renders map cells starting at `origin` within the map.
- **Pre-Clipping & Fast Striding**: Intersects the source map and destination screen viewport before looping. It reads map tiles in cache-friendly row strides and avoids individual per-cell bounds checks inside inner drawing loops.
- **Clip Safety**: Strictly respects screen clipping regions and leaves cells outside the map or viewport untouched.

---

## Camera Tracking (`RL::CameraOrigin`)

To keep a roving player character or focal point centered within a viewport without displaying ugly blank borders past the map boundary, use `CameraOrigin`:

```cpp
Position CameraOrigin(Size map_size,
                      Size viewport_size,
                      Position focus);
```

- Calculates the optimal top-left camera origin `(x, y)` to center `focus` within `viewport_size`.
- Clamps boundaries so that camera viewports smaller than the map never scroll into empty void space beyond map edges.

---

## Screen Coordinate Conversions (`RL::OnScreen`)

To convert coordinates between game map space and screen terminal coordinates (e.g. for drawing dynamic entity overlays, cursors, or mouse clicks):

```cpp
std::optional<Position> OnScreen(const Rect& area,
                                 Position origin,
                                 Position map_pos);
```

- Converts map coordinate `map_pos` to screen cell coordinates given the viewport `area` and camera `origin`.
- Returns `std::nullopt` if `map_pos` falls outside the currently visible viewport.

---

## Full Viewport Loop Example

```cpp
import rtui.screen;
import rtui.map;
import rl.map;
import rl.position;

void RenderGame(RL::CScreen& screen,
                const RL::CMap& map,
                RL::Position player_pos) {
    // Dedicated 70x30 viewport on screen starting at (2, 2)
    RL::Rect viewport_rect(2, 2, 70, 30);

    // Compute camera centered on the player
    RL::Position camera = RL::CameraOrigin(map.getSize(), viewport_rect.size, player_pos);

    // Draw visible portion of map
    RL::DrawMap(screen, map, viewport_rect, camera);

    // Draw player avatar if within viewport
    if (auto screen_pos = RL::OnScreen(viewport_rect, camera, player_pos)) {
        screen.Print(screen_pos->x, screen_pos->y, "[color=yellow]@[/color]");
    }
}
```
