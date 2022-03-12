#include "Tank.h"

#include <iostream>

#include "JSONParser.h"
#include "Map.h"

using namespace std;

Tank::Tank(json j, string tank_id_, string type, int max_health, int speed) : tank_id(stoi(tank_id_)), type(type), max_health(max_health), speed(speed) {
	player_id = j["player_id"].get<int>();
	health = j["health"].get<int>();
	spawn_position = Point(j["spawn_position"]);
	position = Point(j["position"]);
	capture_points = j["capture_points"].get<int>();
}

Query Tank::try_move(vector<MapCode> map_matrix, int rad)
{
    DataAction action;
    queue<int> points_move;
    Point point_to_move;
    vector<int> map_dist(map_matrix.size());
    int max_counter = -1;

    for (int j = 0; j < map_matrix.size(); ++j) {
        map_dist[j] = (map_matrix[j] == MapCode::NOTHING) ? -1 : INT_MAX;
    }
    map_dist[code(position, rad)] = INT_MAX;
    points_move.push(code(position, rad));
    while (!points_move.empty()) {
        int curr_code = points_move.front();
        Point curr_point = decode(curr_code, rad);
        int counter = 0;
        points_move.pop();
        if (distance(curr_point, Point(0, 0, 0)) == 3) ++counter;
        for (const auto& point : near_gex) {
            if (distance(curr_point, point) == 4) break;
            if (distance(curr_point, point) == 3) ++counter;
        }
        if (counter >= max_counter) {
            max_counter = counter;
            point_to_move = curr_point;
        }
        if (max_counter == 3) break;
        for (const auto& point : near_gex) {
            Point buf = curr_point + point;
            int buf_code = code(buf, rad);
            if (can_exist(buf, rad) && map_dist[buf_code] == -1) {
                map_dist[buf_code] = curr_code;
                points_move.push(buf_code);
            }
        }
    }
    if (max_counter == -1) {
        cout << "I can not find a way" << endl;
    }
    else {
        vector<Point> way;
        int code_end = code(point_to_move, rad);
        cout << "MAX Counter : " << max_counter << endl;
        while (map_dist[code_end] != INT_MAX) {
            way.push_back(decode(code_end, rad));
            code_end = map_dist[code_end];
        }
        if (way.size() == 0) {
            max_counter = -1;
        }
        else {
            point_to_move = way[way.size() - 1];
        }
        cout << " Choosed place to move " << endl;
        action.vehicle_id = tank_id;
        action.target = point_to_move;
        map_matrix[code(point_to_move, rad)] = MapCode::FRIENDLY_TANK;
        map_matrix[code(position, rad)] = MapCode::NOTHING;
        return { Action::MOVE, action };
    }
    return {};
}

Query SPG::step(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)> tanks_can_be_attacked)
{
    DataAction action;
    vector<int> map_dist(map_matrix.size());
    int rad = Map::get().size - 1;

    Query move = try_move(map_matrix, rad);
    if (move.code == -1) {
        cout << "Try attack by SPG" << endl;
        Tank* tank_to_attack = nullptr;
        for (auto victim : tanks_can_be_attacked) {
            if (can_attack(*victim, map_matrix)) {
                tank_to_attack = victim;
                break;
            }
        }
        if (tank_to_attack == nullptr) {
            cout << " We can not do anything, we will only wait!" << endl;
        }
        else {
            cout << (json)position << " attacked " << (json)tank_to_attack->position << endl;
            action.vehicle_id = tank_id;
            action.target = tank_to_attack->position;
            --tank_to_attack->health;
            tanks_can_be_attacked.erase(tank_to_attack);
            if (tank_to_attack->health == 0) {
                map_matrix[code(tank_to_attack->position, rad)] = MapCode::NOTHING;
                tank_to_attack->position.x = 10 * rad;
            }
            else {
                tanks_can_be_attacked.insert(tank_to_attack);
            }
            return { Action::SHOOT, action };
        }
    }
    return move;
}

bool SPG::can_attack(const Tank& target, const vector<MapCode>& map_matrix)
{
    if (player_id == target.player_id) return false;
    return (distance(position, target.position) == 3);
}

Query AT_SPG::step(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)> tanks_can_be_attacked)
{
    return Query();
}

bool AT_SPG::can_attack(const Tank& target, const vector<MapCode>& map_matrix)
{
    int rad = Map::get().size - 1;
    if(distance(target.position, position) > 3) return false;
    Point buf(position);
    if (position.x == target.position.x) {
        int i = position.y - target.position.y;
        bool flag = (i > 0);
        for (i = abs(i); i > 1; --i) {
            (flag) ? --buf.y : ++buf.y;
            (flag) ? ++buf.z : --buf.z;
            if (map_matrix[code(buf, rad)] == MapCode::OBSTACLE) return false;
        }
        return true;
    }
    if (position.y == target.position.y) {
        int i = position.x - target.position.x;
        bool flag = (i > 0);
        for (i = abs(i); i > 1; --i) {
            (flag) ? --buf.x : ++buf.x;
            (flag) ? ++buf.z : --buf.z;
            if (map_matrix[code(buf, rad)] == MapCode::OBSTACLE) return false;
        }
        return true;
    }
    if (position.z == target.position.z) {
        int i = position.x - target.position.x;
        bool flag = (i > 0);
        for (i = abs(i); i > 1; --i) {
            (flag) ? --buf.x : ++buf.x;
            (flag) ? ++buf.y : --buf.y;
            if (map_matrix[code(buf, rad)] == MapCode::OBSTACLE) return false;
        }
        return true;
    }
    return false;
}

Query Light_tank::step(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)> tanks_can_be_attacked)
{
    return Query();
}

bool Light_tank::can_attack(const Tank& target, const vector<MapCode>& map_matrix)
{
    return (distance(target.position, position) == 2);
}

Query Medium_tank::step(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)> tanks_can_be_attacked)
{
    return Query();
}

bool Medium_tank::can_attack(const Tank& target, const vector<MapCode>& map_matrix)
{
    return (distance(target.position, position) == 2);
}

Query Heavy_tank::step(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)> tanks_can_be_attacked)
{
    return Query();
}

bool Heavy_tank::can_attack(const Tank& target, const vector<MapCode>& map_matrix)
{
    return (distance(target.position, position) <= 2);
}
