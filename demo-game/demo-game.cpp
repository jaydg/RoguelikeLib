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
 * To compile you need the notcurses library,
 * https://github.com/dankamongmen/notcurses/
 *
 */

import demo_game.game;
import demo_game.io;
import rl.randomness;

int main()
{
    IOInit();
    RL::InitRandomness();
    game.CreateLevel();
    game.MainLoop();
}
