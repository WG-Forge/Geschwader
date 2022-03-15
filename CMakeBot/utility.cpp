#include "utility.h"

#include <memory>

#include "GameState.h"
#include "Tank.h"
#include "Map.h"

using namespace std;

const vector<Point> near_gex = { Point(1, -1,  0), Point(1,  0, -1), Point(0, 1, -1),
   Point(-1, 1,  0), Point(-1,  0, 1), Point(0, -1, 1) };

int code(const Point& p, int rad) {
    return (p.x + rad) * (2 * rad + 1) + (p.y + rad);
}

Point decode(int value, int rad) {
    Point buf;
    buf.x = value / (2 * rad + 1) - rad;
    buf.y = value % (2 * rad + 1) - rad;
    buf.z = 0 - buf.x - buf.y;
    return buf;
}

bool can_exist(const Point& buf, const int& rad) {
    if ((buf.x + buf.y + buf.z) != 0) return false;
    if (max(max(abs(buf.x), abs(buf.y)), abs(buf.z)) > rad) return false;
    return true;
}

int distance(const Point& x1, const Point& x2) {
    return max(max(abs(x1.x - x2.x), abs(x1.y - x2.y)), abs(x1.z - x2.z));
}

int safe_index(Point ps, const vector<MapCode>& map_matrix, int idx)
{
    unique_ptr<Tank> buf = make_unique<SPG>();
    buf->position = ps;
    int counter = 0;
    for (const auto& player : GameState::get().players) {
        if (player.idx != idx) {
            for (auto& attacker : GameState::get().vehicles[player.idx]) {
                bool is_mod = false;
                for (const auto& catapult : Map::get().catapult) {
                    if (attacker->position == catapult) {
                        is_mod = true;
                        break;
                    }
                }
                if (attacker->can_attack(*buf, map_matrix, is_mod)) ++counter;
            }
        }
    }
    return counter;
}
