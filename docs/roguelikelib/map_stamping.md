# Map Stamping & Wilderness {#roguelikelib_map_stamping}

RoguelikeLib supports compositing multiple generated sub-maps into a single master world map through **stamping** and **procedural road stitching**.

---

## The Map Stamping API

`RL::CMap::Stamp` copies a source map into a destination map at an arbitrary top-left position `(x, y)`:

```cpp
void Stamp(const CMap& source,
           Position origin,
           std::span<const std::string_view> skip_tile_types = {});
```

### Key Behaviors
- **Preserves Full Tile Properties**: Copies whole tiles including glyphs, foreground and background colours, transparency, and movement passability flags.
- **Snapshot Safety**: Overlapping self-stamps safely read an internal snapshot before writing.
- **Clipping**: Stamps are automatically clipped to the destination map's right and bottom boundaries without memory overruns. Empty maps and off-map origins are safe no-ops.
- **Tile Skipping**: An optional list of tile types leaves existing destination cells untouched. For example, skipping `"grass"` preserves surrounding wilderness terrain rather than stamping a rectangular patch of plain grass:

```cpp
import rl.map;
import rl.position;
import rl.mapgenerators;
import std;

RL::CMap wilderness(RL::Size(500, 300));
RL::CreateVoronoiWilderness(wilderness);

RL::CMap town(RL::Size(79, 50));
RL::CreateGrowthTown(town);

// Stamp town into wilderness, skipping grass tiles:
const std::array<std::string_view, 1> skip{"grass"};
wilderness.Stamp(town, RL::Position(210, 120), skip);
```

> [!TIP]
> Callers should translate any returned `RL::SBuilding` positions by the `origin` offset when storing global building metadata.

---

## Connecting Stamped Towns with Roads

To automatically connect a stamped town's gates to an existing wilderness road network, import `rl.maputils` and call `RL::StampTownWithRoads`:

```cpp
#include <span>
import rl.map;
import rl.position;
import rl.maputils;
import rl.mapgenerators.growthtown;

std::vector<RL::Position> gates;
RL::CreateGrowthTown(town, 80, &gates);

const std::size_t connected = RL::StampTownWithRoads(
    wilderness,
    town,
    RL::Position(210, 120),
    gates
);
```

`StampTownWithRoads`:
1. Stamps the town while skipping grass tiles to preserve wilderness terrain outside the ramparts.
2. Routes single-tile road connections from external gate positions to surviving wilderness `corridor` or `bridge` tiles.
3. Prefers clear ground, clears trees and rocks where necessary, and builds bridges over water.
4. Returns the total count of gates successfully connected to the road network.

---

## Voronoi Wilderness & Biome Generation

The `rl.mapgenerators.voronoiwilderness` module generates large-scale regional landscapes using Voronoi diagrams:

- **Region Scaling**: Allocates approximately 1 region per 200 tiles (minimum 10), scaling naturally with map dimensions.
- **Forest Biomes**: Feature natural density gradations from thickets to open clearings, scattered trees, passable underbrush (`plant`), and rocks. Forest roads enter clearings near boundaries and branch rather than running parallel.
- **Hill Biomes**: Walkable hills shaded continuously from green lowlands to muted ochre elevations. Contains patches of passable `scrub`, flowering `heather`, and ~2% rocks. Ground keeps its `slope` tile type for terrain rules.
- **Animal Trails & Fallen Logs**: ~30% of hills feature short, faint animal trails (`animal_trail`, `.`, 6–16 tiles long) and occasional fallen logs (`=` horizontally, `|` vertically). Both are passable and follow the map seed.
- **Chasms**: Branching, curved ravines with rocky rims (`#`, `^`), descending cliff faces (`:`), and shadowed depths (`·`). Impassable and protected from road carving.
- **Villages & Fields**: Villages feature buildings facing neighbouring structures with narrow inter-building paths. Agricultural field entrances face villages and connect via winding roads (`BuildWindingPath`).

---

## Road Building Algorithms (`rl.maputils`)

`rl.maputils` provides several route-carving algorithms:

| Helper | Path Shape | Characteristics |
| :--- | :--- | :--- |
| `BuildZigzagPath` / `AddZigzagRoad` | Sharp 90° angular turns | Fast, Manhattan-style connections. |
| `BuildSigsagPath` / `AddSigsagRoad` | Chamfered corners | Cuts 1-cell corners while retaining large turns. |
| `BuildWindingPath` / `AddWindingRoad` | Smooth natural curves | Perturbs spaced waypoints with turn-angle limits. |
| `BuildWindingRoute` | Cost-aware curves | Accepts a tile-cost callback to avoid lakes or prefer paths. |
