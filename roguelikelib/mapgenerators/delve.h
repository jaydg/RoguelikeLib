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
#ifndef RL_DELVE_H
#define RL_DELVE_H

#include "../map.h"
#include "../randomness.h"

#include <cstdio>
#include <cctype>
#include <vector>

namespace RL {

namespace delve_detail {

static const int Xoff[8] = {1, 1, 0, -1, -1, -1, 0, 1};
static const int Yoff[8] = {0, 1, 1, 1, 0, -1, -1, -1};

static const int Hamming[256] = {
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

static const int NgbGrouptab[256] = {
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

static const char* DigpermStrings[] = {
    "rmaze1 1:1000 3:1000 6:1000 17:1000 14:0 19:1000 25:1000 27:1000 29:0 "
    "51:1000 57:1000 102:1000 59:1000 107:0 110:1000 187:1000 127:0 255:0",
    "rmaze2 1:1000 17:1000 14:1000 19:1000 25:1000 27:1000 29:0 51:1000 "
    "57:1000 102:1000 59:1000 107:0 110:1000 187:1000 127:0 255:0",
    "rmaze3 1:1000 17:1000 14:1000 30:0 57:1000 106:0 122:0 255:0",
    "rmaze4 1:1000 17:1000 7:1000 14:1000 30:0 57:1000 106:0 122:0 255:0",
    "rmaze5 1:1000 6:1000 17:1000 19:1000 25:1000 27:1000 51:1000 57:1000 102:1000",
    "rmaze6 1:1000 4:1000 14:50 16:1000 17:1000 19:1000 25:1000 27:1000 "
    "49:1000 1:1000 56:50 57:1000 64:1000 68:1000 70:1000 76:1000 78:1000 "
    "100:1000 102:1000 108:1000 131:50 145:1000 147:1000 153:1000 177:1000 "
    "196:1000 198:1000 204:1000 224:50 228:1000",
    "rmaze7 1:1000 3:1000 6:1000 5:1000 9:1000 10:1000 17:1000 18:1000 "
    "34:1000 7:1000 15:1000 23:1000 29:1000 30:1000 31:1000 55:1000 "
    "61:1000 62:1000 87:1000 94:1000 103:1000 63:1000 95:1000 111:1000 "
    "119:1000 123:1000 126:1000 127:1000 191:1000 255:1000",
    "rmaze8 1:1000 3:200 6:200 17:1000 14:200 19:1000 21:1000 25:1000 "
    "27:200 51:200 53:1000 54:1000 57:1000 85:1000 86:1000 102:200 ",
    "rmaze9 1:1000 3:1000 5:1000 7:200 14:200 18:1000 19:200 38:1000 94:200 ",
    "dmaze1 2:1000 17:1000",
    "dmaze2 1:100 2:1000 9:1000 10:1000 17:1000 18:1000 34:1000",
    "dmaze3 2:1000 9:1000 10:1000 17:1000 18:1000 21:1000 34:1000",
    "dmaze4 2:1000 10:1000 21:1000 85:1000 170:1000 31:1000 47:1000 "
    "55:1000 59:1000 61:1000 62:1000 87:1000 91:1000 94:1000 103:1000 "
    "107:1000 110:1000 122:1000 171:1000 63:1000 95:1000 111:1000 119:1000 "
    "123:1000 126:1000 175:1000 187:1000 127:1000 191:1000 255:1000",
    "dmaze5 2:1000 5:1000 37:200 53:200 58:1000 86:200 95:1000 122:200 171:1000",
    "old11_0 1:1000 2:1000",
    "old12_0 1:1000 2:1000 3:1000 5:1000 6:1000",
    "old23_0 3:1000 5:1000 6:1000 7:1000 13:1000 14:1000 21:1000 22:1000",
    "old23_50 3:1000 5:1000 6:1000 7:1000 9:50 10:50 11:50 13:1000 "
    "14:1000 15:1000 17:50 18:50 19:50 21:1000 22:1000 23:1000 "
    "25:50 26:50 27:50 29:1000 30:1000 34:50 35:50 37:50 "
    "38:50 39:50 41:50 42:50 43:50 45:50 46:50 51:50 "
    "53:1000 54:1000 57:50 58:50 85:1000 86:1000 90:50 102:50 106:50 170:50 ",
    "old24_200 3:1000 5:1000 6:1000 7:1000 9:200 10:200 11:200 13:1000 "
    "14:1000 15:1000 17:200 18:200 19:200 21:1000 22:1000 23:1000 "
    "25:200 26:200 27:200 29:1000 30:1000 34:200 35:200 37:200 "
    "38:200 39:200 41:200 42:200 43:200 45:200 46:200 51:200 "
    "53:1000 54:1000 57:200 58:200 85:1000 86:1000 90:200 102:200 106:200 170:200 ",
    "old34_0 7:1000 13:1000 14:1000 15:1000 21:1000 22:1000 23:1000 "
    "29:1000 30:1000 53:1000 54:1000 85:1000 86:1000 ",
    "old34_10 7:1000 11:10 13:1000 14:1000 15:1000 19:10 21:1000 22:1000 "
    "23:1000 25:10 26:10 27:10 29:1000 30:1000 35:10 37:10 38:10 39:10 "
    "41:10 42:10 43:10 45:10 46:10 51:10 53:1000 54:1000 57:10 58:10 "
    "85:1000 86:1000 90:10 102:10 106:10 170:10 ",
    "old35_0 7:1000 13:1000 14:1000 15:1000 21:1000 22:1000 23:1000 "
    "29:1000 30:1000 31:1000 53:1000 54:1000 55:1000 61:1000 62:1000 "
    "85:1000 86:1000 87:1000 91:1000 94:1000 103:1000",
    "old38_0 7:1000 13:1000 14:1000 15:1000 21:1000 22:1000 23:1000 "
    "29:1000 30:1000 31:1000 53:1000 54:1000 55:1000 61:1000 62:1000 "
    "63:1000 85:1000 86:1000 87:1000 91:1000 94:1000 95:1000 103:1000 "
    "111:1000 119:1000 123:1000 126:1000 127:1000 191:1000 255:1000",
    "old38_2 7:1000 11:2 13:1000 14:1000 15:1000 19:2 21:1000 22:1000 "
    "23:1000 25:2 26:2 27:2 29:1000 30:1000 31:1000 35:2 37:2 38:2 39:2 "
    "41:2 42:2 43:2 45:2 46:2 47:2 51:2 53:1000 54:1000 55:1000 57:2 58:2 "
    "59:2 61:1000 62:1000 63:1000 85:1000 86:1000 87:1000 90:2 91:1000 "
    "94:1000 95:1000 102:2 103:1000 106:2 107:2 110:2 111:1000 119:1000 "
    "122:2 123:1000 126:1000 127:1000 170:2 171:2 175:2 187:2 191:1000 255:1000",
    nullptr
};

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

inline bool InBord(int xsize, int ysize, int x, int y)
{
    return (x >= 1 && x < xsize - 1 && y >= 1 && y < ysize - 1);
}

inline void RndPerm(int* tab, int nelem)
{
    for (int i = 0; i < nelem; i++) {
        int rind = static_cast<int>(Random(i + 1));
        int tmp = tab[rind];
        tab[rind] = tab[i];
        tab[i] = tmp;
    }
}

inline void SetSymmetr(int* digperm, int idx, int prob)
{
    digperm[idx] = prob;
    for (int i = 2; i <= 6; i += 2) {
        digperm[Rotl8(idx, i)] = prob;
    }
}

inline void FillDigperm(int* digperm, int ngb_min, int ngb_max, int conmil, const char* desc)
{
    for (int i = 0; i < 256; i++) {
        digperm[i] = 0;
    }
    if (desc) {
        for (int i = 0; desc[i]; i++) {
            if (isdigit(static_cast<unsigned char>(desc[i])) &&
                (i == 0 || isspace(static_cast<unsigned char>(desc[i - 1])))) {
                int idx, prob;
                if (std::sscanf(&(desc[i]), "%d:%d", &idx, &prob) == 2 &&
                    idx > 0 && idx < 256 && prob >= 0 && prob <= 1000) {
                    SetSymmetr(digperm, idx, prob);
                }
            }
        }
    } else {
        for (int i = 0; i < 256; i++) {
            if (Hamming[i] >= ngb_min && Hamming[i] <= ngb_max) {
                digperm[i] = (NgbGrouptab[i] == 1) ? 1000 : conmil;
            }
        }
    }
}

inline int FindSeed(const int* digperm, int maxham)
{
    int ret = -1;
    int sumprob = 0;
    int maxtillnow = 0;
    for (int i = 0; i < 256; i++) {
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
                ret = i;
            }
        }
    }
    return ret;
}

inline const char* FindDesc(const char* name)
{
    for (int i = 0; DigpermStrings[i]; i++) {
        int off;
        for (off = 0; name[off] && DigpermStrings[i][off]; off++) {
            if (name[off] != DigpermStrings[i][off]) break;
        }
        if (name[off] == 0 && isspace(static_cast<unsigned char>(DigpermStrings[i][off]))) {
            return DigpermStrings[i];
        }
    }
    return nullptr;
}

inline void RandDigperm(int* digperm)
{
    for (int i = 0; i < 256; i++) digperm[i] = 0;
    int expand[] = {1, 2, 3, 6, 14};
    SetSymmetr(digperm, expand[Random(sizeof(expand) / sizeof(expand[0]))], 1000);
    int c1000 = static_cast<int>(Random(5)) + ((Random(4) == 0) ? static_cast<int>(Random(30)) : 4);
    for (int i = 0; i < c1000; i++) {
        SetSymmetr(digperm, static_cast<int>(Random(256)), 1000);
    }
    int c200 = static_cast<int>(Random(5)) + ((Random(4) == 0) ? static_cast<int>(Random(30)) : 4);
    for (int i = 0; i < c200; i++) {
        SetSymmetr(digperm, static_cast<int>(Random(256)), 200);
    }
}

struct CellStore {
    std::vector<int> xs;
    std::vector<int> ys;
    int index;

    void Init(int size)
    {
        xs.resize(size);
        ys.resize(size);
        index = 0;
    }

    bool Store(int x, int y)
    {
        if (index < static_cast<int>(xs.size())) {
            xs[index] = x;
            ys[index] = y;
            index++;
            return true;
        } else {
            int rind = static_cast<int>(Random(index));
            xs[rind] = x;
            ys[rind] = y;
            return false;
        }
    }

    bool Pull(int& x, int& y, int pullflag)
    {
        if (index <= 0) return false;
        int rind;
        switch (pullflag) {
            case 1:
                rind = (index < 125) ? static_cast<int>(Random(index)) :
                    (index - static_cast<int>(Random(25 * ICbrt(index))) - 1);
                break;
            case 2:
                rind = static_cast<int>(Random(index));
                break;
            case 3:
                rind = 0;
                break;
            default:
                return false;
        }
        x = xs[rind];
        y = ys[rind];
        if (index - 1 != rind) {
            xs[rind] = xs[index - 1];
            ys[rind] = ys[index - 1];
        }
        index--;
        return true;
    }
};

inline bool IsPermitted(const CMap& level, const int* digperm, int xsize, int ysize, int x, int y, int flo)
{
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

inline int DigCell(CMap& level, CellStore& cstore, int xsize, int ysize, int x, int y, int storeflag, int flo, int ava)
{
    int order[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    if (!InBord(xsize, ysize, x, y) || level.GetCell(static_cast<size_t>(x), static_cast<size_t>(y)) != ava) {
        return 0;
    }
    level.SetCell(static_cast<size_t>(x), static_cast<size_t>(y), flo);
    switch (storeflag) {
        case 1:
            RndPerm(order, 8);
            break;
        case 2: {
            int r = static_cast<int>(Random(8));
            for (int i = 0; i < 8; i++) order[i] = (r + i) % 8;
            break;
        }
        case 3: {
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
            cstore.Store(nx, ny);
        }
    }
    return 1;
}

inline int DelveOn(CMap& level, CellStore& cstore, int xsize, int ysize, const int* digperm, int cellnum, int pullflag, int storeflag, int flo, int ava)
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

inline int Cavern(CMap& level, int xorig, int yorig, const int* digperm, int cellnum, int pullflag, int storeflag, int flo, int ava)
{
    int xsize = static_cast<int>(level.GetWidth());
    int ysize = static_cast<int>(level.GetHeight());
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

inline
void CreateDelve(CMap &level, const char* digperm_name = nullptr, int cellnum = 0, int pullflag = 1, int storeflag = 1)
{
    if(level.GetWidth() < 3 || level.GetHeight() < 3) {
        return;
    }

    int xsize = static_cast<int>(level.GetWidth());
    int ysize = static_cast<int>(level.GetHeight());

    if(cellnum <= 0) {
        cellnum = xsize * ysize / 5;
    }

    level.Clear(LevelElementWall);

    int digperm[256] = {0};
    int dugcells = 0;
    bool use_random = true;

    if(digperm_name && *digperm_name) {
        const char* desc = delve_detail::FindDesc(digperm_name);
        if(desc) {
            use_random = false;
            delve_detail::FillDigperm(digperm, 0, 0, 0, desc);
            dugcells = delve_detail::Cavern(level, xsize / 2, ysize / 2, digperm,
                cellnum, pullflag, storeflag, LevelElementCorridor, LevelElementWall);
        }
    }

    if(use_random) {
        do {
            delve_detail::RandDigperm(digperm);
            level.Clear(LevelElementWall);
            dugcells = delve_detail::Cavern(level, xsize / 2, ysize / 2, digperm,
                cellnum, pullflag, storeflag, LevelElementCorridor, LevelElementWall);
        } while(dugcells < cellnum && dugcells < xsize * ysize / 3);
    }
}

} // namespace RL

#endif
