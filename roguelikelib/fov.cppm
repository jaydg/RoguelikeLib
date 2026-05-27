// Adam Milazzo's Field Of Vision
// (http://www.adammil.net/blog/v125_Roguelike_Vision_Algorithms.html)

module;

export module rl.fov;

import rl.map;
import rl.matrix;
import rl.position;
import std;

export namespace RL {

class CFOV : public CMatrix<bool> {
private:
    const CMap *map;

    // Represents the slope Y/X as a rational number for the shadowcasting bounds
    struct Slope {
        unsigned int X, Y;
        Slope(unsigned int y, unsigned int x) : X(x), Y(y) {}

        [[nodiscard]] bool Greater(unsigned int y, unsigned int x) const { return Y * x > X * y; }
        [[nodiscard]] bool GreaterOrEqual(unsigned int y, unsigned int x) const { return Y * x >= X * y; }
        [[nodiscard]] bool Less(unsigned int y, unsigned int x) const { return Y * x < X * y; }
    };

    // Calculate Euclidean distance from origin point (0,0) mapped to (x,y)
    [[nodiscard]] inline int GetDistance(int x, int y) const noexcept {
        return static_cast<int>(std::round(std::sqrt(x * x + y * y)));
    }

    // Helper to determine if a map cell blocks the line of sight based on its value.
    [[nodiscard]] inline bool grid_is_blocked(const RL::CMap *map, int x, int y)
    {
        int cell = map->GetCell(static_cast<std::size_t>(x), static_cast<std::size_t>(y));
        return cell == RL::LevelElementWall_value ||
               cell == RL::LevelElementWall ||
               cell == RL::LevelElementDoorClose;
    }

    [[nodiscard]] bool BlocksLight(unsigned int x, unsigned int y, unsigned int octant, Position origin) {
        int nx = static_cast<int>(origin.x);
        int ny = static_cast<int>(origin.y);

        switch(octant) {
            case 0: nx += x; ny -= y; break;
            case 1: nx += y; ny -= x; break;
            case 2: nx -= y; ny -= x; break;
            case 3: nx -= x; ny -= y; break;
            case 4: nx -= x; ny += y; break;
            case 5: nx -= y; ny += x; break;
            case 6: nx += y; ny += x; break;
            case 7: nx += x; ny += y; break;
        }

        if (nx < 0 || ny < 0 || nx >= static_cast<int>(map->GetWidth()) || ny >= static_cast<int>(map->GetHeight())) {
            return true;
        }

        return grid_is_blocked(map, nx, ny);
    }

    void SetVisible(unsigned int x, unsigned int y, unsigned int octant, Position origin) {
        int nx = static_cast<int>(origin.x);
        int ny = static_cast<int>(origin.y);

        switch(octant) {
            case 0: nx += x; ny -= y; break;
            case 1: nx += y; ny -= x; break;
            case 2: nx -= y; ny -= x; break;
            case 3: nx -= x; ny -= y; break;
            case 4: nx -= x; ny += y; break;
            case 5: nx -= y; ny += x; break;
            case 6: nx += y; ny += x; break;
            case 7: nx += x; ny += y; break;
        }

        if (nx >= 0 && ny >= 0 && nx < static_cast<int>(getWidth()) && ny < static_cast<int>(getHeight())) {
            set(static_cast<std::size_t>(nx), static_cast<std::size_t>(ny), true);
        }
    }

    // Adam Milazzo's recursive shadowcasting logic
    //
    // throughout this function there are references to various parts of tiles. a tile's coordinates refer to its
    // center, and the following diagram shows the parts of the tile and the vectors from the origin that pass through
    // those parts. given a part of a tile with vector u, a vector v passes above it if v > u and below it if v < u
    //    g         center:        y / x
    // a------b   a top left:      (y*2+1) / (x*2-1)   i inner top left:      (y*4+1) / (x*4-1)
    // |  /\  |   b top right:     (y*2+1) / (x*2+1)   j inner top right:     (y*4+1) / (x*4+1)
    // |i/__\j|   c bottom left:   (y*2-1) / (x*2-1)   k inner bottom left:   (y*4-1) / (x*4-1)
    //e|/|  |\|f  d bottom right:  (y*2-1) / (x*2+1)   m inner bottom right:  (y*4-1) / (x*4+1)
    // |\|__|/|   e middle left:   (y*2) / (x*2-1)
    // |k\  /m|   f middle right:  (y*2) / (x*2+1)     a-d are the corners of the tile
    // |  \/  |   g top center:    (y*2+1) / (x*2)     e-h are the corners of the inner (wall) diamond
    // c------d   h bottom center: (y*2-1) / (x*2)     i-m are the corners of the inner square (1/2 tile width)
    //    h
    void ComputeOctant(unsigned int octant, Position origin, int rangeLimit, unsigned int x, Slope top, Slope bottom)
    {
        for(; rangeLimit < 0 || x <= static_cast<unsigned int>(rangeLimit); x++)
        {
            // compute the Y coordinates of the top and bottom of the sector. we maintain that top > bottom
            unsigned int topY;
            if (top.X == 1) // top == 1/1
            {
                topY = x;
            }
            else
            {
                topY = ((x * 2 - 1) * top.Y + top.X) / (top.X * 2);

                if (BlocksLight(x, topY, octant, origin)) // if the tile blocks light (i.e. is a wall)...
                {
                    if (top.GreaterOrEqual(topY * 2 + 1, x * 2) && !BlocksLight(x, topY + 1, octant, origin))
                        topY++;
                }
                else // the tile doesn't block light
                {
                    unsigned int ax = x * 2;
                    // use bottom-right if the tile above and right is a wall
                    if (BlocksLight(x + 1, topY + 1, octant, origin)) ax++;
                    if (top.Greater(topY * 2 + 1, ax)) topY++;
                }
            }

            unsigned int bottomY;
            if (bottom.Y == 0) // bottom == 0/?
            {
                bottomY = 0;
            }
            else
            {
                bottomY = ((x * 2 - 1) * bottom.Y + bottom.X) / (bottom.X * 2);

                if (bottom.GreaterOrEqual(bottomY * 2 + 1, x * 2) &&
                    BlocksLight(x, bottomY, octant, origin) &&
                   !BlocksLight(x, bottomY + 1, octant, origin))
                {
                    bottomY++;
                }
            }

            int wasOpaque = -1; // 0:false, 1:true, -1:not applicable
            for (unsigned int y = topY; static_cast<int>(y) >= static_cast<int>(bottomY); y--)
            {
                if (rangeLimit < 0 || GetDistance(static_cast<int>(x), static_cast<int>(y)) <= rangeLimit)
                {
                    bool isOpaque = BlocksLight(x, y, octant, origin);
                    bool isVisible = isOpaque ||
                        ((y != topY || top.Greater(y * 4 - 1, x * 4 + 1)) &&
                         (y != bottomY || bottom.Less(y * 4 + 1, x * 4 - 1)));

                    if (isVisible) SetVisible(x, y, octant, origin);

                    // if we found a transition from clear to opaque or vice versa, adjust the top and bottom vectors
                    if (rangeLimit < 0 || x != static_cast<unsigned int>(rangeLimit))
                    {
                        if (isOpaque)
                        {
                            if (wasOpaque == 0) // clear to opaque transition
                            {
                                unsigned int nx = x * 2, ny = y * 2 + 1; // top center by default
                                if (BlocksLight(x, y + 1, octant, origin)) nx--; // top left if the corner is not beveled

                                if (top.Greater(ny, nx))
                                {
                                    if (y == bottomY) { bottom = Slope(ny, nx); break; }
                                    else ComputeOctant(octant, origin, rangeLimit, x + 1, top, Slope(ny, nx));
                                }
                                else // sector is empty
                                {
                                    if (y == bottomY) return;
                                }
                            }
                            wasOpaque = 1;
                        }
                        else // is clear
                        {
                            if (wasOpaque > 0) // opaque to clear transition
                            {
                                unsigned int nx = x * 2, ny = y * 2 + 1;
                                if (BlocksLight(x + 1, y + 1, octant, origin)) nx++;

                                if (bottom.GreaterOrEqual(ny, nx)) return;
                                top = Slope(ny, nx);
                            }
                            wasOpaque = 0;
                        }
                    }
                }
            }

            // if the column didn't end in a clear tile, there's no reason to continue processing the current sector
            if (wasOpaque != 0) break;
        }
    }

public:
    CFOV() : CMatrix<bool>(Size(0,0), false), map(nullptr) {}
    explicit CFOV(const CMap *map) : CMatrix<bool>(map->getSize(), false), map(map) {}

    // Triggers a recalculation of the Field of View using Adam Milazzo's Shadowcasting
    void Calculate(Position start, int radius)
    {
        // Reset the matrix (clear old visibility)
        for (std::size_t x = 0; x < getWidth(); ++x) {
            for (std::size_t y = 0; y < getHeight(); ++y) {
                set(x, y, false);
            }
        }

        int cx = static_cast<int>(start.x);
        int cy = static_cast<int>(start.y);

        if (!map->OnMap(cx, cy)) return;

        // The starting position is always visible
        set(start.x, start.y, true);

        // Calculate visibility for all 8 octants
        for (unsigned int octant = 0; octant < 8; octant++)
        {
            ComputeOctant(octant, start, radius, 1, Slope(1, 1), Slope(0, 1));
        }
    }
};

} // namespace RL
