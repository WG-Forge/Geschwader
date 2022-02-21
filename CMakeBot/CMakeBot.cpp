#include "CMakeBot.h"

using namespace std;

enum MapCode {
    NOTHING = 0,
    ENEMY_TANK = 1,
    FRIENDLY_TANK = 2,
    OBSTACLE = 3,
};

class Timer {
private:
    using clock_t = chrono::high_resolution_clock;
    using second_t = chrono::duration<double, ratio<1> >;
    chrono::time_point<clock_t> m_beg;

public:
    Timer() : m_beg(clock_t::now()) {}

    void reset() {
        m_beg = clock_t::now();
    }

    double elapsed() const {
        return chrono::duration_cast<second_t>(clock_t::now() - m_beg).count();
    }
};

const vector<Point> near_gex{ Point(1, -1,  0), Point(1,  0, -1), Point(0, 1, -1),
   Point(-1, 1,  0), Point(-1,  0, 1), Point(0, -1, 1) };

//assigns unique index for point
int code(const Point& p, int rad) {
    return (p.x + rad) * (2 * rad + 1) + (p.y + rad);
}

//returns point from unique code
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

bool can_attack_tank(const Tank& player, const Tank& target, const vector<MapCode>& map_matrix, int rad) {
    if (player.player_id == target.player_id) return false;
    if (player.vehicle_type.name == "spg") {
        return (distance(target.position, player.position) == 3);
    }
    if (player.vehicle_type.name == "light_tank") {
        return (distance(target.position, player.position) == 2);
    }
    if (player.vehicle_type.name == "heavy_tank") {
        return (distance(target.position, player.position) <= 2);
    }
    if (player.vehicle_type.name == "medium_tank") {
        return (distance(target.position, player.position) == 2);
    }
    if (player.vehicle_type.name == "at_spg") {
        if (distance(target.position, player.position) > 3) return false;
        Point buf(player.position.x, player.position.y, player.position.z);
        if (player.position.x == target.position.x) {
            int i = player.position.y - target.position.y;
            bool flag = (i > 0);
            for (i = abs(i); i > 1; --i) {
                (flag) ? --buf.y : ++buf.y;
                (flag) ? ++buf.z : --buf.z;
                if (map_matrix[code(buf, rad)] == MapCode::OBSTACLE) return false;
            }
            return true;
        }
        if (player.position.y == target.position.y) {
            int i = player.position.x - target.position.x;
            bool flag = (i > 0);
            for (i = abs(i); i > 1; --i) {
                (flag) ? --buf.x : ++buf.x;
                (flag) ? ++buf.z : --buf.z;
                if (map_matrix[code(buf, rad)] == MapCode::OBSTACLE) return false;
            }
            return true;
        }
        if (player.position.z == target.position.z) {
            int i = player.position.x - target.position.x;
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
}


bool can_attack(int player, int target, GameState& gm) {
    if (player == target) return false;
    if (find(gm.attack_matrix[target].begin(), gm.attack_matrix[target].end(), player) != gm.attack_matrix[target].end()) {
        return true;
    }
    for (const auto& enemy : gm.players) {
        if (enemy.idx == player || enemy.idx == target) continue;
        if (find(gm.attack_matrix[enemy.idx].begin(), gm.attack_matrix[enemy.idx].end(), target) != gm.attack_matrix[enemy.idx].end()) {
            return false;
        }
    }
    return true;
}


int main()
{
    ClientWG wg;
    wg.start_work();

    Query data;
    string name, game;
    cout << "Name : ";
    cin >> name;
    cout << "Game : ";
    cin >> game;
    PlayerSend pl{ name, "", game, 45, 3, false };
    GameState gm;
    Map mm;
    DataAction action;
    vector<Query> actions(5);
    Timer t;

    //login and initialize player
    data = wg.send_data({ Action::LOGIN, pl });
    cout << data.code << " " << data.json_data << endl;
    Player bot(json::parse(data.json_data));

    data = wg.send_data({ Action::MAP });
    cout << data.code << " " << data.json_data << endl;
    mm.from_json(json::parse(data.json_data));

    int rad = mm.size - 1;
    auto cmp = [](Tank* a, Tank* b) { 
        if (a->capture_points == b->capture_points) return a->health < b->health;
        return a->capture_points > b->capture_points; 
    };
    while (true) {
        try {
            data = wg.send_data({ Action::GAME_STATE });
            gm.from_json(json::parse(data.json_data));
        }
        catch (const exception& e) {
            cout << "We have exception : " << e.what() << endl;
            cout << data.code << " " << data.json_data << endl;
            system("pause");
        }
        if (gm.finished) break;
        if (gm.players.size() == pl.num_players && gm.current_player_idx == bot.idx) {
            cout << "start my work" << endl;

            vector<int> can_be_attacked;
            for (const auto& player : gm.players) {
                if (can_attack(bot.idx, player.idx, gm)) {
                    can_be_attacked.push_back(player.idx);
                }
            }

            vector<MapCode> map_matrix((2 * rad + 1) * (2 * rad + 1), MapCode::NOTHING);
            for (auto it = gm.vehicles.begin(); it != gm.vehicles.end(); ++it) {
                for (int i = 0; i < it->second.size(); ++i) {
                    map_matrix[code(it->second[i].position, rad)] = (it->first == bot.idx) ? MapCode::FRIENDLY_TANK : MapCode::ENEMY_TANK;
                }
            }

            for (int i = 0; i < mm.obstacle.size(); ++i) {
                map_matrix[code(mm.obstacle[i], rad)] = MapCode::OBSTACLE;
            }

            set<Tank*, decltype(cmp)> tanks_can_be_attacked(cmp);
            for (int i = 0; i < can_be_attacked.size(); ++i) {
                for (int j = 0; j < gm.vehicles[can_be_attacked[i]].size(); ++j) {
                    tanks_can_be_attacked.insert(&gm.vehicles[can_be_attacked[i]][j]);
                }
            }
            //vector<Tank*> attack_to_destroy(5, nullptr);

            for (int i = 0; i < gm.vehicles[bot.idx].size(); ++i) {
                if (i == 0) {
                    vector<int> map_dist(map_matrix.size());
                    queue<int> points_move;
                    Point point_to_move;
                    int max_counter = -1;

                    for (int j = 0; j < map_matrix.size(); ++j) {
                        map_dist[j] = (map_matrix[j] == MapCode::NOTHING) ? -1 : INT_MAX;
                    }
                    map_dist[code(gm.vehicles[bot.idx][i].position, rad)] = INT_MAX;
                    points_move.push(code(gm.vehicles[bot.idx][i].position, rad));
                    while (!points_move.empty()) {
                        int curr_code = points_move.front();
                        Point curr_point = decode(curr_code, rad);
                        int counter = 0;
                        points_move.pop();
                        if (distance(curr_point, Point(0, 0, 0)) == 3) ++counter;
                        for (int j = 0; j < near_gex.size(); ++j) {
                            if (distance(curr_point, near_gex[j]) == 4) break;
                            if (distance(curr_point, near_gex[j]) == 3) ++counter;
                        }
                        if (counter >= max_counter) {
                            max_counter = counter;
                            point_to_move = curr_point;
                        }
                        if (max_counter == 3) break;
                        for (int j = 0; j < near_gex.size(); ++j) {
                            Point buf = curr_point + near_gex[j];
                            int buf_code = code(buf, rad);
                            if (can_exist(buf, rad) && map_dist[buf_code] == -1) {
                                map_dist[buf_code] = curr_code;
                                points_move.push(buf_code);
                            }
                        }
                    }
                    if (max_counter == -1) {
                        cout << "I can not find anything, you must remove tanks" << endl;
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
                    }
                    if (max_counter != -1) {
                        cout << " Choosed place to move " << endl;
                        action.vehicle_id = gm.vehicles[bot.idx][i].tank_id;
                        action.target = point_to_move;
                        map_matrix[code(point_to_move, rad)] = MapCode::FRIENDLY_TANK;
                        map_matrix[code(gm.vehicles[bot.idx][i].position, rad)] = MapCode::NOTHING;
                        actions[i] = { Action::MOVE, action };
                    }
                    else {
                        cout << "Try attack by SPG" << endl;
                        Tank* tank_to_attack = nullptr;
                        for (auto victim : tanks_can_be_attacked) {
                            if (can_attack_tank(gm.vehicles[bot.idx][i], *victim, map_matrix, rad)) {
                                tank_to_attack = victim;
                                break;
                            }
                        }
                        if (tank_to_attack == nullptr) {
                            cout << " We can not do anything, we will only wait!" << endl;

                        }
                        else {
                            cout << (json)gm.vehicles[bot.idx][i].position << " attacked " << (json)tank_to_attack->position << endl;
                            action.vehicle_id = gm.vehicles[bot.idx][i].tank_id;
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
                            actions[i] = { Action::SHOOT, action };
                        }
                    }
                }
                /*if (i == 1) {
                    vector<int> map_dist(map_matrix.size());
                    queue<int> points_move;
                    Point point_to_move;
                    int max_counter = -1;

                    for (int j = 0; j < map_matrix.size(); ++j) {
                        map_dist[j] = (map_matrix[j] == MapCode::NOTHING) ? -1 : INT_MAX;
                    }
                    map_dist[code(gm.vehicles[bot.idx][i].position, rad)] = INT_MAX;
                    points_move.push(code(gm.vehicles[bot.idx][i].position, rad));
                    while (!points_move.empty()) {
                        int curr_code = points_move.front();
                        Point curr_point = decode(curr_code, rad);
                        int counter = 0;
                        points_move.pop();
                        if (distance(curr_point, Point(0, 0, 0)) == 3) ++counter;
                        for (int j = 0; j < near_gex.size(); ++j) {
                            if (distance(curr_point, near_gex[j]) == 4) break;
                            if (distance(curr_point, near_gex[j]) == 3) ++counter;
                        }
                        if (counter >= max_counter) {
                            max_counter = counter;
                            point_to_move = curr_point;
                        }
                        if (max_counter == 3) break;
                        for (int j = 0; j < near_gex.size(); ++j) {
                            Point buf = curr_point + near_gex[j];
                            int buf_code = code(buf, rad);
                            if (can_exist(buf, rad) && map_dist[buf_code] == -1) {
                                map_dist[buf_code] = curr_code;
                                points_move.push(buf_code);
                            }
                        }
                    }
                    if (max_counter == -1) {
                        cout << "I can not find anything, you must remove tanks" << endl;
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
                    }
                    if (max_counter != -1) {
                        cout << " Choosed place to move " << endl;
                        action.vehicle_id = gm.vehicles[bot.idx][i].tank_id;
                        action.target = point_to_move;
                        map_matrix[code(point_to_move, rad)] = MapCode::FRIENDLY_TANK;
                        map_matrix[code(gm.vehicles[bot.idx][i].position, rad)] = MapCode::NOTHING;
                        actions[i] = { Action::MOVE, action };
                    }
                    else {
                        cout << "Try attack" << endl;
                        Tank* tank_to_attack = nullptr;
                        for (auto victim : tanks_can_be_attacked) {
                            if (can_attack_tank(gm.vehicles[bot.idx][i], *victim, map_matrix, rad)) {
                                tank_to_attack = victim;
                            }
                        }
                        if (tank_to_attack == nullptr) {
                            cout << " We can not do anything, we will only wait!" << endl;

                        }
                        else {
                            cout << (json)gm.vehicles[bot.idx][i].position << " attacked " << (json)tank_to_attack->position << endl;
                            action.vehicle_id = gm.vehicles[bot.idx][i].tank_id;
                            action.target = tank_to_attack->position;
                            if (tank_to_attack->health == 1) {
                                map_matrix[code(tank_to_attack->position, rad)] = MapCode::NOTHING;
                                tank_to_attack->position.x = 10 * rad;
                                tanks_can_be_attacked.erase(tank_to_attack);
                            }
                            --tank_to_attack->health;
                            actions[i] = { Action::SHOOT, action };
                        }
                    }
                }*/
                if (i == 2) {
                    vector<int> map_dist(map_matrix.size());
                    queue<int> points_move;
                    Point point_to_move(0, 0, 0);
                    bool can_reach_center;
                    bool center_is_occupied = (map_matrix[code(point_to_move, rad)] != MapCode::NOTHING);
                    if (gm.vehicles[bot.idx][i].position == point_to_move) {
                        cout << "Try attack by TT on base" << endl;
                        Tank* tank_to_attack = nullptr;
                        for (auto victim : tanks_can_be_attacked) {
                            if (can_attack_tank(gm.vehicles[bot.idx][i], *victim, map_matrix, rad)) {
                                tank_to_attack = victim;
                                break;
                            }
                        }
                        if (tank_to_attack == nullptr) {
                            cout << " We can not do anything, we will only wait!" << endl;

                        }
                        else {
                            cout << (json)gm.vehicles[bot.idx][i].position << " attacked " << (json)tank_to_attack->position << endl;
                            action.vehicle_id = gm.vehicles[bot.idx][i].tank_id;
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
                            actions[i] = { Action::SHOOT, action };
                        }
                        continue;
                    }
                    if (distance(gm.vehicles[bot.idx][i].position, point_to_move) <= 2 && center_is_occupied){
                        cout << "Try attack by TT on occupied base" << endl;
                        Tank* tank_to_attack = nullptr;
                        for (auto victim : tanks_can_be_attacked) {
                            if (victim->position == point_to_move) {
                                tank_to_attack = victim;
                                break;
                            }
                        }
                        if (tank_to_attack == nullptr) {
                            cout << " We can not do anything, we will only wait!" << endl;

                        }
                        else {
                            cout << (json)gm.vehicles[bot.idx][i].position << " attacked " << (json)tank_to_attack->position << endl;
                            action.vehicle_id = gm.vehicles[bot.idx][i].tank_id;
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
                            actions[i] = { Action::SHOOT, action };
                        }
                        continue;
                    }
                    for (int j = 0; j < map_matrix.size(); ++j) {
                        map_dist[j] = (map_matrix[j] == MapCode::NOTHING) ? -1 : INT_MAX;
                    }
                    map_dist[code(gm.vehicles[bot.idx][i].position, rad)] = INT_MAX;
                    map_dist[code(point_to_move, rad)] = -1;
                    points_move.push(code(gm.vehicles[bot.idx][i].position, rad));
                    while (!points_move.empty()) {
                        int curr_code = points_move.front();
                        Point curr_point = decode(curr_code, rad);
                        points_move.pop();
                        if (curr_point == point_to_move) {
                            can_reach_center = true;
                            break;
                        }
                        for (int j = 0; j < near_gex.size(); ++j) {
                            Point buf = curr_point + near_gex[j];
                            int buf_code = code(buf, rad);
                            if (can_exist(buf, rad) && map_dist[buf_code] == -1) {
                                map_dist[buf_code] = curr_code;
                                points_move.push(buf_code);
                            }
                        }
                    }
                    if (can_reach_center) {
                        cout << "Can reach center by TT" << endl;
                        vector<Point> way;
                        int code_end = code(point_to_move, rad);
                        while (map_dist[code_end] != INT_MAX) {
                            way.push_back(decode(code_end, rad));
                            code_end = map_dist[code_end];
                        }
                        point_to_move = way[way.size() - 1];
                        if (map_matrix[code(point_to_move, rad)] == MapCode::NOTHING) {
                            cout << " Choosed place to move " << endl;
                            action.vehicle_id = gm.vehicles[bot.idx][i].tank_id;
                            action.target = point_to_move;
                            map_matrix[code(point_to_move, rad)] = MapCode::FRIENDLY_TANK;
                            map_matrix[code(gm.vehicles[bot.idx][i].position, rad)] = MapCode::NOTHING;
                            actions[i] = { Action::MOVE, action };
                        }
                        else {
                            cout << "I can not clear base" << endl;
                        }
                    }
                    else {
                        cout << " We can not do anything, we will only wait!" << endl;
                    }
                    
                }
            }
            /*for (auto& bot_tank : gm.vehicles[bot.idx]) {
                Tank* victim = nullptr;
                cout << "try attack" << endl;
                for (const auto& enemy : can_be_attacked) {
                    for (auto& vehicle : gm.vehicles[enemy]) {
                        if (distance(bot_tank.position, vehicle.position) == 2) {
                            if (victim == nullptr
                                || victim->health > vehicle.health
                                || (victim->health == vehicle.health
                                    && victim->vehicle_type.health < vehicle.vehicle_type.health)) {
                                victim = &vehicle;
                            }
                        }
                    }
                }
                if (victim != nullptr) {
                    cout << (json)bot_tank.position << " attacked " << (json)victim->position << endl;
                    action.vehicle_id = bot_tank.tank_id;
                    action.target = victim->position;
                    if (victim->health == 1) {
                        map_matrix[victim->position.x + rad][victim->position.y + rad] = 0;
                        victim->position.x = 10 * rad;
                    }
                    --victim->health;
                    data = { Action::SHOOT, action };
                    cout << data.code << " " << data.json_data << endl;
                    data = wg.send_data(data);
                }
                else if (bot_tank.capture_points == 0) {
                    cout << "try move" << endl;
                    vector<Point> way = find_min_way(bot_tank.position, mm.base, mm.obstacle, rad, map_matrix, gm.vehicles[bot.idx]);
                    cout << "way length : " << way.size() << endl;
                    Point position_to_move;
                    bool need_move = false;
                    for (int i = 0; i < bot_tank.vehicle_type.speed; ++i) {
                        int counter = way.size() - 1 - i;
                        if (counter < 0) break;
                        Point try_to_move = way[counter];
                        if (map_matrix[try_to_move.x + rad][try_to_move.y + rad] == -1) {
                            for (const auto& gex : near_gex) {
                                Point buf = bot_tank.position + gex;
                                int dist = max(max(abs(buf.x), abs(buf.y)), abs(buf.z));
                                if (dist <= rad && map_matrix[buf.x + rad][buf.y + rad] == 0) {
                                    position_to_move = buf;
                                    need_move = true;
                                    break;
                                }
                            }
                        }
                        position_to_move = try_to_move;
                        need_move = true;
                    }
                    if (need_move) {
                        action.vehicle_id = bot_tank.tank_id;
                        action.target = position_to_move;
                        map_matrix[position_to_move.x + rad][position_to_move.y + rad] = -2;
                        map_matrix[bot_tank.position.x + rad][bot_tank.position.y + rad] = 0;
                        data = { Action::MOVE, action };
                        cout << data.code << " " << data.json_data << endl;
                        data = wg.send_data(data);
                    }
                }
            }*/
            //data = wg.send_data({ Action::TURN });
        }
    }
    cout << "Winner : " << gm.winner << endl;
    cout << data.code << " " << data.json_data << endl;
    data = wg.send_data({Action::LOGOUT});
    cout << data.code << " " << data.json_data << endl;
    wg.end_work();
    cout << "Time elapsed: " << t.elapsed() << endl;
    system("pause");
    return 0;
}