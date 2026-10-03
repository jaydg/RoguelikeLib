//////////////////////////////////////////////////////////////////////////
// Locations
//
// The places of significance a map generator laid out - the clearings of a
// forest, for instance. A generator hands them back with the map, so that
// whatever builds on top of it next - placing a village, ruins or a camp -
// knows where they are instead of having to find them in the tiles again.
//////////////////////////////////////////////////////////////////////////

module;

export module rl.locations;

export import rl.position;
import std;

export namespace RL
{

enum class ELocationType {
    Clearing    // an open area in a forest
};

struct SLocation {
    ELocationType type{};

    // The middle of the location. What lies there once the map is finished
    // depends on the generator; its description says.
    Position center;

    // How far the location reaches from its center, roughly: a shape
    // that is not round reaches further in some directions than in others.
    // The generator's description says how rough.
    std::size_t radius = 0;
};

} // namespace RL
