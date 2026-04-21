#pragma once

#include "../world/Map.hpp"

namespace AI {

// Returns the first step from 'from' toward 'to' avoiding non-walkable tiles.
// Returns 'from' if no path exists or already at destination.
Position bfsStep(Position from, Position to, const Map& map);

} // namespace AI
