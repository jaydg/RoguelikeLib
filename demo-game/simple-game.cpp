/*!
 * Simple game using RoguelikeLib
 * This example shows all the main features of the RoguelikeLib in a simple real game
 * - Map management and generation
 * - Randomness
 * - FOV calculation
 * - Path finding
 *
 * This game can be a base for your roguelike.
 * The source code is very unoptimized, however "Premature optimization is the root of all evil".
 *
 * To compile you need a PDCurses library (for standard project placed in pdcurses subdirectory)
 * http://pdcurses.sourceforge.net
 */

#include <list>
#include <set>
#include "../roguelikelib/randomness.h"
#include "../roguelikelib/mapgenerators.h"

#include "simple-game.h"
#include "io.h"
#include "rodent.h"

// game is a global singleton
CSimpleGame game;

void CSimpleGame::PlacePlayer()
{
    RL::Position pos(0, 0);

    if(RL::FindOnMapRandomRectangleOfType(level, RL::LevelElementRoom, pos, RL::Size(1, 1))) {
        player.MoveTo(pos);
    } else if(RL::FindOnMapRandomRectangleOfType(level, RL::LevelElementCorridor, pos, RL::Size(1, 1))) {
        player.MoveTo(pos);
    }

    monsters.push_back(&player);
}

void CSimpleGame::AddMonster()
{
    CRodent *new_one = new CRodent;
    RL::Position pos(0, 0);

    if(RL::FindOnMapRandomRectangleOfType(level, RL::LevelElementRoom, pos, RL::Size(1, 1))) {
        new_one->MoveTo(pos);
    } else if(RL::FindOnMapRandomRectangleOfType(level, RL::LevelElementCorridor, pos, RL::Size(1, 1))) {
        new_one->MoveTo(pos);
    }

    monsters.push_back(new_one);
}

void CSimpleGame::MoveAllMonsters()
{
    std::list < CMonster * >::iterator it, _it;

    for(it = monsters.begin(), _it = monsters.end(); it != _it; ++it) {
        CMonster *monster = *it;

        if(monsters_to_remove.find(monster) == monsters_to_remove.end()) {
            monster->DoAction();
        }
    }

    // remove all dead monsters
    std::list < CMonster * >::iterator to_remove;

    for(it = monsters.begin(), _it = monsters.end(); it != _it;) {
        to_remove = it;
        it++;
        CMonster *monster = *to_remove;

        if(monsters_to_remove.find(monster) != monsters_to_remove.end()) {
            delete monster;
            monsters.erase(to_remove);
        }
    }

    monsters_to_remove.clear();
}

void CSimpleGame::CreateLevel()
{
    level.Resize(LEVEL_SIZE_X, LEVEL_SIZE_Y);

    int level_type = RL::Random(6);

    switch(level_type) {
    case 0:
        RL::CreateStandardDunegon(level, 20, false);
        IOPrintString(60, 24, "Standard Dungeon");
        break;

    case 1:
        RL::CreateAntNest(level, true);
        IOPrintString(60, 24, "Ant Nest");
        break;

    case 2:
        RL::CreateMines(level, 12);
        IOPrintString(60, 24, "Mines");
        break;

    case 3:
        RL::CreateCaves(level, 2);
        IOPrintString(60, 24, "Caves");
        break;

    case 4:
        RL::CreateMaze(level, false);
        IOPrintString(60, 24, "Maze");
        break;

    case 5:
        RL::CreateSpaceShuttle(level);
        IOPrintString(60, 24, "Space Shuttle");
        break;
    }

    PlacePlayer();

    for(int index = 0; index < 15; ++index) {
        AddMonster();
    }
}

void CSimpleGame::MainLoop()
{
    for(;;) { // next turn
        if(RL::Random(100) == 0) {
            AddMonster();
        }

        player.Regenerate();
        MoveAllMonsters();
    }
}

CMonster *CSimpleGame::GetMonsterFromCell(const RL::Position &cell)
{
    std::list < CMonster * >::iterator it, _it;

    for(it = game.monsters.begin(), _it = game.monsters.end(); it != _it; ++it) {
        CMonster *monster = *it;

        if(monster->GetPosition() == cell) {
            return monster;
        }
    }

    return nullptr;
}

int main(void)
{
    IOInit();
    RL::InitRandomness();
    game.CreateLevel();
    game.MainLoop();
    return 0;
}
