import rl.map;
import rl.maputils;
import rl.mapgenerators;
import rl.position;
import rl.tile;
import rl.randomness;
import rltests.check;
import std;

using RL::tests::Check;

namespace
{
bool SameTile(const RL::CTile& a, const RL::CTile& b)
{
    return a.getType() == b.getType() && a.getGlyph() == b.getGlyph() && a.getColor() == b.getColor() &&
           a.isTransparent() == b.isTransparent() && a.isPassable() == b.isPassable();
}
}

void TestMapStamp()
{
    RL::CMap source = RL::tests::MakeMap({".:T", "&~^"});
    source.get(0, 0) = RL::CTile("grass", {U'@', 0x123456, false, false});
    RL::CMap target(RL::Size(8, 6));
    target.Clear("room");
    const auto original = target;
    const auto source_original = source;
    const auto random_state = RL::GetRandomState();
    target.Stamp(source, RL::Position(2, 3));
    bool exact = true, surrounding = true, unchanged = true;
    for (std::size_t y = 0; y < target.getHeight(); ++y) {
        for (std::size_t x = 0; x < target.getWidth(); ++x) {
            if (x >= 2 && x < 5 && y >= 3 && y < 5) {
                exact &= SameTile(target.get(x, y), source.get(x - 2, y - 3));
            } else {
                surrounding &= SameTile(target.get(x, y), original.get(x, y));
            }
        }
    }
    for (std::size_t y = 0; y < source.getHeight(); ++y) {
        for (std::size_t x = 0; x < source.getWidth(); ++x) {
            unchanged &= SameTile(source.get(x, y), source_original.get(x, y));
        }
    }
    Check(exact && surrounding && unchanged, "stamps exact tiles at the offset, preserving source and surroundings");
    Check(RL::GetRandomState() == random_state, "stamping consumes no randomness");

    target = original;
    const std::array<std::string_view, 2> skip{"grass", "water"};
    target.Stamp(source, RL::Position(2, 3), skip);
    Check(SameTile(target.get(2, 3), original.get(2, 3)) && SameTile(target.get(3, 4), original.get(3, 4)) &&
          SameTile(target.get(4, 3), source.get(2, 0)), "skipped tile types leave the destination intact");

    target = original;
    target.Stamp(source, RL::Position(6, 5));
    Check(SameTile(target.get(6, 5), source.get(0, 0)) && SameTile(target.get(7, 5), source.get(1, 0)) &&
          SameTile(target.get(5, 5), original.get(5, 5)), "clips at right and bottom edges");
    const auto clipped = target;
    target.Stamp(source, RL::Position(8, 0));
    target.Stamp(source, RL::Position(0, 6));
    target.Stamp(source, RL::Position());
    target.Stamp(RL::CMap(RL::Size(0, 3)), RL::Position(0, 0));
    target.Stamp(RL::CMap(RL::Size(3, 0)), RL::Position(0, 0));
    bool no_op = true;
    for (std::size_t y = 0; y < target.getHeight(); ++y) {
        for (std::size_t x = 0; x < target.getWidth(); ++x) {
            no_op &= SameTile(target.get(x, y), clipped.get(x, y));
        }
    }
    Check(no_op, "off-map origins and empty sources are no-ops without overflow");
    RL::CMap empty(RL::Size(0, 0));
    empty.Stamp(source, RL::Position(0, 0));

    auto self = source;
    self.Stamp(self, RL::Position(1, 1));
    Check(SameTile(self.get(1, 1), source.get(0, 0)) && SameTile(self.get(2, 1), source.get(1, 0)),
          "overlapping self-stamps copy the original cells");
    self = source;
    self.Stamp(self, RL::Position(1, 0));
    Check(SameTile(self.get(1, 0), source.get(0, 0)) && SameTile(self.get(2, 0), source.get(1, 0)),
          "horizontal self-stamps do not cascade a tile into the next cell");
    self = source;
    self.Stamp(self, RL::Position(1, 0), skip);
    Check(SameTile(self.get(1, 0), source.get(1, 0)) && SameTile(self.get(2, 0), source.get(1, 0)),
          "masked self-stamps preserve skipped destinations and avoid cascading writes");
}


void TestTownRoads()
{
    RL::CMap town(RL::Size(3, 3));
    town.SetCell(1, 1, "room");
    town.SetCell(0, 1, "corridor");
    const std::array gates{RL::Position(0, 1)};
    RL::CMap landscape(RL::Size(9, 7));
    landscape.Clear("grass");
    landscape.SetCell(0, 3, "corridor");
    for (std::size_t y = 0; y < 7; ++y) {
        landscape.SetCell(1, y, "water");
    }
    const auto old_road = landscape.get(0, 3);
    Check(RL::StampTownWithRoads(landscape, town, RL::Position(3, 2), gates) == 1,
          "joins a town gate to the existing wilderness road");
    Check(landscape.get(1, 3).getType() == "bridge" && landscape.get(2, 3).getType() == "corridor",
          "the gate connection bridges the river and ends at the forest road");
    Check(SameTile(landscape.get(0, 3), old_road), "preserves the original road tile");
    bool intact = true;
    for (std::size_t y = 0; y < 3; ++y) {
        for (std::size_t x = 0; x < 3; ++x) {
            intact &= SameTile(landscape.get(x + 3, y + 2), town.get(x, y));
        }
    }
    Check(intact, "connections preserve every stamped wall and interior tile");
    landscape.Clear("grass");
    Check(RL::StampTownWithRoads(landscape, town, RL::Position(3, 2), gates) == 0 &&
          landscape.get(2, 3).getType() == "grass" && landscape.get(3, 3).getType() == "corridor",
          "without an existing road network the town is stamped without inventing road stubs");
    Check(RL::StampTownWithRoads(landscape, town, RL::Position(), gates) == 0,
          "an off-map stamp does not create connectors");
    for (unsigned seed = 0; seed < 20; ++seed) {
        RL::InitRandomness(seed);
        RL::CMap generated(RL::Size(79, 50));
        std::vector<RL::Position> generated_gates;
        RL::CreateGrowthTown(generated, 80, &generated_gates);
        RL::CMap surrounding(RL::Size(99, 70));
        surrounding.Clear("grass");
        surrounding.SetCell(0, 35, "corridor");
        Check(RL::StampTownWithRoads(surrounding, generated, RL::Position(10, 10), generated_gates) == 4,
              "all four generated gates connect to the surrounding road network");
        bool preserved = true;
        for (std::size_t y = 0; y < generated.getHeight(); ++y) {
            for (std::size_t x = 0; x < generated.getWidth(); ++x) {
                if (generated.get(x, y).getType() != "grass") {
                    preserved &= SameTile(surrounding.get(x + 10, y + 10), generated.get(x, y));
                }
            }
        }
        Check(preserved, "connecting generated towns preserves all stamped tiles");
    }
}

void TestSymmetricStructure()
{
    RL::InitRandomness(1234);
    RL::CMap shuttle(RL::Size(79, 50));
    RL::CreateSpaceShuttle(shuttle, 20);

    std::size_t shuttle_rooms = 0;
    std::size_t shuttle_walls = 0;
    for (std::size_t y = 0; y < shuttle.getHeight(); ++y) {
        for (std::size_t x = 0; x < shuttle.getWidth(); ++x) {
            if (shuttle.get(x, y).getType() == "room") shuttle_rooms++;
            if (shuttle.get(x, y).getType() == "wall") shuttle_walls++;
        }
    }
    Check(shuttle_rooms > 200 && shuttle_walls > 200, "space shuttle generates rooms and walls");

    RL::InitRandomness(1234);
    RL::CMap castle(RL::Size(79, 50));
    RL::CreateCastle(castle, 20);

    std::size_t castle_rooms = 0;
    std::size_t castle_walls = 0;
    std::size_t lr_matches = 0;
    for (std::size_t y = 0; y < castle.getHeight(); ++y) {
        for (std::size_t x = 0; x < castle.getWidth() / 2; ++x) {
            const auto& left = castle.get(x, y);
            const auto& right = castle.get(castle.getWidth() - 1 - x, y);
            if (left.getType() == right.getType()) {
                lr_matches++;
            }
        }
    }
    for (std::size_t y = 0; y < castle.getHeight(); ++y) {
        for (std::size_t x = 0; x < castle.getWidth(); ++x) {
            if (castle.get(x, y).getType() == "room") castle_rooms++;
            if (castle.get(x, y).getType() == "wall") castle_walls++;
        }
    }
    Check(castle_rooms > 200 && castle_walls > 200, "castle generates rooms and walls");
    // Most cells in the left and right halves should match symmetrically
    const std::size_t total_half_cells = (castle.getWidth() / 2) * castle.getHeight();
    Check(lr_matches > total_half_cells * 9 / 10, "castle exhibits strong left-right symmetry");

    // Test void_outside
    RL::InitRandomness(1234);
    RL::CMap void_castle(RL::Size(79, 50));
    RL::CreateCastle(void_castle, 20, true);
    std::size_t void_tiles = 0;
    bool valid_voids = true;
    for (std::size_t y = 0; y < void_castle.getHeight(); ++y) {
        for (std::size_t x = 0; x < void_castle.getWidth(); ++x) {
            if (void_castle.get(x, y).getType() == "void") {
                void_tiles++;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        const int nx = static_cast<int>(x) + dx;
                        const int ny = static_cast<int>(y) + dy;
                        if (nx >= 0 && nx < static_cast<int>(void_castle.getWidth()) &&
                            ny >= 0 && ny < static_cast<int>(void_castle.getHeight())) {
                            const auto type = void_castle.get(nx, ny).getType();
                            if (type != "wall" && type != "void") {
                                valid_voids = false;
                            }
                        }
                    }
                }
            }
        }
    }
    Check(void_tiles > 500, "void_outside generates outside void tiles");
    Check(valid_voids, "void tiles never border room interiors directly without walls");

    // Test stamping void_castle into a background
    RL::CMap background(RL::Size(79, 50));
    background.Clear("grass");
    const std::array<std::string_view, 1> skip{"void"};
    background.Stamp(void_castle, RL::Position(0, 0), skip);

    std::size_t preserved_grass = 0;
    std::size_t stamped_rooms = 0;
    for (std::size_t y = 0; y < background.getHeight(); ++y) {
        for (std::size_t x = 0; x < background.getWidth(); ++x) {
            if (background.get(x, y).getType() == "grass") preserved_grass++;
            if (background.get(x, y).getType() == "room") stamped_rooms++;
        }
    }
    Check(preserved_grass == void_tiles, "stamping with void mask preserves exact outside background");
    Check(stamped_rooms == castle_rooms, "stamping transfers all castle rooms onto background");

    // Door validity check
    std::size_t invalid_doors = 0;
    std::size_t total_doors = 0;
    for (std::size_t y = 0; y < void_castle.getHeight(); ++y) {
        for (std::size_t x = 0; x < void_castle.getWidth(); ++x) {
            const auto t = void_castle.get(x, y).getType();
            if (t == "door_closed" || t == "door_open") {
                total_doors++;
                const auto north = (y > 0) ? void_castle.get(x, y - 1).getType() : "void";
                const auto south = (y + 1 < void_castle.getHeight()) ? void_castle.get(x, y + 1).getType() : "void";
                const auto west = (x > 0) ? void_castle.get(x - 1, y).getType() : "void";
                const auto east = (x + 1 < void_castle.getWidth()) ? void_castle.get(x + 1, y).getType() : "void";

                const bool horiz_walls = (west == "wall" && east == "wall");
                const bool vert_walls = (north == "wall" && south == "wall");
                if (!horiz_walls && !vert_walls) {
                    invalid_doors++;
                }
                const auto& door_tile = void_castle.get(x, y);
                if (t == "door_closed") {
                    if (horiz_walls && door_tile.getGlyph() != U'-') {
                        invalid_doors++;
                    }
                    if (vert_walls && door_tile.getGlyph() != U'|') {
                        invalid_doors++;
                    }
                }
            }
        }
    }
    Check(total_doors > 0 && invalid_doors == 0, "all internal doors are framed with directional '-' or '|' glyphs");

    // Test castle with outside entrance door
    RL::CMap entrance_castle(RL::Size(79, 50));
    RL::CreateCastle(entrance_castle, 20, true, true);
    std::size_t outside_doors = 0;
    for (std::size_t y = 0; y < entrance_castle.getHeight(); ++y) {
        for (std::size_t x = 0; x < entrance_castle.getWidth(); ++x) {
            if (entrance_castle.get(x, y).getType() == "door_closed") {
                const auto north = (y > 0) ? entrance_castle.get(x, y - 1).getType() : "void";
                const auto south = (y + 1 < entrance_castle.getHeight()) ? entrance_castle.get(x, y + 1).getType() : "void";
                if ((north == "void" && south != "void") || (south == "void" && north != "void")) {
                    outside_doors++;
                }
            }
        }
    }
    Check(outside_doors >= 1, "outside_door generates entrance gate connecting void to interior");

    // Test castle with outside windows
    RL::CMap window_castle(RL::Size(79, 50));
    RL::CreateCastle(window_castle, 20, true, true, true);
    std::size_t total_windows = 0;
    std::size_t invalid_windows = 0;
    bool symmetric_windows = true;

    for (std::size_t y = 0; y < window_castle.getHeight(); ++y) {
        for (std::size_t x = 0; x < window_castle.getWidth(); ++x) {
            const auto& tile = window_castle.get(x, y);
            if (tile.getType() == "window") {
                total_windows++;

                if (!tile.isTransparent() || tile.isPassable()) {
                    invalid_windows++;
                }

                const auto north = (y > 0) ? window_castle.get(x, y - 1).getType() : "void";
                const auto south = (y + 1 < window_castle.getHeight()) ? window_castle.get(x, y + 1).getType() : "void";
                const auto west = (x > 0) ? window_castle.get(x - 1, y).getType() : "void";
                const auto east = (x + 1 < window_castle.getWidth()) ? window_castle.get(x + 1, y).getType() : "void";

                auto is_int = [](std::string_view t) { return t == "room" || t == "corridor"; };
                auto is_ext = [](std::string_view t) { return t == "void"; };

                const bool horiz_framed = (west == "wall" && east == "wall") &&
                                          ((is_int(north) && is_ext(south)) || (is_int(south) && is_ext(north)));
                const bool vert_framed = (north == "wall" && south == "wall") &&
                                         ((is_int(west) && is_ext(east)) || (is_int(east) && is_ext(west)));

                if (!horiz_framed && !vert_framed) {
                    invalid_windows++;
                }

                if (horiz_framed && tile.getGlyph() != U'-') {
                    invalid_windows++;
                }
                if (vert_framed && tile.getGlyph() != U'|') {
                    invalid_windows++;
                }

                // Check vertical mirror symmetry
                const std::size_t mx = window_castle.getWidth() - 1 - x;
                if (window_castle.get(mx, y).getType() != "window") {
                    symmetric_windows = false;
                }
            }
        }
    }
    Check(total_windows > 0, "castle generates outside windows when requested");
    Check(invalid_windows == 0, "all castle windows are transparent, impassable, and have '-' or '|' glyphs matching wall framing");
    Check(symmetric_windows, "castle windows are vertically symmetric");

    // Test space shuttle with outside windows
    RL::CMap window_shuttle(RL::Size(80, 40));
    RL::CreateSpaceShuttle(window_shuttle, 20, true, false, 0.40f);
    std::size_t shuttle_windows = 0;
    std::size_t invalid_shuttle_windows = 0;
    bool symmetric_shuttle_windows = true;

    for (std::size_t y = 0; y < window_shuttle.getHeight(); ++y) {
        for (std::size_t x = 0; x < window_shuttle.getWidth(); ++x) {
            const auto& tile = window_shuttle.get(x, y);
            if (tile.getType() == "window") {
                shuttle_windows++;

                if (!tile.isTransparent() || tile.isPassable()) {
                    invalid_shuttle_windows++;
                }

                const auto north = (y > 0) ? window_shuttle.get(x, y - 1).getType() : "void";
                const auto south = (y + 1 < window_shuttle.getHeight()) ? window_shuttle.get(x, y + 1).getType() : "void";
                const auto west = (x > 0) ? window_shuttle.get(x - 1, y).getType() : "void";
                const auto east = (x + 1 < window_shuttle.getWidth()) ? window_shuttle.get(x + 1, y).getType() : "void";

                auto is_int = [](std::string_view t) { return t == "room" || t == "corridor"; };
                auto is_ext = [](std::string_view t) { return t == "void"; };

                const bool horiz_framed = (west == "wall" && east == "wall") &&
                                          ((is_int(north) && is_ext(south)) || (is_int(south) && is_ext(north)));
                const bool vert_framed = (north == "wall" && south == "wall") &&
                                         ((is_int(west) && is_ext(east)) || (is_int(east) && is_ext(west)));

                if (!horiz_framed && !vert_framed) {
                    invalid_shuttle_windows++;
                }

                if (horiz_framed && tile.getGlyph() != U'-') {
                    invalid_shuttle_windows++;
                }
                if (vert_framed && tile.getGlyph() != U'|') {
                    invalid_shuttle_windows++;
                }

                // Check horizontal mirror symmetry
                const std::size_t my = window_shuttle.getHeight() - 1 - y;
                if (window_shuttle.get(x, my).getType() != "window") {
                    symmetric_shuttle_windows = false;
                }
            }
        }
    }
    Check(shuttle_windows > 0, "space shuttle generates outside windows when requested");
    Check(invalid_shuttle_windows == 0, "all space shuttle windows are transparent, impassable, and on framed outside walls");
    Check(symmetric_shuttle_windows, "space shuttle windows are horizontally symmetric");

    // Test zero probability produces no windows
    RL::CMap no_window_castle(RL::Size(79, 50));
    RL::CreateCastle(no_window_castle, 20, true, true, 0.0f);
    std::size_t zero_windows = 0;
    for (std::size_t y = 0; y < no_window_castle.getHeight(); ++y) {
        for (std::size_t x = 0; x < no_window_castle.getWidth(); ++x) {
            if (no_window_castle.get(x, y).getType() == "window") {
                zero_windows++;
            }
        }
    }
    Check(zero_windows == 0, "window_probability = 0 produces no windows");
}

void TestDoubleDoors()
{
    // 1. Horizontal double door in a horizontal wall: '#' '#' 'D' 'D' '#' '#'
    RL::CMap h_map(RL::Size(8, 5));
    h_map.Clear("room");
    for (std::size_t x = 0; x < 8; ++x) {
        h_map.SetCell(x, 2, "wall");
    }
    // Place double doors at (3, 2) and (4, 2)
    h_map.SetCell(3, 2, "door_closed");
    h_map.SetCell(4, 2, "door_closed");
    RL::UpdateDoorAndWindowGlyphs(h_map);

    Check(h_map.get(3, 2).getGlyph() == U'-', "horizontal closed double door left has '-' glyph");
    Check(h_map.get(4, 2).getGlyph() == U'-', "horizontal closed double door right has '-' glyph");

    // Open left door
    RL::OpenDoor(h_map, 3, 2);
    Check(h_map.get(3, 2).getType() == "door_open" && h_map.get(3, 2).getGlyph() == U'/',
          "opening left door of horizontal double door assigns '/'");
    Check(h_map.get(4, 2).getType() == "door_closed" && h_map.get(4, 2).getGlyph() == U'-',
          "right door remains closed with '-'");

    // Open right door
    RL::OpenDoor(h_map, 4, 2);
    Check(h_map.get(3, 2).getType() == "door_open" && h_map.get(3, 2).getGlyph() == U'/',
          "opened horizontal double door left has '/' glyph");
    Check(h_map.get(4, 2).getType() == "door_open" && h_map.get(4, 2).getGlyph() == U'\\',
          "opened horizontal double door right has '\\' glyph");

    // Close right door
    RL::CloseDoor(h_map, 4, 2);
    Check(h_map.get(4, 2).getType() == "door_closed" && h_map.get(4, 2).getGlyph() == U'-',
          "closing right door restores 'door_closed' with '-' glyph");
    Check(h_map.get(3, 2).getType() == "door_open" && h_map.get(3, 2).getGlyph() == U'/',
          "left door remains open with '/'");

    // Close left door
    RL::CloseDoor(h_map, 3, 2);
    Check(h_map.get(3, 2).getType() == "door_closed" && h_map.get(3, 2).getGlyph() == U'-',
          "closing left door restores 'door_closed' with '-' glyph");

    // 2. Vertical double door in a vertical wall:
    RL::CMap v_map(RL::Size(5, 8));
    v_map.Clear("room");
    for (std::size_t y = 0; y < 8; ++y) {
        v_map.SetCell(2, y, "wall");
    }
    // Place double doors at (2, 3) and (2, 4)
    v_map.SetCell(2, 3, "door_closed");
    v_map.SetCell(2, 4, "door_closed");
    RL::UpdateDoorAndWindowGlyphs(v_map);

    Check(v_map.get(2, 3).getGlyph() == U'|', "vertical closed double door top has '|' glyph");
    Check(v_map.get(2, 4).getGlyph() == U'|', "vertical closed double door bottom has '|' glyph");

    // Open top door
    RL::OpenDoor(v_map, 2, 3);
    Check(v_map.get(2, 3).getType() == "door_open" && v_map.get(2, 3).getGlyph() == U'\\',
          "opening top door of vertical double door assigns '\\'");
    Check(v_map.get(2, 4).getType() == "door_closed" && v_map.get(2, 4).getGlyph() == U'|',
          "bottom door remains closed with '|'");

    // Open bottom door
    RL::OpenDoor(v_map, 2, 4);
    Check(v_map.get(2, 3).getType() == "door_open" && v_map.get(2, 3).getGlyph() == U'\\',
          "opened vertical double door top has '\\' glyph");
    Check(v_map.get(2, 4).getType() == "door_open" && v_map.get(2, 4).getGlyph() == U'/',
          "opened vertical double door bottom has '/' glyph");

    // Close top door
    RL::CloseDoor(v_map, 2, 3);
    Check(v_map.get(2, 3).getType() == "door_closed" && v_map.get(2, 3).getGlyph() == U'|',
          "closing top door restores 'door_closed' with '|' glyph");
    Check(v_map.get(2, 4).getType() == "door_open" && v_map.get(2, 4).getGlyph() == U'/',
          "bottom door remains open with '/'");

    // 3. Single doors
    RL::CMap s_map(RL::Size(5, 5));
    s_map.Clear("room");
    s_map.SetCell(1, 2, "wall");
    s_map.SetCell(2, 2, "door_closed");
    s_map.SetCell(3, 2, "wall");
    RL::UpdateDoorAndWindowGlyphs(s_map);
    Check(s_map.get(2, 2).getGlyph() == U'-', "single horizontal closed door has '-' glyph");
    RL::OpenDoor(s_map, 2, 2);
    Check(s_map.get(2, 2).getType() == "door_open" && s_map.get(2, 2).getGlyph() == U'/',
          "single horizontal open door has '/' glyph");

    s_map.SetCell(2, 1, "wall");
    s_map.SetCell(2, 2, "door_closed");
    s_map.SetCell(2, 3, "wall");
    s_map.SetCell(1, 2, "room");
    s_map.SetCell(3, 2, "room");
    RL::UpdateDoorAndWindowGlyphs(s_map);
    Check(s_map.get(2, 2).getGlyph() == U'|', "single vertical closed door has '|' glyph");
    RL::OpenDoor(s_map, 2, 2);
    Check(s_map.get(2, 2).getType() == "door_open" && s_map.get(2, 2).getGlyph() == U'/',
          "single vertical open door has '/' glyph");

    // 4. UpdateOpenDoorGlyphs across entire map
    RL::CMap batch_map(RL::Size(10, 10));
    batch_map.Clear("room");
    // Horizontal pair at (3, 3) and (4, 3)
    batch_map.SetCell(2, 3, "wall");
    batch_map.SetCell(3, 3, "door_open");
    batch_map.SetCell(4, 3, "door_open");
    batch_map.SetCell(5, 3, "wall");
    // Vertical pair at (7, 6) and (7, 7)
    batch_map.SetCell(7, 5, "wall");
    batch_map.SetCell(7, 6, "door_open");
    batch_map.SetCell(7, 7, "door_open");
    batch_map.SetCell(7, 8, "wall");

    RL::UpdateOpenDoorGlyphs(batch_map);
    Check(batch_map.get(3, 3).getGlyph() == U'/' && batch_map.get(4, 3).getGlyph() == U'\\',
          "UpdateOpenDoorGlyphs resolves horizontal double door to '/\\'");
    Check(batch_map.get(7, 6).getGlyph() == U'\\' && batch_map.get(7, 7).getGlyph() == U'/',
          "UpdateOpenDoorGlyphs resolves vertical double door to '\\' over '/'");
}

void TestBuildingInterior()
{
    // 1. Small cottage (3x3 inside, area <= 9): full floor, no partitions
    {
        RL::CMap map(RL::Size(15, 15));
        map.Clear("grass");
        RL::SBuilding cottage;
        cottage.corner1 = RL::Position(2, 2);
        cottage.corner2 = RL::Position(6, 6);
        cottage.door = RL::Position(2, 4);

        RL::CreateBuildingInterior(map, cottage);

        Check(map.get(cottage.door).getType() == "door_closed", "cottage entrance door is placed");
        Check(map.get(cottage.door).getGlyph() == U'|', "vertical entrance door has '|' glyph");

        bool walls_intact = true;
        for (std::size_t y = cottage.corner1.y; y <= cottage.corner2.y; ++y) {
            for (std::size_t x = cottage.corner1.x; x <= cottage.corner2.x; ++x) {
                if (x == cottage.corner1.x || x == cottage.corner2.x ||
                    y == cottage.corner1.y || y == cottage.corner2.y) {
                    if (RL::Position(x, y) != cottage.door) {
                        walls_intact &= map.get(x, y).getType() == "wall";
                    }
                } else {
                    walls_intact &= map.get(x, y).getType() == "room";
                }
            }
        }
        Check(walls_intact, "cottage perimeter wall is intact and interior is fully open room");
    }

    // 2. Large building with odd dimensions (15x15 footprint, 13x13 inside)
    {
        RL::InitRandomness(123);
        RL::CMap map(RL::Size(25, 25));
        map.Clear("grass");
        RL::SBuilding building;
        building.corner1 = RL::Position(3, 3);
        building.corner2 = RL::Position(17, 17);
        building.door = RL::Position(10, 3); // top wall

        RL::CreateBuildingInterior(map, building);

        Check(map.get(building.door).getType() == "door_closed", "entrance door placed");
        Check(map.get(building.door).getGlyph() == U'-', "horizontal entrance door has '-' glyph");

        // Verify perimeter walls are intact except at entrance
        bool perimeter_ok = true;
        for (std::size_t y = building.corner1.y; y <= building.corner2.y; ++y) {
            for (std::size_t x = building.corner1.x; x <= building.corner2.x; ++x) {
                if (x == building.corner1.x || x == building.corner2.x ||
                    y == building.corner1.y || y == building.corner2.y) {
                    if (RL::Position(x, y) != building.door) {
                        perimeter_ok &= map.get(x, y).getType() == "wall";
                    }
                }
            }
        }
        Check(perimeter_ok, "large building perimeter walls are completely intact");

        // Count interior rooms, internal walls, and internal doors
        std::size_t room_cells = 0, internal_doors = 0, internal_walls = 0;
        for (std::size_t y = building.corner1.y + 1; y < building.corner2.y; ++y) {
            for (std::size_t x = building.corner1.x + 1; x < building.corner2.x; ++x) {
                const auto type = map.get(x, y).getType();
                if (type == "room") ++room_cells;
                else if (type == "door_closed") ++internal_doors;
                else if (type == "wall") ++internal_walls;
            }
        }
        Check(room_cells > 0 && internal_walls > 0 && internal_doors > 0,
              "large building contains internal walls, rooms, and doors");

        // Connectivity check: starting at entrance, all rooms must be reachable
        std::vector<bool> visited(map.getWidth() * map.getHeight(), false);
        std::vector<RL::Position> queue{building.door};
        visited[building.door.y * map.getWidth() + building.door.x] = true;
        for (std::size_t i = 0; i < queue.size(); ++i) {
            const auto p = queue[i];
            const std::array next{RL::Position(p.x - 1, p.y), RL::Position(p.x + 1, p.y),
                                  RL::Position(p.x, p.y - 1), RL::Position(p.x, p.y + 1)};
            for (auto n : next) {
                if (n.x <= building.corner1.x || n.x >= building.corner2.x ||
                    n.y <= building.corner1.y || n.y >= building.corner2.y) continue;
                if (visited[n.y * map.getWidth() + n.x]) continue;
                const auto t = map.get(n).getType();
                if (t == "room" || t == "door_closed") {
                    visited[n.y * map.getWidth() + n.x] = true;
                    queue.push_back(n);
                }
            }
        }
        bool all_rooms_reachable = true;
        for (std::size_t y = building.corner1.y + 1; y < building.corner2.y; ++y) {
            for (std::size_t x = building.corner1.x + 1; x < building.corner2.x; ++x) {
                if (map.get(x, y).getType() == "room") {
                    all_rooms_reachable &= visited[y * map.getWidth() + x];
                }
            }
        }
        Check(all_rooms_reachable, "all interior rooms are reachable from the entrance door");
    }

    // 3. Large building with even dimensions (16x14 footprint)
    {
        RL::InitRandomness(456);
        RL::CMap map(RL::Size(30, 30));
        map.Clear("grass");
        RL::SBuilding building;
        building.corner1 = RL::Position(5, 5);
        building.corner2 = RL::Position(20, 18); // width 16, height 14
        building.door = RL::Position(5, 10); // west wall

        RL::CreateBuildingInterior(map, building);

        Check(map.get(building.door).getType() == "door_closed", "even dimension entrance door placed");
        Check(map.get(building.door).getGlyph() == U'|', "vertical entrance door has '|' glyph");

        bool perimeter_ok = true;
        for (std::size_t y = building.corner1.y; y <= building.corner2.y; ++y) {
            for (std::size_t x = building.corner1.x; x <= building.corner2.x; ++x) {
                if (x == building.corner1.x || x == building.corner2.x ||
                    y == building.corner1.y || y == building.corner2.y) {
                    if (RL::Position(x, y) != building.door) {
                        perimeter_ok &= map.get(x, y).getType() == "wall";
                    }
                }
            }
        }
        Check(perimeter_ok, "even dimension building outer walls are strictly 1-tile thick");
    }

    // 4. Backward compatibility alias TownInterior
    {
        RL::InitRandomness(789);
        RL::CMap map1(RL::Size(20, 20)), map2(RL::Size(20, 20));
        map1.Clear("grass");
        map2.Clear("grass");
        RL::SBuilding b;
        b.corner1 = RL::Position(2, 2);
        b.corner2 = RL::Position(14, 14);
        b.door = RL::Position(8, 2);

        RL::InitRandomness(999);
        RL::CreateBuildingInterior(map1, b);
        RL::InitRandomness(999);
        RL::TownInterior(map2, b);

        bool identical = true;
        for (std::size_t y = 0; y < 20; ++y) {
            for (std::size_t x = 0; x < 20; ++x) {
                identical &= map1.get(x, y).getType() == map2.get(x, y).getType() &&
                             map1.get(x, y).getGlyph() == map2.get(x, y).getGlyph();
            }
        }
        Check(identical, "TownInterior is an alias producing identical results to CreateBuildingInterior");
    }
}

void TestSimpleCity()
{
    RL::CMap map(RL::Size(70, 70));
    RL::InitRandomness(42);
    RL::CreateSimpleCity(map, 6);

    std::size_t buildings_found = 0;
    std::size_t doors_found = 0;
    std::size_t roads_found = 0;

    for (std::size_t y = 0; y < map.getHeight(); ++y) {
        for (std::size_t x = 0; x < map.getWidth(); ++x) {
            const auto type = map.get(x, y).getType();
            if (type == "door_closed") {
                ++doors_found;
                const auto glyph = map.get(x, y).getGlyph();
                Check(glyph == U'-' || glyph == U'|', "all doors in simple city have directional glyphs");
            } else if (type == "corridor") {
                ++roads_found;
            } else if (type == "room") {
                ++buildings_found;
            }
        }
    }

    Check(buildings_found > 0, "simple city generates building interiors with rooms");
    Check(doors_found >= 6, "simple city places entrance and interior doors");
    Check(roads_found > 0, "simple city generates road networks");
}
