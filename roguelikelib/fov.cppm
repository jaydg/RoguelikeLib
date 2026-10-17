/**
 * @file fov.cppm
 * @brief Field of view (FOV) calculation using Adam Milazzo's recursive shadowcasting.
 *
 * Implements the recursive shadowcasting algorithm described by Adam Milazzo in
 * <a href="http://www.adammil.net/blog/v125_Roguelike_Vision_Algorithms.html">Roguelike Vision Algorithms</a>.
 */

module;

export module rl.fov;

import rl.map;
import rl.matrix;
import rl.position;
import rl.tile;
import std;

export namespace RL
{

/**
 * @brief Predicate callback determining whether light passes through coordinates `(x, y)`.
 *
 * Used by @ref CFOV when evaluating visibility without a direct @ref CMap instance.
 *
 * @param x Horizontal column coordinate.
 * @param y Vertical row coordinate.
 * @return True if the cell at `(x, y)` is transparent (lets light pass through).
 */
using Transparency = std::function<bool(std::size_t x, std::size_t y)>;

/**
 * @brief 2D boolean visibility matrix computed via Adam Milazzo's recursive shadowcasting.
 *
 * Inherits from @ref CMatrix<bool>, where each cell represents whether a tile is currently
 * visible (`true`) or obscured in darkness/shadow (`false`).
 *
 * Based on the algorithm by Adam Milazzo
 * (<a href="http://www.adammil.net/blog/v125_Roguelike_Vision_Algorithms.html">Roguelike Vision Algorithms</a>).
 *
 * Key characteristics:
 * - **Symmetric Line-of-Sight:** Guarantees reciprocal visibility between clear tiles (if A sees B, B sees A).
 * - **Sub-tile Precision:** Uses exact rational slopes (@f$\frac{Y}{X}@f$) without floating-point rounding errors,
 *   treating tiles with inner wall diamonds and corner bevels to avoid diagonal beam leaks or corner artifacts.
 * - **Circular Euclidean Horizon:** Uses Euclidean squared distance (@f$x^2 + y^2 \le r^2@f$) to bound
 *   visibility into a natural circle, or unbounded vision across the whole grid when passed a negative radius.
 * - **Flexible Transparency Providers:** Can sample directly from a @ref CMap (via @ref CTile::isTransparent)
 *   or an arbitrary @ref Transparency predicate for multi-layered levels or custom vision-blocking entities.
 *
 * @see CMatrix
 * @see CMap
 * @see Position
 */
class CFOV : public CMatrix<bool>
{
private:
    const CMap* map = nullptr;
    Transparency transparent;

    /**
     * @brief Represents ray slope @f$\frac{Y}{X}@f$ as an exact rational number.
     *
     * Used for shadowcasting sector boundaries to avoid floating-point drift and rounding errors.
     */
    struct Slope {
        unsigned int X; ///< Run (horizontal delta component in octant space).
        unsigned int Y; ///< Rise (vertical delta component in octant space).

        /**
         * @brief Constructs a slope with given rise @p y and run @p x.
         */
        Slope(unsigned int y, unsigned int x) : X(x), Y(y) {}

        /**
         * @brief Tests if this slope is strictly greater than @f$\frac{y}{x}@f$.
         */
        [[nodiscard]] bool Greater(unsigned int y, unsigned int x) const
        {
            return Y * x > X * y;
        }

        /**
         * @brief Tests if this slope is greater than or equal to @f$\frac{y}{x}@f$.
         */
        [[nodiscard]] bool GreaterOrEqual(unsigned int y, unsigned int x) const
        {
            return Y * x >= X * y;
        }

        /**
         * @brief Tests if this slope is strictly less than @f$\frac{y}{x}@f$.
         */
        [[nodiscard]] bool Less(unsigned int y, unsigned int x) const
        {
            return Y * x < X * y;
        }
    };

    /**
     * @brief Computes squared Euclidean distance @f$x^2 + y^2@f$ from local origin.
     * @param x Horizontal offset.
     * @param y Vertical offset.
     * @return Squared distance value.
     */
    [[nodiscard]] inline int GetDistanceSquared(int x, int y) const noexcept
    {
        return x * x + y * y;
    }

    /**
     * @brief Coordinate multipliers for transforming octant-relative coordinates to grid space.
     */
    struct OctantOffset {
        std::int8_t xx, xy, yx, yy;
    };

    /**
     * @brief Transformation lookup table for the 8 octants.
     */
    static constexpr std::array<OctantOffset, 8> octantOffsets = {{
        { 1,  0,  0, -1}, // 0: nx += x; ny -= y;
        { 0,  1, -1,  0}, // 1: nx += y; ny -= x;
        { 0, -1, -1,  0}, // 2: nx -= y; ny -= x;
        {-1,  0,  0, -1}, // 3: nx -= x; ny -= y;
        {-1,  0,  0,  1}, // 4: nx -= x; ny += y;
        { 0, -1,  1,  0}, // 5: nx -= y; ny += x;
        { 0,  1,  1,  0}, // 6: nx += y; ny += x;
        { 1,  0,  0,  1}  // 7: nx += x; ny += y;
    }};

    /**
     * @brief Transforms octant-relative coordinates `(x, y)` to absolute grid coordinates.
     *
     * @param x Octant horizontal distance.
     * @param y Octant vertical distance.
     * @param octant Octant index in range [0, 7].
     * @param origin Vantage point in grid space.
     * @return Pair of absolute grid coordinates `(nx, ny)`.
     */
    [[nodiscard]] static inline std::pair<int, int> TransformOctant(
        unsigned int x, unsigned int y, unsigned int octant, const Position& origin) noexcept
    {
        const auto& [xx, xy, yx, yy] = octantOffsets[octant];
        const int ix = static_cast<int>(x);
        const int iy = static_cast<int>(y);
        return {
            static_cast<int>(origin.x) + ix * xx + iy * xy,
            static_cast<int>(origin.y) + ix * yx + iy * yy
        };
    }

    /**
     * @brief Checks if the cell at octant-relative `(x, y)` blocks light.
     *
     * Out-of-bounds coordinates are considered opaque (blocking light).
     *
     * @param x Octant horizontal distance.
     * @param y Octant vertical distance.
     * @param octant Octant index in range [0, 7].
     * @param origin Vantage point in grid space.
     * @return True if light is blocked.
     */
    [[nodiscard]] bool BlocksLight(unsigned int x, unsigned int y, unsigned int octant, const Position& origin)
    {
        const auto [nx, ny] = TransformOctant(x, y, octant, origin);

        if (nx < 0 || ny < 0 || nx >= static_cast<int>(getWidth()) || ny >= static_cast<int>(getHeight())) {
            return true;
        }

        if (map != nullptr) {
            return !map->get_unchecked(static_cast<std::size_t>(nx), static_cast<std::size_t>(ny)).isTransparent();
        }

        return !transparent(static_cast<std::size_t>(nx), static_cast<std::size_t>(ny));
    }

    /**
     * @brief Marks cell at octant-relative `(x, y)` as visible in the matrix.
     *
     * Silently ignores coordinates outside the matrix bounds.
     *
     * @param x Octant horizontal distance.
     * @param y Octant vertical distance.
     * @param octant Octant index in range [0, 7].
     * @param origin Vantage point in grid space.
     */
    void SetVisible(unsigned int x, unsigned int y, unsigned int octant, const Position& origin)
    {
        const auto [nx, ny] = TransformOctant(x, y, octant, origin);

        if (nx >= 0 && ny >= 0 && nx < static_cast<int>(getWidth()) && ny < static_cast<int>(getHeight())) {
            set_unchecked(static_cast<std::size_t>(nx), static_cast<std::size_t>(ny), true);
        }
    }

    // *INDENT-OFF*
    //
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
    //
    // *INDENT-ON*
    /**
     * @brief Recursively casts shadows within a single octant.
     *
     * Traverses columns from `x` outwards, maintaining upper (`top`) and lower (`bottom`)
     * slope bounds for unobstructed light cones. When opaque obstacles are encountered,
     * the cone splits or narrows recursively.
     *
     * @param octant Octant index in range [0, 7].
     * @param origin Center vantage point.
     * @param rangeLimit Maximum radius in cells (< 0 for unlimited).
     * @param rangeLimitSq Precomputed squared range limit (@f$r^2@f$).
     * @param x Current column distance within the octant.
     * @param top Upper slope limit of the light cone.
     * @param bottom Lower slope limit of the light cone.
     */
    void ComputeOctant(unsigned int octant, const Position& origin, int rangeLimit, int rangeLimitSq, unsigned int x, Slope top, Slope bottom)
    {
        for (; rangeLimit < 0 || x <= static_cast<unsigned int>(rangeLimit); x++) {
            // compute the Y coordinates of the top and bottom of the sector. we maintain that top > bottom
            unsigned int topY;

            if (top.X == 1) { // top == 1/1
                topY = x;
            } else {
                topY = ((x * 2 - 1) * top.Y + top.X) / (top.X * 2);

                if (BlocksLight(x, topY, octant, origin)) { // if the tile blocks light (i.e. is a wall)...
                    if (top.GreaterOrEqual(topY * 2 + 1, x * 2) && !BlocksLight(x, topY + 1, octant, origin)) {
                        topY++;
                    }
                } else { // the tile doesn't block light
                    unsigned int ax = x * 2;

                    // use bottom-right if the tile above and right is a wall
                    if (BlocksLight(x + 1, topY + 1, octant, origin)) {
                        ax++;
                    }

                    if (top.Greater(topY * 2 + 1, ax)) {
                        topY++;
                    }
                }
            }

            unsigned int bottomY;

            if (bottom.Y == 0) { // bottom == 0/?
                bottomY = 0;
            } else {
                bottomY = ((x * 2 - 1) * bottom.Y + bottom.X) / (bottom.X * 2);

                if (bottom.GreaterOrEqual(bottomY * 2 + 1, x * 2) &&
                        BlocksLight(x, bottomY, octant, origin) &&
                        !BlocksLight(x, bottomY + 1, octant, origin)) {
                    bottomY++;
                }
            }

            int wasOpaque = -1; // 0:false, 1:true, -1:not applicable

            for (unsigned int y = topY; static_cast<int>(y) >= static_cast<int>(bottomY); y--) {
                if (rangeLimit < 0 || GetDistanceSquared(static_cast<int>(x), static_cast<int>(y)) <= rangeLimitSq) {
                    bool isOpaque = BlocksLight(x, y, octant, origin);
                    bool isVisible = isOpaque ||
                                     ((y != topY || top.Greater(y * 4 - 1, x * 4 + 1)) &&
                                      (y != bottomY || bottom.Less(y * 4 + 1, x * 4 - 1)));

                    if (isVisible) {
                        SetVisible(x, y, octant, origin);
                    }

                    // if we found a transition from clear to opaque or vice versa, adjust the top and bottom vectors
                    if (rangeLimit < 0 || x != static_cast<unsigned int>(rangeLimit)) {
                        if (isOpaque) {
                            if (wasOpaque == 0) { // clear to opaque transition
                                unsigned int nx = x * 2, ny = y * 2 + 1; // top center by default

                                if (BlocksLight(x, y + 1, octant, origin)) {
                                    nx--;    // top left if the corner is not beveled
                                }

                                if (top.Greater(ny, nx)) {
                                    if (y == bottomY) {
                                        bottom = Slope(ny, nx);
                                        break;
                                    } else {
                                        ComputeOctant(octant, origin, rangeLimit, rangeLimitSq, x + 1, top, Slope(ny, nx));
                                    }
                                } else { // sector is empty
                                    if (y == bottomY) {
                                        return;
                                    }
                                }
                            }

                            wasOpaque = 1;
                        } else { // is clear
                            if (wasOpaque > 0) { // opaque to clear transition
                                unsigned int nx = x * 2, ny = y * 2 + 1;

                                if (BlocksLight(x + 1, y + 1, octant, origin)) {
                                    nx++;
                                }

                                if (bottom.GreaterOrEqual(ny, nx)) {
                                    return;
                                }

                                top = Slope(ny, nx);
                            }

                            wasOpaque = 0;
                        }
                    }
                }
            }

            // if the column didn't end in a clear tile, there's no reason to continue processing the current sector
            if (wasOpaque != 0) {
                break;
            }
        }
    }

public:
    /**
     * @brief Constructs an empty field of view matrix of size 0x0.
     */
    CFOV() : CMatrix<bool>(Size(0, 0), false) {}

    /**
     * @brief Constructs a field of view grid matching the dimensions of a map.
     *
     * Samples transparency directly from the map's tiles using @ref CTile::isTransparent().
     *
     * @param a_map Pointer to the map whose tile grid is used for line-of-sight checks.
     */
    explicit CFOV(const CMap *a_map)
        : CMatrix<bool>(a_map->getSize(), false), map(a_map)
    {
    }

    /**
     * @brief Constructs a field of view grid with custom dimensions and a transparency predicate.
     *
     * Useful when visibility depends on custom rules, multiple level layers, dynamic obstacles,
     * or non-map data structures. The cells queried by the predicate will always lie within
     * the specified size.
     *
     * @param size Grid dimensions (width and height).
     * @param a_transparent Callback taking `(x, y)` coordinates and returning `true` if transparent.
     */
    CFOV(Size size, Transparency a_transparent) : CMatrix<bool>(size, false), transparent(std::move(a_transparent)) {}

    /**
     * @brief Calculates visibility outwards from a vantage point up to a given radius.
     *
     * Clears previous visibility results (resetting all cells to `false`), marks the
     * origin as visible, and casts recursive shadow cones through all 8 octants.
     *
     * If @p start is outside the matrix boundaries, the method returns immediately
     * without modifying visibility.
     *
     * @param start Center vantage point coordinates.
     * @param radius Maximum view distance in cells. If negative, visibility is unbounded
     *               (limited only by the grid boundaries and line-of-sight obstacles).
     */
    void Calculate(const Position& start, int radius)
    {
        // Reset the matrix (clear old visibility)
        fill(false);

        int cx = static_cast<int>(start.x);
        int cy = static_cast<int>(start.y);

        if (!inside(cx, cy)) {
            return;
        }

        // The starting position is always visible
        set(start.x, start.y, true);

        // Precompute squared range limit to avoid sqrt in the inner loop
        int rangeLimitSq = radius * radius;

        // Calculate visibility for all 8 octants
        for (unsigned int octant = 0; octant < 8; octant++) {
            ComputeOctant(octant, start, radius, rangeLimitSq, 1, Slope(1, 1), Slope(0, 1));
        }
    }
};

} // namespace RL
