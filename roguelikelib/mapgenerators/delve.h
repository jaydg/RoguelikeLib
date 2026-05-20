/* Written by Kusigrosz in December 2008, April 2011
 * This program is in public domain, with all its bugs etc.
 * No warranty whatsoever.
 *
 * Delve a random maze/cavern.
 *
 * Algorithm description:
 * The algorithm operates on a rectangular grid. Each cell can be
 * LevelElementWall or LevelElementWall. A (non-border) cell has 8
 * neigbours - diagonals count.
 *
 * There is also a cell store with two operations: store a given cell
 * on top, and pull a cell from the store. There is no check for
 * repetitions, so a given cell can be stored multiple times.
 *
 * The cell to be pulled is selected depending on the value of pullflag:
 *     PULL_CUBEROOT: randomly from all the store if N_in_store < 125,
 *         and randomly from the top 25 * cube_root(N_in_store) otherwise.
 *     PULL_ALL: randomly from all the store.
 *     PULL_BOTTOM: always the bottommost cell of the store.
 *
 * The algorithm starts with most of the map filled with WALL, with a
 * "seed" of some FLOOR cells; their neigbouring WALL cells are in
 * store. The main loop in delveon() is repeated until the desired
 * number of FLOOR cells is reached, or there is nothing in store:
 *     Pull a cell from the store;
 *     Encode the WALL/FLOOR pattern of its 8 neighbours in an 8-bit
 *         number, and use it as an index into the digperm table, to
 *         fetch the permil probability of such pattern permitting digging.
 *     With the given probability, convert the cell to FLOOR, and store
 *         its WALL neighbours, depending on the value of the storeflag:
 *         STORE_PERM: in random order
 *         STORE_CW: clockwise (starting from a random one)
 *         STORE_CCW: counterclockwise (starting from a random one)
 *
 * The digperm table entries are 256 values from 0 to 1000; the bits
 * of the indices correspond to the neighbours of the given cell - lsb
 * is the cell to the right, then clockwise; FLOOR is 1 and WALL 0.
 * The values are permil probabilities of the cell being allowed for
 * digging. For example, an entry 750 at index 179 (10110011) means
 * that the pattern:
 *  101
 *  1c1
 *  001
 * will permit digging of the central cell with probability 0.75
 *
 * If the name of the digperm string is not found, the digperm table
 * is generated randomly in randdigperm. As such random generation
 * may result in a table that won't allow digging enough cells,
 * the table generation / cavern generation is repeated until a high
 * enough number of cells is dug.
 *
 */

#pragma once

#include "../map.h"
#include "../randomness.h"
#include "../position.h"

#include <algorithm>
#include <array>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace RL {

namespace delve_detail {

// pullflag values: from which part of cellstore cells are pulled
enum class PullFlag {
    CUBEROOT = 1,
    ALL      = 2,
    BOTTOM   = 3,
    DEFAULT  = CUBEROOT
};

// storeflag values: in what order the neighbour cells are stored
enum class StoreFlag {
    PERM  = 1,
    CW    = 2,
    CCW   = 3,
    DEFAULT = PERM
};

constexpr std::array<int, 8> Xoff = {1, 1, 0, -1, -1, -1, 0, 1};
constexpr std::array<int, 8> Yoff = {0, 1, 1, 1, 0, -1, -1, -1};

/* The Hamming weights (numbers of 1's) of values 0 - 255 */
constexpr std::array<int, 256> Hamming = {
    0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};

/* The number of groups of 1's in the 8 cell neighbourhood of a cell.
 * The lsb corresponds to the cell to the right, then clockwise.
 * A group is a set of cells that are connected (diagonally too).
 * For example:
 * 101                 110              101
 * 1x0 2 groups        1x1 1 group      0x0 4 groups
 * 010                 010              101
 */
constexpr std::array<int, 256> NgbGrouptab = {
    0,1,1,1,1,1,1,1,1,2,2,2,1,1,1,1,
    1,2,2,2,1,1,1,1,1,2,2,2,1,1,1,1,
    1,2,2,2,2,2,2,2,2,3,3,3,2,2,2,2,
    1,2,2,2,1,1,1,1,1,2,2,2,1,1,1,1,
    1,1,2,1,2,1,2,1,2,2,3,2,2,1,2,1,
    1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,
    1,1,2,1,2,1,2,1,2,2,3,2,2,1,2,1,
    1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,
    1,1,2,1,2,1,2,1,2,2,3,2,2,1,2,1,
    2,2,3,2,2,1,2,1,2,2,3,2,2,1,2,1,
    2,2,3,2,3,2,3,2,3,3,4,3,3,2,3,2,
    2,2,3,2,2,1,2,1,2,2,3,2,2,1,2,1,
    1,1,2,1,2,1,2,1,2,2,3,2,2,1,2,1,
    1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,
    1,1,2,1,2,1,2,1,2,2,3,2,2,1,2,1,
    1,1,2,1,1,1,1,1,1,1,2,1,1,1,1,1
};

struct digperm_config {
    std::string_view desc;
    StoreFlag store_flag = StoreFlag::DEFAULT;
    PullFlag pull_flag = PullFlag::DEFAULT;
    int ngb_min = 0;
    int ngb_max = 0;
    int conmil  = 0;
};

const std::unordered_map<std::string_view, digperm_config> DigpermPresets = {
    // rect maze with loops
    {
        "rmaze1", {
            "1:1000 3:1000 6:1000 17:1000 14:0 19:1000 25:1000 27:1000 29:0 "
            "51:1000 57:1000 102:1000 59:1000 107:0 110:1000 187:1000 127:0 255:0"
        }
    },
    // dense rect maze / cavern with lots of pillars (griddy - town)
    {
        "rmaze2", {
            "1:1000 17:1000 14:1000 19:1000 25:1000 27:1000 29:0 51:1000 "
            "57:1000 102:1000 59:1000 107:0 110:1000 187:1000 127:0 255:0"
        }
    },
    // dense rect maze / cavern, long multipillars (no pillars?
    {
        "rmaze3", { "1:1000 17:1000 14:1000 30:0 57:1000 106:0 122:0 255:0" }
    },
    // dense rect maze with 2x2 rooms at ends, may die young
    {
        "rmaze4", { "1:1000 17:1000 7:1000 14:1000 30:0 57:1000 106:0 122:0 255:0" }
    },
    // together with pulling always the 0-th cell, a rect town wall builder
    {
        "rmaze5", { "1:1000 6:1000 17:1000 19:1000 25:1000 27:1000 51:1000 57:1000 102:1000" }
    },
    // sparse rect town grid, few pillars, may die young
    {
        "rmaze6", {
            "1:1000 4:1000 14:50 16:1000 17:1000 19:1000 25:1000 27:1000 "
            "49:1000 1:1000 56:50 57:1000 64:1000 68:1000 70:1000 76:1000 78:1000 "
            "100:1000 102:1000 108:1000 131:50 145:1000 147:1000 153:1000 177:1000 "
            "196:1000 198:1000 204:1000 224:50 228:1000"
        }
    },
    // mostly rect maze - mostly caverns actually
    {
        "rmaze7", {
            "1:1000 3:1000 6:1000 5:1000 9:1000 10:1000 17:1000 18:1000 "
            "34:1000 7:1000 15:1000 23:1000 29:1000 30:1000 31:1000 55:1000 "
            "61:1000 62:1000 87:1000 94:1000 103:1000 63:1000 95:1000 111:1000 "
            "119:1000 123:1000 126:1000 127:1000 191:1000 255:1000"
        }
    },
    // another rect maze
    {
        "rmaze8", {
            "1:1000 3:200 6:200 17:1000 14:200 19:1000 21:1000 25:1000 "
            "27:200 51:200 53:1000 54:1000 57:1000 85:1000 86:1000 102:200 "
        }
    },
    // rect maze, call with STORE_CW or CCW and PULL_BOTTOM
    {
        "rmaze9", {
            "1:1000 3:1000 5:1000 7:200 14:200 18:1000 19:200 38:1000 94:200 ",
            StoreFlag::CW, PullFlag::BOTTOM
        }
    },
    // narrow diag maze with occasional straight connection
    {
        "dmaze1", { "2:1000 17:1000" }
    },
    // mostly diag narrow mes
    {
        "dmaze2", { "1:100 2:1000 9:1000 10:1000 17:1000 18:1000 34:1000" }
    },
    // mostly diag maze with some straight corridor
    {
        "dmaze3", { "2:1000 9:1000 10:1000 17:1000 18:1000 21:1000 34:1000" }
    },
    // narrow diag maze with wider cavern
    {
        "dmaze4", {
            "2:1000 10:1000 21:1000 85:1000 170:1000 31:1000 47:1000 "
            "55:1000 59:1000 61:1000 62:1000 87:1000 91:1000 94:1000 103:1000 "
            "107:1000 110:1000 122:1000 171:1000 63:1000 95:1000 111:1000 119:1000 "
            "123:1000 126:1000 175:1000 187:1000 127:1000 191:1000 255:1000"}
    },
    // mixed narrow maze, try with PULL_BOTTOM
    {
        "dmaze5", {
            "2:1000 5:1000 37:200 53:200 58:1000 86:200 95:1000 122:200 171:1000",
            StoreFlag::DEFAULT, PullFlag::BOTTOM
        }
    },
    // old type cavern ngb_min = 1, ngb_max = 1, connchance = 0 permi
    {
        "old11_0", {
            "1:1000 2:1000",
            StoreFlag::DEFAULT, PullFlag::DEFAULT,
            1, 1, 0

        }
    },
    // old type cavern ngb_min = 1, ngb_max = 2, connchance = 0 permil
    {
        "old12_0", {
            "1:1000 2:1000 3:1000 5:1000 6:1000",
            StoreFlag::DEFAULT, PullFlag::DEFAULT,
            1, 2, 0
        }
    },
    // old type cavern ngb_min = 2, ngb_max = 3, connchance = 0 permil
    {
        "old23_0", {
            "3:1000 5:1000 6:1000 7:1000 13:1000 14:1000 21:1000 22:1000",
            StoreFlag::DEFAULT, PullFlag::DEFAULT,
            2, 3, 0
        }
    },
    // old type cavern ngb_min = 2, ngb_max = 3, connchance = 50 permi
    {
        "old23_50", {
            "3:1000 5:1000 6:1000 7:1000 9:50 10:50 11:50 13:1000 "
            "14:1000 15:1000 17:50 18:50 19:50 21:1000 22:1000 23:1000 "
            "25:50 26:50 27:50 29:1000 30:1000 34:50 35:50 37:50 "
            "38:50 39:50 41:50 42:50 43:50 45:50 46:50 51:50 "
            "53:1000 54:1000 57:50 58:50 85:1000 86:1000 90:50 102:50 106:50 170:50 ",
            StoreFlag::DEFAULT, PullFlag::DEFAULT,
            2, 3, 50
        }
    },
    // old type cavern ngb_min = 2, ngb_max = 4, connchance = 200 permi
    {
        "old24_200", {
            "3:1000 5:1000 6:1000 7:1000 9:200 10:200 11:200 13:1000 "
            "14:1000 15:1000 17:200 18:200 19:200 21:1000 22:1000 23:1000 "
            "25:200 26:200 27:200 29:1000 30:1000 34:200 35:200 37:200 "
            "38:200 39:200 41:200 42:200 43:200 45:200 46:200 51:200 "
            "53:1000 54:1000 57:200 58:200 85:1000 86:1000 90:200 102:200 106:200 170:200 ",
            StoreFlag::DEFAULT, PullFlag::DEFAULT,
            2, 4, 200
        }
    },
    // old type cavern ngb_min = 3, ngb_max = 4, connchance = 0 permi
    {
        "old34_0", {
            "7:1000 13:1000 14:1000 15:1000 21:1000 22:1000 23:1000 "
            "29:1000 30:1000 53:1000 54:1000 85:1000 86:1000 ",
            StoreFlag::DEFAULT, PullFlag::DEFAULT,
            3, 4, 0
        }
    },
    // old type cavern ngb_min = 3, ngb_max = 4, connchance = 10 permi
    {
        "old34_10", {
            "7:1000 11:10 13:1000 14:1000 15:1000 19:10 21:1000 22:1000 "
            "23:1000 25:10 26:10 27:10 29:1000 30:1000 35:10 37:10 38:10 39:10 "
            "41:10 42:10 43:10 45:10 46:10 51:10 53:1000 54:1000 57:10 58:10 "
            "85:1000 86:1000 90:10 102:10 106:10 170:10 ",
            StoreFlag::DEFAULT, PullFlag::DEFAULT,
            3, 4, 10
        }
    },
    // old type cavern ngb_min = 3, ngb_max = 5, connchance = 0 permil
    {
        "old35_0", {
            "7:1000 13:1000 14:1000 15:1000 21:1000 22:1000 23:1000 "
            "29:1000 30:1000 31:1000 53:1000 54:1000 55:1000 61:1000 62:1000 "
            "85:1000 86:1000 87:1000 91:1000 94:1000 103:1000",
            StoreFlag::DEFAULT, PullFlag::DEFAULT,
            3, 5, 0
        }
    },
    // old type cavern ngb_min = 3, ngb_max = 8, connchance = 0 permi
    {
        "old38_0", {
            "7:1000 13:1000 14:1000 15:1000 21:1000 22:1000 23:1000 "
            "29:1000 30:1000 31:1000 53:1000 54:1000 55:1000 61:1000 62:1000 "
            "63:1000 85:1000 86:1000 87:1000 91:1000 94:1000 95:1000 103:1000 "
            "111:1000 119:1000 123:1000 126:1000 127:1000 191:1000 255:1000",
            StoreFlag::DEFAULT, PullFlag::DEFAULT,
            3, 8, 0
        }
    },
    // old type cavern ngb_min = 3, ngb_max = 8, connchance = 2 permil
    {
        "old38_2", {
            "7:1000 11:2 13:1000 14:1000 15:1000 19:2 21:1000 22:1000 "
            "23:1000 25:2 26:2 27:2 29:1000 30:1000 31:1000 35:2 37:2 38:2 39:2 "
            "41:2 42:2 43:2 45:2 46:2 47:2 51:2 53:1000 54:1000 55:1000 57:2 58:2 "
            "59:2 61:1000 62:1000 63:1000 85:1000 86:1000 87:1000 90:2 91:1000 "
            "94:1000 95:1000 102:2 103:1000 106:2 107:2 110:2 111:1000 119:1000 "
            "122:2 123:1000 126:1000 127:1000 170:2 171:2 175:2 187:2 191:1000 255:1000",
            StoreFlag::DEFAULT, PullFlag::DEFAULT,
            3, 8, 2
        }
    }
};

/* A table of neighbour patterns and their digperm indices is
 * presented below; '@' represents FLOOR and '-' WALL. A usable
 * digperm string like those above needs to contain only entries
 * with nonzero probabilities - others can be omitted.
 *
 * When creating a digperm table, the probabilities in entries
 * corresponding to the given pattern rotated by 90, 180 and 270
 * degrees are set (by setsymmetr()) to the same value - otherwise
 * the generated cavern would 'lean' in one direction.

#---   ---   ---   ---   ---   ---   ---   ---   ---   ---   @--
#-c-   -c@   -c-   -c@   -c@   -c-   -c@   -c-   @c@   @c-   -c-
#---   ---   --@   --@   -@-   -@@   @--   @-@   ---   --@   --@
0:0   1:0   2:0   3:0   5:0   6:0   9:0   10:0  17:0  18:0  34:0

#---   ---   ---   ---   ---   ---   ---   ---   ---   @--   @--
#-c@   -c@   -c@   -c-   @c@   @c@   @c-   @c@   @c-   -c@   -c@
#-@@   @-@   @@-   @@@   --@   -@-   -@@   @--   @-@   --@   -@-
7:0   11:0  13:0  14:0  19:0  21:0  22:0  25:0  26:0  35:0  37:0

#@--   @--   @--   ---   ---   ---   ---   ---   @--   @--   @--
#-c-   -c@   -c-   -c@   @c@   @c@   @c@   @c-   -c@   -c@   -c@
#-@@   @--   @-@   @@@   -@@   @-@   @@-   @@@   -@@   @-@   @@-
38:0  41:0  42:0  15:0  23:0  27:0  29:0  30:0  39:0  43:0  45:0

#@--   @--   @--   @--   @--   @--   -@-   -@-   -@-   @@-   @@-
#-c-   @c@   @c@   @c-   @c@   @c-   @c@   @c-   @c-   -c-   -c-
#@@@   --@   -@-   -@@   @--   @-@   -@-   -@@   @-@   -@@   @-@
46:0  51:0  53:0  54:0  57:0  58:0  85:0  86:0  90:0  102:0 106:0

#@-@   ---   @--   @--   @--   @--   @--   -@-   -@-   -@-   @@-
#-c-   @c@   -c@   @c@   @c@   @c@   @c-   @c@   @c@   @c-   -c@
#@-@   @@@   @@@   -@@   @-@   @@-   @@@   -@@   @-@   @@@   -@@
170:0 31:0  47:0  55:0  59:0  61:0  62:0  87:0  91:0  94:0  103:0

#@@-   @@-   @@-   @-@   @--   -@-   @@-   @@-   @@-   @@-   @-@
#-c@   -c-   @c-   -c@   @c@   @c@   -c@   @c@   @c@   @c-   -c@
#@-@   @@@   @-@   @-@   @@@   @@@   @@@   -@@   @-@   @@@   @@@
107:0 110:0 122:0 171:0 63:0  95:0  111:0 119:0 123:0 126:0 175:0

#@-@   @@-   @-@   @@@
#@c@   @c@   @c@   @c@
#@-@   @@@   @@@   @@@
187:0 127:0 191:0 255:0
 */

// Rotate left an 8-bit value.
inline int Rotl8(int d, int n)
{
    return (((d >> (8 - n)) & 0xff) | (((d << n) & 0xff)));
}

inline int ICbrt(int arg)
{
    if (arg <= 0) return 0;
    if (arg > 0x7fffffff) arg = 0x7fffffff;
    int ret;
    if (arg < 1000) ret = 10;
    else if (arg < 1000000) ret = 100;
    else if (arg < 1000000000) ret = 1000;
    else ret = 1290;

    int delta;
    do {
        delta = (arg - ret * ret * ret) / (2 * ret * ret);
        ret += delta;
    } while (delta);

    if (ret * ret * ret > arg) ret--;
    return ret;
}

inline bool InBord(size_t xsize, size_t ysize, int x, int y)
{
    // Evaluates with int cast to avoid unsigned underflow issues if x or y hit negative bounds
    return (x >= 1 && x < static_cast<int>(xsize) - 1 &&
            y >= 1 && y < static_cast<int>(ysize) - 1);
}

/* Set the digperm entry at idx to prob, and fix the symmetry by
 * copying the value to entries at idx rotated by 2, 4, 6 bits - this
 * corresponds to pattern rotation by 90, 180, 270 degrees - to prevent
 * direction bias.
 */
inline void SetSymmetr(std::array<int, 256>& digperm, int idx, int prob)
{
    digperm[idx] = prob;
    for (int i = 2; i <= 6; i += 2) {
        digperm[Rotl8(idx, i)] = prob;
    }
}

/* Fill the digperm array with permil probabilities. If desc is non-null,
 * the entries in format idx:prob are scanned from the desc string,
 * otherwise they are generated depending on ngb_min, ngb_max and conmil
 * in the following way: If the number of 1s (floor neighbours) in the
 * index is less than ngb_min or more than ngb_max, the entry stays 0.
 * Otherwise, if digging the cell c wouldn't connect previously
 * unconnected groups of floor cells, the entry is set to 1000. Otherwise,
 * the entry is set to conmil.
 */
inline void FillDigperm(std::array<int, 256>& digperm, int ngb_min, int ngb_max, int conmil, std::string_view desc)
{
    digperm.fill(0);

    if (!desc.empty()) {
        std::istringstream iss{std::string(desc)};
        std::string token;

        while (iss >> token) {
            auto colon_pos = token.find(':');
            if (colon_pos != std::string::npos) {
                try {
                    int idx = std::stoi(token.substr(0, colon_pos));
                    int prob = std::stoi(token.substr(colon_pos + 1));
                    if (idx > 0 && idx < 256 && prob >= 0 && prob <= 1000) {
                        SetSymmetr(digperm, idx, prob);
                    }
                } catch (...) {
                    // Suppress parsing exceptions
                }
            }
        }
    } else {
        // generate from params
        for (size_t i = 0; i < 256; i++) {
            if (Hamming[i] >= ngb_min && Hamming[i] <= ngb_max) {
                digperm[i] = (NgbGrouptab[i] == 1) ? 1000 : conmil;
            }
        }
    }
}

/* Select a digperm index of Hamming weight no higher than maxham.
 * Prefer those with a higher Hamming weight - among equals chose
 * with probability proportional to the value of the digperm entry.
 * If no positive probabilities found, return -1;
 */
inline int FindSeed(const std::array<int, 256>& digperm, int maxham)
{
    int ret = -1;
    int sumprob = 0;
    int maxtillnow = 0;

    for (size_t i = 0; i < 256; i++) {
        if (Hamming[i] <= maxham && digperm[i] > 0) {
            if (Hamming[i] > maxtillnow) {
                sumprob = digperm[i];
                maxtillnow = Hamming[i];
            } else if (Hamming[i] == maxtillnow) {
                sumprob += digperm[i];
            } else {
                continue;
            }
            if (sumprob > 0 && static_cast<int>(Random(sumprob)) < digperm[i]) {
                ret = static_cast<int>(i);
            }
        }
    }
    return ret;
}

/* A crude way of generating a random digperm table. Will often give
 * interesting results, but certainly doesn't explore a significant part
 * of the possibilities.
 */
inline void RandDigperm(std::array<int, 256>& digperm)
{
    digperm.fill(0);

    /* These are indices of patterns that are the main ways of pattern
     * expansion. If the probabilities at all these entries are too
     * low, the pattern will very likely die young. */
    constexpr std::array<int, 5> expand = {1, 2, 3, 6, 14};

    /* Make sure the pattern has at least some possibility of expanding */
    SetSymmetr(digperm, expand[Random(expand.size())], 1000);

    /* usually a few, sometimes a few dozen entries with 1000 permil prob */
    int c1000 = static_cast<int>(Random(5)) + ((Random(4) == 0) ? static_cast<int>(Random(30)) : 4);
    for (int i = 0; i < c1000; i++) {
        SetSymmetr(digperm, static_cast<int>(Random(256)), 1000);
    }

    /* usually a few, sometimes a few dozen entries with 200 permil prob */
    int c200 = static_cast<int>(Random(5)) + ((Random(4) == 0) ? static_cast<int>(Random(30)) : 4);
    for (int i = 0; i < c200; i++) {
        SetSymmetr(digperm, static_cast<int>(Random(256)), 200);
    }
}

struct CellStore {
    std::vector<Position> cells;
    size_t index = 0;

    void Init(size_t size)
    {
        cells.resize(size);
        index = 0;
    }

    bool Store(size_t x, size_t y)
    {
        if (index < cells.size()) {
            cells[index] = Position(x, y);
            index++;
            return true;
        } else {
            size_t rind = Random(index);
            cells[rind] = Position(x, y);
            return false;
        }
    }

    bool Pull(int& x, int& y, PullFlag pullflag)
    {
        if (index <= 0) return false;

        size_t rind;
        switch (pullflag) {
            case PullFlag::CUBEROOT:
                // fluffy patterns
                rind = (index < 125) ? Random(index) :
                    (index - Random(25 * ICbrt(static_cast<int>(index))) - 1);
                break;
            case PullFlag::ALL:
                // compact patterns
                rind = Random(index);
                break;
            case PullFlag::BOTTOM:
                // usually winding patterns
                rind = 0;
                break;
            default:
                return false;
        }

        x = static_cast<int>(cells[rind].x);
        y = static_cast<int>(cells[rind].y);

        if (index - 1 != rind) {
            cells[rind] = cells[index - 1];
        }
        index--;
        return true;
    }
};

/* Is digging the cell at x, y permitted by digperm? Randomized,
 * uses the values in digperm as permil probability.
 */
inline bool IsPermitted(const CMap& level, const std::array<int, 256>& digperm, size_t xsize, size_t ysize, int x, int y, int flo)
{
    // lowest bit is the cell to the right, then clockwisw
    int bitmap = 0;

    for (int i = 0; i < 8; i++) {
        bitmap >>= 1;
        int nx = x + Xoff[i];
        int ny = y + Yoff[i];
        if (InBord(xsize, ysize, nx, ny) && level.GetCell(static_cast<size_t>(nx), static_cast<size_t>(ny)) == flo) {
            bitmap |= 0x80;
        }
    }
    return static_cast<int>(Random(1000)) < digperm[bitmap];
}

/* Dig out an ava cell to flo and store its ava neighbours in
 * the order depending on storeflag.
 */
inline int DigCell(CMap& level, CellStore& cstore, size_t xsize, size_t ysize, int x, int y, StoreFlag storeflag, int flo, int ava)
{
    if (!InBord(xsize, ysize, x, y) || level.GetCell(static_cast<size_t>(x), static_cast<size_t>(y)) != ava) {
        return 0;
    }

    level.SetCell(static_cast<size_t>(x), static_cast<size_t>(y), flo);

    std::array<int, 8> order = {0, 1, 2, 3, 4, 5, 6, 7};

    switch (storeflag) {
        case StoreFlag::PERM:
            RL::Shuffle(order.begin(), order.end());
            break;
        case StoreFlag::CW: {
            int r = static_cast<int>(Random(8));
            for (int i = 0; i < 8; i++) order[i] = (r + i) % 8;
            break;
        }
        case StoreFlag::CCW: {
            int r = static_cast<int>(Random(8));
            for (int i = 0; i < 8; i++) order[i] = (8 + r - i) % 8;
            break;
        }
        default:
            return 0;
    }

    for (int i = 0; i < 8; i++) {
        int j = order[i];
        int nx = x + Xoff[j];
        int ny = y + Yoff[j];

        if (InBord(xsize, ysize, nx, ny) && level.GetCell(static_cast<size_t>(nx), static_cast<size_t>(ny)) == ava) {
            cstore.Store(static_cast<size_t>(nx), static_cast<size_t>(ny));
        }
    }
    return 1;
}

// Continue digging until cellnum or no more cells in store.
inline int DelveOn(CMap& level, CellStore& cstore, size_t xsize, size_t ysize,
    const std::array<int, 256>& digperm, int cellnum, PullFlag pullflag,
    StoreFlag storeflag, int flo, int ava)
{
    int count = 0;
    int x, y;
    while (count < cellnum && cstore.Pull(x, y, pullflag)) {
        if (IsPermitted(level, digperm, xsize, ysize, x, y, flo)) {
            count += DigCell(level, cstore, xsize, ysize, x, y, storeflag, flo, ava);
        }
    }
    return count;
}

/* Generate a random cavern of cellnum cells.
 * Old type caverns can be generated by calling first:
 * FillDigperm(&digperm, ngb_min, ngb_max, conmil, NULL);
 * then:
 * Cavern(level, xorig, yorig, digperm, cellnum,
 *     PullFlag::CUBEROOT, StoreFlag::PERM, FLOOR, WALL);
 */
inline int Cavern(CMap& level, int xorig, int yorig, const std::array<int, 256>& digperm,
    int cellnum, PullFlag pullflag, StoreFlag storeflag, int flo, int ava)
{
    size_t xsize = level.GetWidth();
    size_t ysize = level.GetHeight();

    int seed = FindSeed(digperm, cellnum);
    if (seed < 0) return 0;

    CellStore cstore;
    cstore.Init(8 * xsize * ysize);

    int count = 0;
    count += DigCell(level, cstore, xsize, ysize, xorig, yorig, storeflag, flo, ava);

    for (int i = 0; i < 8; i++) {
        int x = xorig + Xoff[i];
        int y = yorig + Yoff[i];
        if (count < cellnum && ((seed >> i) & 0x1)) {
            count += DigCell(level, cstore, xsize, ysize, x, y, storeflag, flo, ava);
        }
    }

    if (count < cellnum) {
        count += DelveOn(level, cstore, xsize, ysize, digperm, cellnum - count, pullflag, storeflag, flo, ava);
    }
    return count;
}

} // namespace delve_detail

inline std::vector<std::string_view> GetDelvePresets() {
    std::vector<std::string_view> keys;
    for (const auto& [name, _] : delve_detail::DigpermPresets) {
        keys.push_back(name);
    }

    return keys;
}

inline
void CreateDelve(CMap &level, std::string_view digperm_name = {}, int cellnum = 0,
    delve_detail::PullFlag pullflag = delve_detail::PullFlag::DEFAULT,
    delve_detail::StoreFlag storeflag = delve_detail::StoreFlag::DEFAULT)
{
    if (level.GetWidth() < 3 || level.GetHeight() < 3) {
        return;
    }

    size_t xsize = level.GetWidth();
    size_t ysize = level.GetHeight();

    if (cellnum <= 0) {
        cellnum = static_cast<int>(xsize * ysize) / 5;
    }

    level.Clear(LevelElementWall);

    std::array<int, 256> digperm{};
    int dugcells = 0;
    bool use_random = true;

    if (!digperm_name.empty()) {
        auto it = delve_detail::DigpermPresets.find(digperm_name);
        if (it != delve_detail::DigpermPresets.end()) {
            use_random = false;
            const delve_detail::digperm_config& config = it->second;
            delve_detail::FillDigperm(digperm, config.ngb_min, config.ngb_max, config.conmil, config.desc);
            dugcells = delve_detail::Cavern(level, static_cast<int>(xsize) / 2, static_cast<int>(ysize) / 2, digperm,
                cellnum,
                pullflag == delve_detail::PullFlag::DEFAULT ? config.pull_flag : pullflag,
                storeflag == delve_detail::StoreFlag::DEFAULT ? config.store_flag : storeflag,
                LevelElementCorridor, LevelElementWall);
        }
    }

    if (use_random) {
        do {
            delve_detail::RandDigperm(digperm);
            level.Clear(LevelElementWall);
            dugcells = delve_detail::Cavern(level, static_cast<int>(xsize) / 2, static_cast<int>(ysize) / 2, digperm,
                cellnum, pullflag, storeflag, LevelElementCorridor, LevelElementWall);
        } while (dugcells < cellnum && dugcells < static_cast<int>(xsize * ysize) / 3);
    }
}

} // namespace RL
