# Procedural Town Generation {#roguelikelib_town_generation}

The `rl.mapgenerators.growthtown` module (also exported by `rl.mapgenerators`) provides `RL::CreateGrowthTown(map, number_of_buildings = 80, gates = nullptr)`.

This generator generates a fortified medieval town complete with an uneven curtain wall, defensive ramparts, market squares, public wells, winding main roads, and cottages with procedural multi-room interiors.

---

## Town Anatomy

1. **Town Walls & Ramparts**  
   The town is encircled by an uneven perimeter wall. Rampart tiles overlap at steps to prevent movement through diagonal gaps. Four town gates provide two-tile openings connecting the town's interior to the outside countryside.
2. **Roads & Market Squares**  
   Winding main thoroughfares (three tiles wide) connect the four gates to a central market square with a well.
3. **Secondary Wells**  
   After buildings and gardens are placed, the generator identifies the largest open paved square in each quarter and places a public well at its centre. Narrow lanes and main roads are excluded; quarters lacking a square at least 3×3 tiles wide receive no additional well. All wells use the `well` tile (glyph `o`).
4. **Cottages, Plots & Gardens**  
   Cottages front onto the streets and fill back plots, separated by shared one-tile passages. Unbuilt courtyards are paved; greenery within the walls is limited to occasional two-tile gardens. Countryside outside the ramparts remains wild green terrain.
5. **Building Dimensions**  
   Most cottages measure 5–7 tiles along each axis (including exterior walls); smaller workshops and sheds fill narrow remnant plots. Building entrances can face any of the four cardinal directions.

---

## Procedural Room Interiors (Lopes et al. Adaptation)

The building interiors are inspired by **[Lopes et al., A Constrained Growth Method for Procedural Floor Plan Generation (GAMEON 2010)](https://graphics.tudelft.nl/~rafa/myPapers/bidarra.GAMEON10.pdf)**:

- **Seed Expansion**: Room seeds grow as rectangular regions and expand into remaining interior space along their frontiers based on relative area weights.
- **Door Placement**: Favours a larger public entrance room and connects the room adjacency graph.
- **Small Buildings**: Cottages with at most 9 interior tiles remain single-room spaces, avoiding cramped partitions.
- **Reachability**: All interior rooms are guaranteed to be reachable from the street using cardinal steps when doors are open.

---

## API Usage

```cpp
import rl.map;
import rl.position;
import rl.mapgenerators.growthtown;
import rl.randomness;
import std;

RL::InitRandomness(42); // Seed for reproducible generation

RL::CMap town_map(RL::Size(80, 50));
std::vector<RL::Position> gates;

// Generate town with up to 80 buildings and retrieve gate coordinates
std::vector<RL::SBuilding> buildings = RL::CreateGrowthTown(town_map, 80, &gates);

std::println("Generated {} buildings and {} gates", buildings.size(), gates.size());
for (const auto& b : buildings) {
    std::println("Building at ({}, {}) with door at ({}, {})",
                 b.rect.pos.x, b.rect.pos.y, b.door.x, b.door.y);
}
```

### Notes & Edge Cases
- **Building Count**: If the requested number of buildings exceeds the available parcel space inside the walls, the generator places as many as will fit and returns the actual list.
- **Small Maps**: Empty maps and maps smaller than 19×19 cells receive simple crossing paths without buildings to avoid generation failure.
