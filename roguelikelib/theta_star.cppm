//////////////////////////////////////////////////////////////////////////
// Theta* Path Finding Algorithm
//
// Implementation of the Theta* algorithm for any-angle path planning on grids
// as described in "Theta*: Any-Angle Path Planning on Grids" by Daniel et al.
//
// Theta* extends A* to find paths that are not constrained to grid edges,
// allowing for shorter, more natural paths in grid-based environments.
//////////////////////////////////////////////////////////////////////////

module;

# include <cmath>
# include <limits>
# include <queue>
# include <unordered_map>
# include <algorithm>
# include <memory>

export module rl.theta_star;

import rl.map;
import rl.matrix;
import rl.position;
import rl.distance;
import std;

export namespace RL
{

// Forward declarations
class CThetaStarNode;
using ThetaStarNodePtr = std::shared_ptr<CThetaStarNode>;

// Line of sight check result
struct SLineOfSightResult {
    bool visible;
    Position blocking_pos; // Position of blocking obstacle, if any

    SLineOfSightResult(bool v = true, Position bp = Position(Position::invalid, Position::invalid))
        : visible(v), blocking_pos(bp) {}
};

// Theta* node structure
class CThetaStarNode {
public:
    Position pos;
    ThetaStarNodePtr parent;
    double g; // Cost from start to current node
    double h; // Heuristic cost from current node to goal
    double f; // Total cost (g + h)

    // For Angle-Propagation Theta*
    double angle_start;
    double angle_end;

    CThetaStarNode(Position p, ThetaStarNodePtr par = nullptr, double g_val = 0.0, double h_val = 0.0)
        : pos(p), parent(par), g(g_val), h(h_val), f(g_val + h_val),
          angle_start(0.0), angle_end(2 * M_PI) {}

    bool operator>(const CThetaStarNode& other) const {
        // For priority queue (min-heap based on f-value)
        if (f != other.f) {
            return f > other.f; // Higher f-value has lower priority
        }
        // Tie-breaking: prefer higher g-value (more informed paths)
        return g > other.g;
    }

    bool operator<(const CThetaStarNode& other) const {
        return f < other.f;
    }
};

// Comparison function for priority queue
struct ThetaStarNodeCompare {
    bool operator()(const ThetaStarNodePtr& a, const ThetaStarNodePtr& b) const {
        return *a > *b;
    }
};

// Main Theta* pathfinding class
class CThetaStar {
public:
    // Constructor
    CThetaStar() {
        // Initialize default line of sight function
        line_of_sight_func = [this](const CMap& level, const Position& start, const Position& end) {
            return this->DefaultCheckLineOfSight(level, start, end);
        };

        // Initialize default heuristic function
        heuristic_func = [](const Position& a, const Position& b) {
            // Euclidean distance heuristic
            double dx = static_cast<double>(std::abs(static_cast<int>(a.x) - static_cast<int>(b.x)));
            double dy = static_cast<double>(std::abs(static_cast<int>(a.y) - static_cast<int>(b.y)));
            return std::sqrt(dx * dx + dy * dy);
        };
    }

    // Basic Theta* path finding
    // level: The map/grid to navigate
    // start: Starting position
    // end: Goal position
    // path: Output path (if found)
    // diagonals: Whether to allow diagonal movement
    // Returns: true if path found, false otherwise
    bool FindPath(
        const CMap& level,
        const Position& start,
        const Position& end,
        std::vector<Position>& path,
        bool diagonals = true)
    {
        return BasicThetaStar(level, start, end, path, diagonals);
    }

    // Angle-Propagation Theta* path finding
    bool FindPathAP(
        const CMap& level,
        const Position& start,
        const Position& end,
        std::vector<Position>& path,
        bool diagonals = true)
    {
        return AnglePropagationThetaStar(level, start, end, path, diagonals);
    }

    // Set custom line of sight function (for different collision detection)
    void SetLineOfSightFunction(std::function<SLineOfSightResult(const CMap&, const Position&, const Position&)> los_func) {
        line_of_sight_func = los_func;
    }

    // Set custom heuristic function
    void SetHeuristicFunction(std::function<double(const Position&, const Position&)> heuristic_func) {
        heuristic_func = heuristic_func;
    }

    // Default line of sight checking using Bresenham's algorithm
    // Made public so it can be used by the CheckLineOfSight free function
    SLineOfSightResult DefaultCheckLineOfSight(
        const CMap& level,
        const Position& start,
        const Position& end) const
    {
        // If start and end are the same, they're visible
        if (start == end) {
            return SLineOfSightResult(true);
        }

        // Use Bresenham's line algorithm to check for obstacles
        std::vector<Position> line = start.BuildBresenhamLine(end);

        // Check each position along the line (except start and end)
        for (size_t i = 1; i < line.size() - 1; i++) {
            const Position& pos = line[i];

            // If any position along the line is not passable, there's no line of sight
            if (!level.inside(pos) || !level.get(pos).isPassable()) {
                return SLineOfSightResult(false, pos);
            }
        }

        return SLineOfSightResult(true);
    }

private:
    // Line of sight function type
    std::function<SLineOfSightResult(const CMap&, const Position&, const Position&)> line_of_sight_func;

    // Heuristic function type
    std::function<double(const Position&, const Position&)> heuristic_func =
        [](const Position& a, const Position& b) {
            // Euclidean distance heuristic
            double dx = static_cast<double>(std::abs(static_cast<int>(a.x) - static_cast<int>(b.x)));
            double dy = static_cast<double>(std::abs(static_cast<int>(a.y) - static_cast<int>(b.y)));
            return std::sqrt(dx * dx + dy * dy);
        };

    // Basic Theta* algorithm implementation
    bool BasicThetaStar(
        const CMap& level,
        const Position& start,
        const Position& end,
        std::vector<Position>& path,
        bool diagonals)
    {
        // Clear any existing path
        path.clear();

        // Check if start or end positions are valid
        if (!level.inside(start) || !level.inside(end)) {
            return false;
        }

        // Check if start or end positions are blocked
        if (!level.get(start).isPassable() || !level.get(end).isPassable()) {
            return false;
        }

        // If start equals end, return trivial path
        if (start == end) {
            path.push_back(start);
            return true;
        }

        // Priority queue for open set
        auto open_set_compare = [](const ThetaStarNodePtr& a, const ThetaStarNodePtr& b) {
            return *a > *b;
        };
        std::priority_queue<ThetaStarNodePtr, std::vector<ThetaStarNodePtr>, decltype(open_set_compare)>
            open_set(open_set_compare);

        // Create start node
        auto start_node = std::make_shared<CThetaStarNode>(start, nullptr, 0.0, heuristic_func(start, end));
        open_set.push(start_node);

        // Map to store the best node for each position
        std::unordered_map<std::size_t, std::unordered_map<std::size_t, ThetaStarNodePtr>> nodes;
        nodes[start.x][start.y] = start_node;

        // Get neighbors offsets
        std::vector<Position> neighbors = GetNeighborOffsets(diagonals);

        while (!open_set.empty()) {
            // Get node with lowest f-value
            auto current = open_set.top();
            open_set.pop();

            // Check if we reached the goal
            if (current->pos == end) {
                // Reconstruct path
                ReconstructPath(current, path);
                return true;
            }

            // Explore neighbors
            for (const auto& offset : neighbors) {
                // Calculate neighbor position with bounds checking
                int nx = static_cast<int>(current->pos.x) + static_cast<int>(offset.x);
                int ny = static_cast<int>(current->pos.y) + static_cast<int>(offset.y);

                // Skip if neighbor would be out of bounds (we'll check exact bounds later)
                if (nx < 0 || ny < 0) {
                    continue;
                }

                Position neighbor_pos{
                    static_cast<std::size_t>(nx),
                    static_cast<std::size_t>(ny)
                };

                // Check if neighbor is within bounds and passable
                if (!level.inside(neighbor_pos) || !level.get(neighbor_pos).isPassable()) {
                    continue;
                }

                // Calculate tentative g-value
                double edge_cost = CalculateEdgeCost(current->pos, neighbor_pos);
                double tentative_g = current->g + edge_cost;

                // Check if this neighbor already exists and has a better path
                auto& x_nodes = nodes[neighbor_pos.x];
                auto it = x_nodes.find(neighbor_pos.y);

                ThetaStarNodePtr neighbor_node;
                bool new_node = false;

                if (it == x_nodes.end()) {
                    // Create new node
                    neighbor_node = std::make_shared<CThetaStarNode>(
                        neighbor_pos, current, tentative_g, heuristic_func(neighbor_pos, end));
                    x_nodes[neighbor_pos.y] = neighbor_node;
                    new_node = true;
                } else {
                    neighbor_node = it->second;

                    // If we found a better path to this node, update it
                    if (tentative_g < neighbor_node->g) {
                        neighbor_node->g = tentative_g;
                        neighbor_node->f = tentative_g + neighbor_node->h;
                        neighbor_node->parent = current;
                    } else {
                        // Already have a better path to this node
                        continue;
                    }
                }

                // Theta* specific: Check if we can see the parent from the neighbor
                // If so, we might be able to create a direct path
                if (current->parent) {
                    // Check line of sight between current node's parent and neighbor
                    SLineOfSightResult los = line_of_sight_func(level, current->parent->pos, neighbor_pos);
                    if (los.visible) {
                        // Calculate the cost of the direct path
                        double direct_cost = current->parent->g +
                            CalculateEdgeCost(current->parent->pos, neighbor_pos);

                        // If the direct path is better than the current path through current node
                        if (direct_cost < neighbor_node->g) {
                            neighbor_node->g = direct_cost;
                            neighbor_node->f = direct_cost + neighbor_node->h;
                            neighbor_node->parent = current->parent;
                        }
                    }
                }

                // Add to open set if not already there
                // Note: We don't check if it's already in the open set because
                // we want to re-evaluate nodes when we find better paths
                open_set.push(neighbor_node);
            }
        }

        // No path found
        return false;
    }

    // Angle-Propagation Theta* algorithm implementation
    bool AnglePropagationThetaStar(
        const CMap& level,
        const Position& start,
        const Position& end,
        std::vector<Position>& path,
        bool diagonals)
    {
        // Clear any existing path
        path.clear();

        // Check if start or end positions are valid
        if (!level.inside(start) || !level.inside(end)) {
            return false;
        }

        // Check if start or end positions are blocked
        if (!level.get(start).isPassable() || !level.get(end).isPassable()) {
            return false;
        }

        // If start equals end, return trivial path
        if (start == end) {
            path.push_back(start);
            return true;
        }

        // Priority queue for open set
        auto open_set_compare = [](const ThetaStarNodePtr& a, const ThetaStarNodePtr& b) {
            return *a > *b;
        };
        std::priority_queue<ThetaStarNodePtr, std::vector<ThetaStarNodePtr>, decltype(open_set_compare)>
            open_set(open_set_compare);

        // Create start node
        auto start_node = std::make_shared<CThetaStarNode>(start, nullptr, 0.0, heuristic_func(start, end));
        start_node->angle_start = 0.0;
        start_node->angle_end = 2 * M_PI;
        open_set.push(start_node);

        // Map to store the best node for each position
        std::unordered_map<std::size_t, std::unordered_map<std::size_t, ThetaStarNodePtr>> nodes;
        nodes[start.x][start.y] = start_node;

        // Get neighbors offsets
        std::vector<Position> neighbors = GetNeighborOffsets(diagonals);

        while (!open_set.empty()) {
            // Get node with lowest f-value
            auto current = open_set.top();
            open_set.pop();

            // Check if we reached the goal
            if (current->pos == end) {
                // Reconstruct path
                ReconstructPath(current, path);
                return true;
            }

            // Explore neighbors
            for (const auto& offset : neighbors) {
                // Calculate neighbor position with bounds checking
                int nx = static_cast<int>(current->pos.x) + static_cast<int>(offset.x);
                int ny = static_cast<int>(current->pos.y) + static_cast<int>(offset.y);

                // Skip if neighbor would be out of bounds (we'll check exact bounds later)
                if (nx < 0 || ny < 0) {
                    continue;
                }

                Position neighbor_pos{
                    static_cast<std::size_t>(nx),
                    static_cast<std::size_t>(ny)
                };

                // Check if neighbor is within bounds and passable
                if (!level.inside(neighbor_pos) || !level.get(neighbor_pos).isPassable()) {
                    continue;
                }

                // Calculate tentative g-value
                double edge_cost = CalculateEdgeCost(current->pos, neighbor_pos);
                double tentative_g = current->g + edge_cost;

                // Check if this neighbor already exists and has a better path
                auto& x_nodes = nodes[neighbor_pos.x];
                auto it = x_nodes.find(neighbor_pos.y);

                ThetaStarNodePtr neighbor_node;
                bool new_node = false;

                if (it == x_nodes.end()) {
                    // Create new node
                    neighbor_node = std::make_shared<CThetaStarNode>(
                        neighbor_pos, current, tentative_g, heuristic_func(neighbor_pos, end));
                    x_nodes[neighbor_pos.y] = neighbor_node;
                    new_node = true;
                } else {
                    neighbor_node = it->second;

                    // If we found a better path to this node, update it
                    if (tentative_g < neighbor_node->g) {
                        neighbor_node->g = tentative_g;
                        neighbor_node->f = tentative_g + neighbor_node->h;
                        neighbor_node->parent = current;
                    } else {
                        // Already have a better path to this node
                        continue;
                    }
                }

                // Theta* specific: Check if we can see the parent from the neighbor
                // If so, we might be able to create a direct path
                if (current->parent) {
                    // Check line of sight between current node's parent and neighbor
                    SLineOfSightResult los = line_of_sight_func(level, current->parent->pos, neighbor_pos);
                    if (los.visible) {
                        // Calculate the cost of the direct path
                        double direct_cost = current->parent->g +
                            CalculateEdgeCost(current->parent->pos, neighbor_pos);

                        // If the direct path is better than the current path through current node
                        if (direct_cost < neighbor_node->g) {
                            neighbor_node->g = direct_cost;
                            neighbor_node->f = direct_cost + neighbor_node->h;
                            neighbor_node->parent = current->parent;

                            // Propagate angle ranges
                            neighbor_node->angle_start = current->parent->angle_start;
                            neighbor_node->angle_end = current->parent->angle_end;
                        }
                    }
                }

                // Angle-Propagation specific: Propagate angle ranges
                if (!new_node) {
                    // For existing nodes, we need to check angle propagation
                    // This is a simplified version - full AP Theta* is more complex
                    double angle_to_current = CalculateAngle(current->pos, neighbor_pos);

                    // Check if current node's angle range includes the angle to neighbor
                    if (AngleInRange(angle_to_current, current->angle_start, current->angle_end)) {
                        // Propagate the angle range
                        neighbor_node->angle_start = current->angle_start;
                        neighbor_node->angle_end = current->angle_end;
                    }
                }

                // Add to open set
                open_set.push(neighbor_node);
            }
        }

        // No path found
        return false;
    }


    // Reconstruct path from goal node to start
    void ReconstructPath(ThetaStarNodePtr node, std::vector<Position>& path) const
    {
        std::vector<Position> temp_path;

        // Walk backwards from goal to start
        while (node != nullptr) {
            temp_path.push_back(node->pos);
            node = node->parent;
        }

        // Reverse to get path from start to goal
        path.assign(temp_path.rbegin(), temp_path.rend());
    }

    // Get neighbor offsets based on diagonal movement setting
    std::vector<Position> GetNeighborOffsets(bool diagonals) const
    {
        std::vector<Position> offsets;

        // Cardinal directions (N, S, E, W)
        offsets.emplace_back(0, 1);     // North
        offsets.emplace_back(0, -1);    // South
        offsets.emplace_back(1, 0);     // East
        offsets.emplace_back(-1, 0);    // West

        if (diagonals) {
            // Diagonal directions (NE, NW, SE, SW)
            offsets.emplace_back(1, 1);     // Northeast
            offsets.emplace_back(-1, 1);    // Northwest
            offsets.emplace_back(1, -1);    // Southeast
            offsets.emplace_back(-1, -1);   // Southwest
        }

        return offsets;
    }

    // Calculate edge cost between two positions (Euclidean distance)
    double CalculateEdgeCost(const Position& a, const Position& b) const
    {
        double dx = static_cast<double>(std::abs(static_cast<int>(a.x) - static_cast<int>(b.x)));
        double dy = static_cast<double>(std::abs(static_cast<int>(a.y) - static_cast<int>(b.y)));
        return std::sqrt(dx * dx + dy * dy);
    }

    // Calculate angle from a to b in radians
    double CalculateAngle(const Position& from, const Position& to) const
    {
        double dx = static_cast<double>(static_cast<int>(to.x) - static_cast<int>(from.x));
        double dy = static_cast<double>(static_cast<int>(to.y) - static_cast<int>(from.y));
        return std::atan2(dy, dx);
    }

    // Check if an angle is within a range
    bool AngleInRange(double angle, double start, double end) const
    {
        // Normalize angle to [0, 2*PI)
        while (angle < 0) angle += 2 * M_PI;
        while (angle >= 2 * M_PI) angle -= 2 * M_PI;

        // Handle range wrapping around 0
        if (start <= end) {
            return angle >= start && angle <= end;
        } else {
            return angle >= start || angle <= end;
        }
    }
};

// Convenience function for Basic Theta*
inline bool ThetaStarFindPath(
    const CMap& level,
    const Position& start,
    const Position& end,
    std::vector<Position>& path,
    bool diagonals = true)
{
    CThetaStar theta_star;
    return theta_star.FindPath(level, start, end, path, diagonals);
}

// Convenience function for Angle-Propagation Theta*
inline bool ThetaStarFindPathAP(
    const CMap& level,
    const Position& start,
    const Position& end,
    std::vector<Position>& path,
    bool diagonals = true)
{
    CThetaStar theta_star;
    return theta_star.FindPathAP(level, start, end, path, diagonals);
}

// Standalone line of sight check function
inline SLineOfSightResult CheckLineOfSight(
    const CMap& level,
    const Position& start,
    const Position& end)
{
    CThetaStar theta_star;
    return theta_star.DefaultCheckLineOfSight(level, start, end);
}

} // namespace RL
