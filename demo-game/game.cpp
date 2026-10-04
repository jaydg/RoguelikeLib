module;

export module demo_game.game: impl;

import demo_game.game;
import demo_game.io;
import demo_game.monster;
import demo_game.player;
import demo_game.rodent;
import rl.maputils;
import rl.mapgenerators;
import rl.randomness;
import std;

void CSimpleGame::PlacePlayer()
{
    RL::Position pos(0, 0);

    if (RL::FindOnMapRandomRectangleOfType(level, "room", pos, RL::Size(1, 1))) {
        player.MoveTo(pos);
    } else if (RL::FindOnMapRandomRectangleOfType(level, "corridor", pos, RL::Size(1, 1))) {
        player.MoveTo(pos);
    }

    monsters.push_back(&player);
    scheduler.Schedule(&player, 0);
}

void CSimpleGame::AddMonster()
{
    auto *new_one = new CRodent;
    RL::Position pos(0, 0);

    if (RL::FindOnMapRandomRectangleOfType(level, "room", pos, RL::Size(1, 1))) {
        new_one->MoveTo(pos);
    } else if (RL::FindOnMapRandomRectangleOfType(level, "corridor", pos, RL::Size(1, 1))) {
        new_one->MoveTo(pos);
    }

    monsters.push_back(new_one);
    scheduler.Schedule(new_one, new_one->Delay());
}

void CSimpleGame::RemoveDeadMonsters()
{
    for (CMonster *monster : monsters_to_remove) {
        scheduler.Remove(monster);
        monsters.remove(monster);
        delete monster;
    }

    monsters_to_remove.clear();
}

CMonster* CSimpleGame::GetMonsterFromCell(const RL::Position& cell)
{
    std::list <CMonster*>::iterator it, _it;

    for (it = monsters.begin(), _it = monsters.end(); it != _it; ++it) {
        CMonster *monster = *it;

        if (monster->GetPosition() == cell) {
            return monster;
        }
    }

    return nullptr;
}

void CSimpleGame::CreateLevel()
{
    int level_type = RL::Random(6);

    switch (level_type) {
    case 0:
        RL::CreateStandardDungeon(level, 20, false);
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
    default:
        RL::CreateSpaceShuttle(level);
        IOPrintString(60, 24, "Space Shuttle");
        break;
    }

    PlacePlayer();

    for (int index = 0; index < 15; ++index) {
        AddMonster();
    }
}

[[noreturn]] void CSimpleGame::MainLoop()
{
    for (;;) {
        // The player is always on the timeline, so someone always is next
        CMonster *actor = *scheduler.Next();

        // Once a turn of the player
        if (actor == &player) {
            if (RL::Random(100) == 0) {
                AddMonster();
            }

            player.Regenerate();
        }

        actor->DoAction();

        // Monsters killed in the action leave the game; whoever acted is
        // back on the timeline for its next action
        const bool alive = !monsters_to_remove.contains(actor);
        RemoveDeadMonsters();

        if (alive) {
            scheduler.Schedule(actor, actor->Delay());
        }
    }
}
