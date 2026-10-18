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
}

