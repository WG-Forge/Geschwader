#include "Tank.h"

#include <iostream>
#include <memory>
#include <queue>

#include "JSONParser.h"
#include "Map.h"

using namespace std;

Query Tank::step(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy)
{
    Query step_data;
    if (attack_to_destroy[(int)type]) {
        step_data = try_attack(map_matrix, tanks_can_be_attacked, attack_to_destroy);
    }

    return step_data.code != -1 ? step_data : try_move(map_matrix, tanks_can_be_attacked, attack_to_destroy);
}

Tank::Tank(json j, string tank_id_, TankType type, int max_health, int speed) : tank_id(stoi(tank_id_)), type(type), max_health(max_health), speed(speed) {
	player_id = j["player_id"].get<int>();
	health = j["health"].get<int>();
	spawn_position = Point(j["spawn_position"]);
	position = Point(j["position"]);
	capture_points = j["capture_points"].get<int>();
    shoot_range_bonus = j["shoot_range_bonus"].get<int>();
}

bool SPG::can_attack(const Tank& target, const vector<MapCode>& map_matrix)
{
    if (player_id == target.player_id) return false;
    int dist = distance(position, target.position);
    if (dist == 0) return false;
    return (dist <= (3 + shoot_range_bonus) && dist >= 3);
}

Query SPG::try_move(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy)
{
    DataAction action;
    vector<int> map_dist(map_matrix.size(), -1);
    queue<int> points_move;
    Point point_to_move;
    int rad = Map::get().rad;
    int max_counter = -1;
    int code_my_tank = code(position, rad);
    int max_counter_enemy = 0;

    for (int j = 0; j < map_matrix.size(); ++j) {
        map_dist[j] = (map_matrix[j] == MapCode::FRIENDLY_TANK || map_matrix[j] == MapCode::ENEMY_TANK || map_matrix[j] == MapCode::OBSTACLE) ? INT_MAX : -1;
    }

    map_dist[code_my_tank] = INT_MAX;
    points_move.push(code_my_tank);

    while (!points_move.empty()) {
        int curr_code = points_move.front();
        points_move.pop();
        Point curr_point = decode(curr_code, rad);
        int counter = 0;
        int counter_enemy = 0;
        vector<int> low_priority;

        if (distance(curr_point, Point(0, 0, 0)) >= 3 && distance(curr_point, Point(0, 0, 0)) <= (3 + shoot_range_bonus)) {
            ++counter;
            if (map_matrix[code(Point(0, 0, 0), rad)] == MapCode::ENEMY_TANK) ++counter_enemy;
        }
        for (int j = 0; j < near_gex.size(); ++j) {
            if (distance(curr_point, near_gex[j]) >= 3 && distance(curr_point, near_gex[j]) <= (3 + shoot_range_bonus)) {
                ++counter;
                if (map_matrix[code(near_gex[j], rad)] == MapCode::ENEMY_TANK) ++counter_enemy;
            }
        }
        if (counter_enemy > max_counter_enemy || (counter_enemy == max_counter_enemy && counter > max_counter)) {
            max_counter = counter;
            max_counter_enemy = counter_enemy;
            point_to_move = curr_point;
        }

        for (int j = 0; j < near_gex.size(); ++j) {
            Point buf = curr_point + near_gex[j];
            int buf_code = code(buf, rad);
            if (can_exist(buf, rad) && map_dist[buf_code] == -1) {
                if (map_matrix[buf_code] == MapCode::CATAPULT) {
                    map_dist[buf_code] = curr_code;
                    points_move.push(buf_code);
                }
                else {
                    low_priority.push_back(buf_code);
                }
            }
        }
        for (int i = 0; i < low_priority.size(); ++i) {
            map_dist[low_priority[i]] = curr_code;
            points_move.push(low_priority[i]);
        }
    }

    if (position == point_to_move) {
        // we on place, we shouldnt do anything.
        Tank* tank_to_kill = nullptr;
        cout << "Try to shoot anybody" << endl;
        for (auto& target : tanks_can_be_attacked) {
            if (can_attack(*target, map_matrix)) {
                bool can_be_used = true;
                for (int i = 0; i < attack_to_destroy.size(); ++i) {
                    if (attack_to_destroy[i] == target) {
                        can_be_used = false;
                        break;
                    }
                }
                if (can_be_used) {
                    tank_to_kill = target;
                    break;
                }
            }
        }
        if (tank_to_kill) {
            cout << (json)position << " attacked " << (json)tank_to_kill->position << endl;
            action.vehicle_id = tank_id;
            action.target = tank_to_kill->position;
            tanks_can_be_attacked.erase(tank_to_kill);
            --tank_to_kill->health;
            if (tank_to_kill->health == 0) {
                map_matrix[code(tank_to_kill->position, rad)] = MapCode::NOTHING;
                for (const auto& catapult : Map::get().catapult) {
                    if (tank_to_kill->position == catapult) {
                        map_matrix[code(catapult, rad)] = MapCode::CATAPULT;
                        break;
                    }
                }
                tank_to_kill->position.x = 10 * rad;
            }
            else {
                tanks_can_be_attacked.insert(tank_to_kill);
            }
            return { Action::SHOOT, action };
        }
        return {};
    }

    cout << "Max counter for SPG" << endl;
    cout << "MAX Counter : " << max_counter << endl;

    if (max_counter == -1) {
        cout << "Try safe" << endl;
        // safe your tank
        max_counter = 11;
        int dist = 10 * rad;

        for (int j = 0; j < near_gex.size(); ++j) {
            Point buf = position + near_gex[j];
            int buf_code = code(buf, rad);
            if (can_exist(buf, rad) && (map_matrix[buf_code] != MapCode::FRIENDLY_TANK && map_matrix[buf_code] != MapCode::ENEMY_TANK && map_matrix[buf_code] != MapCode::OBSTACLE)) {
                int index = safe_index(buf, map_matrix, player_id);
                if (index < max_counter || (index == max_counter && dist > distance(buf, Point(0, 0, 0)))) {
                    point_to_move = buf;
                    max_counter = index;
                    dist = distance(buf, Point(0, 0, 0));
                    continue;
                }
            }
        }
    }
    else {
        vector<Point> way;
        int code_end = code(point_to_move, rad);
        while (map_dist[code_end] != INT_MAX) {
            way.push_back(decode(code_end, rad));
            code_end = map_dist[code_end];
        }
        point_to_move = way[way.size() - 1];
    }
    cout << " Choosed place to move by SPG" << endl;
    action.vehicle_id = tank_id;
    action.target = point_to_move;
    map_matrix[code(point_to_move, rad)] = MapCode::FRIENDLY_TANK;
    map_matrix[code_my_tank] = MapCode::NOTHING;
    for (const auto& catapult : Map::get().catapult) {
        if (position == catapult) {
            map_matrix[code_my_tank] = MapCode::CATAPULT;
            break;
        }
    }
    return { Action::MOVE, action };
}

Query SPG::try_attack(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy)
{
    DataAction action;
    int rad = Map::get().rad;
    cout << (json)position << " attacked " << (json)attack_to_destroy[0]->position << endl;
    action.vehicle_id = tank_id;
    action.target = attack_to_destroy[0]->position;
    tanks_can_be_attacked.erase(attack_to_destroy[0]);
    --attack_to_destroy[0]->health;
    if (attack_to_destroy[0]->health == 0) {
        map_matrix[code(attack_to_destroy[0]->position, rad)] = MapCode::NOTHING;
        for (const auto& catapult : Map::get().catapult) {
            if (attack_to_destroy[0]->position == catapult) {
                map_matrix[code(catapult, rad)] = MapCode::CATAPULT;
                break;
            }
        }
        attack_to_destroy[0]->position.x = 10 * rad;
    }
    else {
        tanks_can_be_attacked.insert(attack_to_destroy[0]);
    }
    return { Action::SHOOT, action };
}

bool AT_SPG::can_attack(const Tank& target, const vector<MapCode>& map_matrix)
{
    if (player_id == target.player_id) return false;
    int dist = distance(target.position, position);
    if (dist == 0) return false;
    if (dist > (3 + shoot_range_bonus)) return false;

    int rad = Map::get().rad;
    Point buf = position;

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

Query AT_SPG::try_move(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy)
{
    DataAction action;
    int rad = Map::get().rad;
    cout << "start AT SPG " << endl;
    cout << " No shoot" << endl;
    vector<int> map_dist(map_matrix.size(), -1);
    queue<int> points_move;
    Point point_to_move;
    int max_counter = -1;
    int code_my_tank = code(position, rad);

    for (int j = 0; j < map_matrix.size(); ++j) {
        map_dist[j] = (map_matrix[j] == MapCode::FRIENDLY_TANK || map_matrix[j] == MapCode::ENEMY_TANK || map_matrix[j] == MapCode::OBSTACLE) ? INT_MAX : -1;
    }

    map_dist[code_my_tank] = INT_MAX;
    points_move.push(code_my_tank);

    while (!points_move.empty()) {
        int curr_code = points_move.front();
        points_move.pop();
        Point curr_point = decode(curr_code, rad);
        int counter = 0;
        vector<int> low_priority;
        unique_ptr<Tank> buf_attacker = make_unique<AT_SPG>();
        buf_attacker->position = curr_point;
        buf_attacker->player_id = 1;
        unique_ptr<Tank> buf_base = make_unique<SPG>();
        buf_base->position = Point(0, 0, 0);
        buf_base->player_id = 2;

        if (buf_attacker->can_attack(*buf_base, map_matrix)) ++counter;
        for (int j = 0; j < near_gex.size(); ++j) {
            buf_base->position = near_gex[j];
            if (buf_attacker->can_attack(*buf_base, map_matrix)) ++counter;
        }

        if (counter > max_counter && distance(curr_point, Point(0, 0, 0)) == 1) {
            max_counter = counter;
            point_to_move = curr_point;
        }
        if (max_counter == 4) break;

        for (int j = 0; j < near_gex.size(); ++j) {
            Point buf = curr_point + near_gex[j];
            int buf_code = code(buf, rad);
            if (can_exist(buf, rad) && map_dist[buf_code] == -1) {
                if (map_matrix[buf_code] == MapCode::CATAPULT) {
                    map_dist[buf_code] = curr_code;
                    points_move.push(buf_code);
                }
                else {
                    low_priority.push_back(buf_code);
                }
            }
        }
        for (int i = 0; i < low_priority.size(); ++i) {
            map_dist[low_priority[i]] = curr_code;
            points_move.push(low_priority[i]);
        }
    }

    if (position == point_to_move) {
        Tank* tank_to_kill = nullptr;
        cout << "Try to shoot anybody" << endl;
        for (auto& target : tanks_can_be_attacked) {
            if (can_attack(*target, map_matrix)) {
                bool can_be_used = true;
                for (int i = 0; i < attack_to_destroy.size(); ++i) {
                    if (attack_to_destroy[i] == target) {
                        can_be_used = false;
                        break;
                    }
                }
                if (can_be_used) {
                    tank_to_kill = target;
                    break;
                }
            }
        }
        if (tank_to_kill) {
            cout << (json)position << " attacked " << (json)tank_to_kill->position << endl;
            action.vehicle_id = tank_id;
            action.target = tank_to_kill->position;
            tanks_can_be_attacked.erase(tank_to_kill);
            --tank_to_kill->health;
            if (tank_to_kill->health == 0) {
                map_matrix[code(tank_to_kill->position, rad)] = MapCode::NOTHING;
                for (const auto& catapult : Map::get().catapult) {
                    if (tank_to_kill->position == catapult) {
                        map_matrix[code(catapult, rad)] = MapCode::CATAPULT;
                        break;
                    }
                }
                tank_to_kill->position.x = 10 * rad;
            }
            else {
                tanks_can_be_attacked.insert(tank_to_kill);
            }
            return { Action::SHOOT, action };
        }
        return {};
    }

    cout << "MAX Counter : " << max_counter << endl;

    if (max_counter == -1) {
        // safe your tank
        max_counter = 11;
        int dist = 2 * rad;

        for (int j = 0; j < near_gex.size(); ++j) {
            Point buf = position + near_gex[j];
            int buf_code = code(buf, rad);
            if (can_exist(buf, rad) && (map_matrix[buf_code] != MapCode::FRIENDLY_TANK && map_matrix[buf_code] != MapCode::ENEMY_TANK && map_matrix[buf_code] != MapCode::OBSTACLE)) {
                int index = safe_index(buf, map_matrix, player_id);
                if (index < max_counter || (index == max_counter && dist > distance(buf, Point(0, 0, 0)))) {
                    point_to_move = buf;
                    max_counter = index;
                    dist = distance(buf, Point(0, 0, 0));
                    continue;
                }
            }
        }
    }
    else {
        vector<Point> way;
        int code_end = code(point_to_move, rad);
        while (map_dist[code_end] != INT_MAX) {
            way.push_back(decode(code_end, rad));
            code_end = map_dist[code_end];
        }
        point_to_move = way[way.size() - 1];
    }
    cout << " Choosed place to move by AT_SPG" << endl;
    action.vehicle_id = tank_id;
    action.target = point_to_move;
    map_matrix[code(point_to_move, rad)] = MapCode::FRIENDLY_TANK;
    map_matrix[code_my_tank] = MapCode::NOTHING;
    for (const auto& catapult : Map::get().catapult) {
        if (position == catapult) {
            map_matrix[code_my_tank] = MapCode::CATAPULT;
            break;
        }
    }
    return { Action::MOVE, action };
}

Query AT_SPG::try_attack(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy)
{
    DataAction action;
    int rad = Map::get().rad;
    //shoot
    cout << (json)position << " attacked " << (json)attack_to_destroy[4]->position << endl;
    action.vehicle_id = tank_id;
    action.target = position + (position - attack_to_destroy[4]->position) / distance(position, attack_to_destroy[4]->position);
    tanks_can_be_attacked.erase(attack_to_destroy[4]);
    --attack_to_destroy[4]->health;
    if (attack_to_destroy[4]->health == 0) {
        map_matrix[code(attack_to_destroy[4]->position, rad)] = MapCode::NOTHING;
        for (const auto& catapult : Map::get().catapult) {
            if (attack_to_destroy[4]->position == catapult) {
                map_matrix[code(catapult, rad)] = MapCode::CATAPULT;
                break;
            }
        }
        attack_to_destroy[4]->position.x = 10 * rad;
    }
    else {
        tanks_can_be_attacked.insert(attack_to_destroy[4]);
    }
    return { Action::SHOOT, action };
}

bool Light_tank::can_attack(const Tank& target, const vector<MapCode>& map_matrix)
{
    if (player_id == target.player_id) return false;
    int dist = distance(position, target.position);
    if (dist == 0) return false;
    return (dist <= (2 + shoot_range_bonus) && dist >= 2);
}

Query Light_tank::try_move(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy)
{
    DataAction action;
    int rad = Map::get().rad;
    int code_my_tank = code(position, rad);
    Tank* tank_to_kill = nullptr;
    queue<int> find_tank;
    vector<int> map_tank(map_matrix.size(), -1);

    for (int j = 0; j < map_matrix.size(); ++j) {
        map_tank[j] = (map_matrix[j] == MapCode::OBSTACLE) ? INT_MAX : -1;
    }

    map_tank[code_my_tank] = INT_MAX;
    find_tank.push(code_my_tank);

    while (!find_tank.empty()) {
        int curr_code = find_tank.front();
        find_tank.pop();
        Point curr_point = decode(curr_code, rad);

        if (map_matrix[curr_code] == MapCode::ENEMY_TANK) {
            for (auto& target : tanks_can_be_attacked) {
                if (target->position == curr_point) {
                    if (target->health != 1) continue;
                    bool can_be_used = true;
                    for (int i = 0; i < attack_to_destroy.size(); ++i) {
                        if (attack_to_destroy[i] == target) {
                            can_be_used = false;
                            break;
                        }
                    }
                    if (can_be_used) {
                        tank_to_kill = target;
                        break;
                    }
                }
            }
            if (tank_to_kill) break;
        }

        for (int j = 0; j < near_gex.size(); ++j) {
            Point buf = curr_point + near_gex[j];
            int buf_code = code(buf, rad);
            if (can_exist(buf, rad) && map_tank[buf_code] == -1) {
                map_tank[buf_code] = curr_code;
                find_tank.push(buf_code);
            }
        }
    }

    if (!tank_to_kill) {
        cout << "Try to shoot anybody" << endl;
        for (auto& target : tanks_can_be_attacked) {
            if (can_attack(*target, map_matrix)) {
                bool can_be_used = true;
                for (int i = 0; i < attack_to_destroy.size(); ++i) {
                    if (attack_to_destroy[i] == target) {
                        can_be_used = false;
                        break;
                    }
                }
                if (can_be_used) {
                    tank_to_kill = target;
                    break;
                }
            }
        }
        if (tank_to_kill) {
            cout << (json)position << " attacked " << (json)tank_to_kill->position << endl;
            action.vehicle_id = tank_id;
            action.target = tank_to_kill->position;
            tanks_can_be_attacked.erase(tank_to_kill);
            --tank_to_kill->health;
            if (tank_to_kill->health == 0) {
                map_matrix[code(tank_to_kill->position, rad)] = MapCode::NOTHING;
                for (const auto& catapult : Map::get().catapult) {
                    if (tank_to_kill->position == catapult) {
                        map_matrix[code(catapult, rad)] = MapCode::CATAPULT;
                        break;
                    }
                }
                tank_to_kill->position.x = 10 * rad;
            }
            else {
                tanks_can_be_attacked.insert(tank_to_kill);
            }
            return { Action::SHOOT, action };
        }
        return {};
    }

    vector<int> map_dist(map_matrix.size(), -1);
    queue<pair<int, int>> points_move;
    Point point_to_move;
    bool find_position = false;

    for (int j = 0; j < map_matrix.size(); ++j) {
        map_dist[j] = (map_matrix[j] == MapCode::OBSTACLE) ? INT_MAX : -1;
    }

    map_dist[code_my_tank] = INT_MAX;
    points_move.push(make_pair(code_my_tank, 0));

    while (!points_move.empty()) {
        int curr_code = points_move.front().first;
        int made_steps = points_move.front().second;
        points_move.pop();
        Point curr_point = decode(curr_code, rad);

        int dist = distance(curr_point, tank_to_kill->position);

        if (dist == 2 && map_matrix[curr_code] != MapCode::ENEMY_TANK && map_matrix[curr_code] != MapCode::FRIENDLY_TANK) {
            point_to_move = curr_point;
            find_position = true;
            break;
        }
        for (int j = 0; j < near_gex.size(); ++j) {
            Point buf = curr_point + near_gex[j];
            int buf_code = code(buf, rad);
            if (can_exist(buf, rad) && map_dist[buf_code] == -1) {
                if (made_steps != 2 || (made_steps == 2 && map_matrix[buf_code] != MapCode::ENEMY_TANK && map_matrix[buf_code] != MapCode::FRIENDLY_TANK)) {
                    map_dist[buf_code] = curr_code;
                    points_move.push(make_pair(buf_code, made_steps + 1));
                }
            }
        }
    }

    vector<Point> way;

    int code_end = code(point_to_move, rad);
    while (map_dist[code_end] != INT_MAX) {
        way.push_back(decode(code_end, rad));
        code_end = map_dist[code_end];
    }
    cout << "Way size for LT : " << way.size() << endl;
    if (way.size() == 1) {
        point_to_move = way[0];
    }
    if (way.size() == 2) {
        point_to_move = way[0];
    }

    if (way.size() >= 3) {
        point_to_move = way[way.size() - 3];
    }
    cout << " Choosed place to move by LT" << endl;
    action.vehicle_id = tank_id;
    action.target = point_to_move;
    map_matrix[code(point_to_move, rad)] = MapCode::FRIENDLY_TANK;
    map_matrix[code_my_tank] = MapCode::NOTHING;
    for (const auto& catapult : Map::get().catapult) {
        if (position == catapult) {
            map_matrix[code_my_tank] = MapCode::CATAPULT;
            break;
        }
    }
    return { Action::MOVE, action };
}

Query Light_tank::try_attack(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy)
{
    DataAction action;
    int rad = Map::get().rad;
    //shoot
    cout << (json)position << " attacked " << (json)attack_to_destroy[1]->position << endl;
    action.vehicle_id = tank_id;
    action.target = attack_to_destroy[1]->position;
    tanks_can_be_attacked.erase(attack_to_destroy[1]);
    --attack_to_destroy[1]->health;
    if (attack_to_destroy[1]->health == 0) {
        map_matrix[code(attack_to_destroy[1]->position, rad)] = MapCode::NOTHING;
        for (const auto& catapult : Map::get().catapult) {
            if (attack_to_destroy[1]->position == catapult) {
                map_matrix[code(catapult, rad)] = MapCode::CATAPULT;
                break;
            }
        }
        attack_to_destroy[1]->position.x = 10 * rad;
    }
    else {
        tanks_can_be_attacked.insert(attack_to_destroy[1]);
    }
    return { Action::SHOOT, action };
}

bool Medium_tank::can_attack(const Tank& target, const vector<MapCode>& map_matrix)
{
    if (player_id == target.player_id) return false;
    int dist = distance(target.position, position);
    if (dist == 0) return false;
    return (dist <= (2 + shoot_range_bonus) && dist >= 2);
}

Query Medium_tank::try_move(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy)
{
    DataAction action;
    int rad = Map::get().rad;
    vector<int> map_dist(map_matrix.size(), -1);
    queue<pair<int, int>> points_move;
    int code_my_tank = code(position, rad);
    Point point_to_move;
    int dist_to_center = 12;

    if (distance(position, Point(0, 0, 0)) == 1) {
        cout << "We on place, only wait and shoot" << endl;
        cout << "Try to shoot anybody" << endl;
        Tank* tank_to_kill = nullptr;
        for (auto& target : tanks_can_be_attacked) {
            if (can_attack(*target, map_matrix)) {
                bool can_be_used = true;
                for (int i = 0; i < attack_to_destroy.size(); ++i) {
                    if (attack_to_destroy[i] == target) {
                        can_be_used = false;
                        break;
                    }
                }
                if (can_be_used) {
                    tank_to_kill = target;
                    break;
                }
            }
        }
        if (tank_to_kill) {
            cout << (json)position << " attacked " << (json)tank_to_kill->position << endl;
            action.vehicle_id = tank_id;
            action.target = tank_to_kill->position;
            tanks_can_be_attacked.erase(tank_to_kill);
            --tank_to_kill->health;
            if (tank_to_kill->health == 0) {
                map_matrix[code(tank_to_kill->position, rad)] = MapCode::NOTHING;
                for (const auto& catapult : Map::get().catapult) {
                    if (tank_to_kill->position == catapult) {
                        map_matrix[code(catapult, rad)] = MapCode::CATAPULT;
                        break;
                    }
                }
                tank_to_kill->position.x = 10 * rad;
            }
            else {
                tanks_can_be_attacked.insert(tank_to_kill);
            }
            return { Action::SHOOT, action };
        }
        return {};
    }

    for (int j = 0; j < map_matrix.size(); ++j) {
        map_dist[j] = (map_matrix[j] == MapCode::OBSTACLE) ? INT_MAX : -1;
    }

    map_dist[code_my_tank] = INT_MAX;
    points_move.push(make_pair(code_my_tank, 0));

    while (!points_move.empty()) {
        int curr_code = points_move.front().first;
        int made_steps = points_move.front().second;
        points_move.pop();
        Point curr_point = decode(curr_code, rad);

        int dist = distance(curr_point, Point(0, 0, 0));
        if (dist >= 1 && dist < dist_to_center && map_matrix[curr_code] != MapCode::ENEMY_TANK && map_matrix[curr_code] != MapCode::FRIENDLY_TANK) {
            point_to_move = curr_point;
            dist_to_center = dist;
        }
        if (dist_to_center == 1) break;

        for (int j = 0; j < near_gex.size(); ++j) {
            Point buf = curr_point + near_gex[j];
            int buf_code = code(buf, rad);
            if (can_exist(buf, rad) && map_dist[buf_code] == -1) {
                if (made_steps != 1 || (made_steps == 1 && map_matrix[buf_code] != MapCode::ENEMY_TANK && map_matrix[buf_code] != MapCode::FRIENDLY_TANK)) {
                    map_dist[buf_code] = curr_code;
                    points_move.push(make_pair(buf_code, made_steps + 1));
                }
            }
        }
    }

    vector<Point> way;
    bool can_move = false;
    int code_end = code(point_to_move, rad);
    while (map_dist[code_end] != INT_MAX) {
        way.push_back(decode(code_end, rad));
        code_end = map_dist[code_end];
    }
    cout << "Way size for ST : " << way.size() << endl;
    if (way.size() == 1) {
        point_to_move = way[0];
        can_move = true;
    }
    if (way.size() >= 2) {
        point_to_move = way[way.size() - 2];
        can_move = true;
    }

    if (!can_move) {
        cout << "I can not move my ST, only wait" << endl;
        return {};
    }

    if (can_move) {
        cout << " Choosed place to move by ST" << endl;
        action.vehicle_id = tank_id;
        action.target = point_to_move;
        map_matrix[code(point_to_move, rad)] = MapCode::FRIENDLY_TANK;
        map_matrix[code_my_tank] = MapCode::NOTHING;
        for (const auto& catapult : Map::get().catapult) {
            if (position == catapult) {
                map_matrix[code_my_tank] = MapCode::CATAPULT;
                break;
            }
        }
        return { Action::MOVE, action };
    }
}

Query Medium_tank::try_attack(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy)
{
    DataAction action;
    int rad = Map::get().rad;
    //shoot
    cout << (json)position << " attacked " << (json)attack_to_destroy[3]->position << endl;
    action.vehicle_id = tank_id;
    action.target = attack_to_destroy[3]->position;
    tanks_can_be_attacked.erase(attack_to_destroy[3]);
    --attack_to_destroy[3]->health;
    if (attack_to_destroy[3]->health == 0) {
        map_matrix[code(attack_to_destroy[3]->position, rad)] = MapCode::NOTHING;
        for (const auto& catapult : Map::get().catapult) {
            if (attack_to_destroy[3]->position == catapult) {
                map_matrix[code(catapult, rad)] = MapCode::CATAPULT;
                break;
            }
        }
        attack_to_destroy[3]->position.x = 10 * rad;
    }
    else {
        tanks_can_be_attacked.insert(attack_to_destroy[3]);
    }
    return { Action::SHOOT, action };
}

bool Heavy_tank::can_attack(const Tank& target, const vector<MapCode>& map_matrix)
{
    if (player_id == target.player_id) return false;
    int dist = distance(target.position, position);
    if (dist == 0) return false;
    return (dist <= (2 + shoot_range_bonus));
}

Query Heavy_tank::try_move(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy)
{
    DataAction action;
    int rad = Map::get().rad;
    vector<int> map_dist(map_matrix.size(), -1);
    queue<int> points_move;
    int code_my_tank = code(position, rad);
    Point point_to_move(0, 0, 0);
    Point point_to_move_if_occupied;
    bool can_reach_center = false;
    int dist_to_center = 15;

    for (int j = 0; j < map_matrix.size(); ++j) {
        map_dist[j] = (map_matrix[j] == MapCode::FRIENDLY_TANK || map_matrix[j] == MapCode::ENEMY_TANK || map_matrix[j] == MapCode::OBSTACLE) ? INT_MAX : -1;
    }

    map_dist[code_my_tank] = INT_MAX;
    points_move.push(code_my_tank);

    while (!points_move.empty()) {
        int curr_code = points_move.front();
        points_move.pop();
        Point curr_point = decode(curr_code, rad);
        vector<int> low_priority;
        bool on_base = false;

        if (curr_point == point_to_move) {
            can_reach_center = true;
            break;
        }

        int dist = distance(curr_point, point_to_move);

        if (dist < dist_to_center) {
            point_to_move_if_occupied = curr_point;
            dist_to_center = dist;
        }

        for (int j = 0; j < near_gex.size(); ++j) {
            Point buf = curr_point + near_gex[j];
            int buf_code = code(buf, rad);
            if (can_exist(buf, rad) && map_dist[buf_code] == -1) {
                if (map_matrix[buf_code] == MapCode::CATAPULT) {
                    map_dist[buf_code] = curr_code;
                    points_move.push(buf_code);
                }
                else {
                    low_priority.push_back(buf_code);
                }
            }
        }
        for (int i = 0; i < low_priority.size(); ++i) {
            map_dist[low_priority[i]] = curr_code;
            points_move.push(low_priority[i]);
        }
    }

    if (!can_reach_center) {
        point_to_move = point_to_move_if_occupied;
    }

    if (position == point_to_move) {
        cout << "Try to shoot anybody" << endl;
        Tank* tank_to_kill = nullptr;
        for (auto& target : tanks_can_be_attacked) {
            if (can_attack(*target, map_matrix)) {
                bool can_be_used = true;
                for (int i = 0; i < attack_to_destroy.size(); ++i) {
                    if (attack_to_destroy[i] == target) {
                        can_be_used = false;
                        break;
                    }
                }
                if (can_be_used) {
                    tank_to_kill = target;
                    break;
                }
            }
        }
        if (tank_to_kill) {
            cout << (json)position << " attacked " << (json)tank_to_kill->position << endl;
            action.vehicle_id = tank_id;
            action.target = tank_to_kill->position;
            tanks_can_be_attacked.erase(tank_to_kill);
            --tank_to_kill->health;
            if (tank_to_kill->health == 0) {
                map_matrix[code(tank_to_kill->position, rad)] = MapCode::NOTHING;
                for (const auto& catapult : Map::get().catapult) {
                    if (tank_to_kill->position == catapult) {
                        map_matrix[code(catapult, rad)] = MapCode::CATAPULT;
                        break;
                    }
                }
                tank_to_kill->position.x = 10 * rad;
            }
            else {
                tanks_can_be_attacked.insert(tank_to_kill);
            }
            return { Action::SHOOT, action };
        }
        return {};
    }

    cout << "Can reach anything by TT" << endl;
    vector<Point> way;
    int code_end = code(point_to_move, rad);
    while (map_dist[code_end] != INT_MAX) {
        way.push_back(decode(code_end, rad));
        code_end = map_dist[code_end];
    }
    point_to_move = way[way.size() - 1];

    cout << " Choosed place to move by TT" << endl;
    action.vehicle_id = tank_id;
    action.target = point_to_move;
    map_matrix[code(point_to_move, rad)] = MapCode::FRIENDLY_TANK;
    map_matrix[code_my_tank] = MapCode::NOTHING;
    for (const auto& catapult : Map::get().catapult) {
        if (position == catapult) {
            map_matrix[code_my_tank] = MapCode::CATAPULT;
            break;
        }
    }
    return { Action::MOVE, action };
}

Query Heavy_tank::try_attack(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy)
{
    DataAction action;
    int rad = Map::get().rad;
    //shoot
    cout << (json)position << " attacked " << (json)attack_to_destroy[2]->position << endl;
    action.vehicle_id = tank_id;
    action.target = attack_to_destroy[2]->position;
    tanks_can_be_attacked.erase(attack_to_destroy[2]);
    --attack_to_destroy[2]->health;
    if (attack_to_destroy[2]->health == 0) {
        map_matrix[code(attack_to_destroy[2]->position, rad)] = MapCode::NOTHING;
        for (const auto& catapult : Map::get().catapult) {
            if (attack_to_destroy[2]->position == catapult) {
                map_matrix[code(catapult, rad)] = MapCode::CATAPULT;
                break;
            }
        }
        attack_to_destroy[2]->position.x = 10 * rad;
    }
    else {
        tanks_can_be_attacked.insert(attack_to_destroy[2]);
    }
    return { Action::SHOOT, action };
}
