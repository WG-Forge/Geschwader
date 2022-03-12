#include "CMakeBot.h"

using namespace std;

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

bool is_enemy_neutral(int player, int target) {
    if (player == target) return false;
    if (find(GameState::get().attack_matrix[target].begin(), GameState::get().attack_matrix[target].end(), player) != GameState::get().attack_matrix[target].end()) {
        return true;
    }
    for (const auto& enemy : GameState::get().players) {
        if (enemy.idx == player || enemy.idx == target) continue;
        if (find(GameState::get().attack_matrix[enemy.idx].begin(), GameState::get().attack_matrix[enemy.idx].end(), target) != GameState::get().attack_matrix[enemy.idx].end()) {
            return false;
        }
    }
    return true;
}

int safe_index(Point ps, GameState& gm, int rad, const vector<MapCode>& map_matrix, int idx, const Map& mm) {
    Tank buf;
    buf.position = ps;
    int counter = 0;
    for (int i = 0; i < gm.players.size(); ++i) {
        if (gm.players[i].idx != idx) {
            for (auto& attacker : gm.vehicles[gm.players[i].idx]) {
                bool is_mod = false;
                for (int i = 0; i < mm.catapult.size(); ++i) {
                    if (attacker.position == mm.catapult[i]) {
                        is_mod = true;
                        break;
                    }
                }
                if (can_attack_tank(attacker, buf, map_matrix, rad, is_mod)) ++counter;
            }
        }
    }
    return counter;
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
    PlayerSend pl{ name, "", game, 45, 1, false };
    DataAction action;
    Timer t;

    //login and initialize player
    data = wg.send_data({ Action::LOGIN, pl });
    cout << data.code << " " << data.json_data << endl;
    Player bot(json::parse(data.json_data));

    data = wg.send_data({ Action::MAP });
    cout << data.code << " " << data.json_data << endl;
    Map::get().update(json::parse(data.json_data));

    int rad = mm.size - 1;

    auto cmp = [](Tank* a, Tank* b) { 
        if (a->capture_points == b->capture_points) return a->health < b->health;
        return a->capture_points > b->capture_points; 
    };
    auto cmp2 = [](pair<int, Tank*> a, pair<int, Tank*> b) {
        if (a.first == b.first) return a.second->health < a.second->health;
        return a.first < b.first;
    };

    auto cmp3 = [](pair<int, int> a, pair<int, int> b) {
        return a.first < b.first;
    };

    int rad = Map::get().size - 1;
    Graphics g(1000, 1000);
    g.set_active(false);
    thread render_thread(&Graphics::update, &g);
    while (true) {
        data = wg.send_data({ Action::GAME_STATE });
        gm.from_json(json::parse(data.json_data));
        if (gm.finished) break;
        if (gm.players.size() == pl.num_players && gm.current_player_idx == bot.idx) {
        try {
            data = wg.send_data({ Action::GAME_STATE });
            GameState::get().update(json::parse(data.json_data));
        }
        catch (const exception& e) {
            cout << "We have exception : " << e.what() << endl;
            cout << data.code << " " << data.json_data << endl;
            system("pause");
        }
        if (GameState::get().finished) break;
        if (GameState::get().players.size() == pl.num_players && GameState::get().current_player_idx == bot.idx) {
            cout << "start my work" << endl;
            data = wg.send_data({ Action::MAP });
            mm.from_json(json::parse(data.json_data));

            vector<int> can_be_attacked;
            for (const auto& player : GameState::get().players) {
                if (is_enemy_neutral(bot.idx, player.idx)) {
                    can_be_attacked.push_back(player.idx);
                }
            }

            vector<MapCode> map_matrix((2 * rad + 1) * (2 * rad + 1), MapCode::NOTHING);

            for (int i = 0; i < Map::get().obstacle.size(); ++i) {
                map_matrix[code(Map::get().obstacle[i], rad)] = MapCode::OBSTACLE;
            }

            for (int i = 0; i < mm.catapult.size(); ++i) {
                map_matrix[code(mm.catapult[i], rad)] = MapCode::CATAPULT;
            }


            set<Tank*, decltype(cmp)> tanks_can_be_attacked(cmp);
            set<pair<int, Tank*>, decltype(cmp2)> how_many_can_attack(cmp2);

            for (int i = 0; i < can_be_attacked.size(); ++i) {
                for (int j = 0; j < GameState::get().vehicles[can_be_attacked[i]].size(); ++j) {
                    tanks_can_be_attacked.insert(GameState::get().vehicles[can_be_attacked[i]][j].get());
                }
            }
            for (auto& my_tank : gm.vehicles[bot.idx]) {
                int counter = 0;
                bool is_mod = (map_matrix[code(my_tank.position, rad)] == MapCode::CATAPULT);
                for (auto& target : tanks_can_be_attacked) {
                    if (can_attack_tank(my_tank, *target, map_matrix, rad, is_mod)) ++counter;
                }
                how_many_can_attack.insert(make_pair(counter, &my_tank));
            }

            vector<Tank*> attack_to_destroy(5, nullptr);

            for (auto& target : tanks_can_be_attacked) {
                int counter_health = target->health;
                for (int i = 0; i < gm.vehicles[bot.idx].size(); ++i) {
                    if (attack_to_destroy[i]) continue;
                    bool is_mod = (map_matrix[code(gm.vehicles[bot.idx][i].position, rad)] == MapCode::CATAPULT);
                    if (can_attack_tank(gm.vehicles[bot.idx][i], *target, map_matrix, rad, is_mod)) {
                        --counter_health;
                    }
                }

                if (counter_health > 0) continue;

                counter_health = target->health;
                for (auto& [points, my_tank] : how_many_can_attack) {
                    if (attack_to_destroy[my_tank->vehicle_type.name]) continue;
                    bool is_mod = (map_matrix[code(my_tank->position, rad)] == MapCode::CATAPULT);
                    if (can_attack_tank(*my_tank, *target, map_matrix, rad, is_mod)) {
                        attack_to_destroy[my_tank->vehicle_type.name] = target;
                        --counter_health;
                    }
                    if (counter_health == 0) break;
                }
            }

            for (auto it = gm.vehicles.begin(); it != gm.vehicles.end(); ++it) {
                for (int i = 0; i < it->second.size(); ++i) {
                    map_matrix[code(it->second[i].position, rad)] = (it->first == bot.idx) ? MapCode::FRIENDLY_TANK : MapCode::ENEMY_TANK;
                }
            }

            for (auto& curr_tank : gm.vehicles[bot.idx]) {
                if (curr_tank.vehicle_type.name == 0) { // SPG tank logic
                    if (attack_to_destroy[0]) {
                        //shoot
                        cout << (json)curr_tank.position << " attacked " << (json)attack_to_destroy[0]->position << endl;
                        action.vehicle_id = curr_tank.tank_id;
                        action.target = attack_to_destroy[0]->position;
                        tanks_can_be_attacked.erase(attack_to_destroy[0]);
                        --attack_to_destroy[0]->health;
                        if (attack_to_destroy[0]->health == 0) {
                            map_matrix[code(attack_to_destroy[0]->position, rad)] = MapCode::NOTHING;
                            for (int i = 0; i < mm.catapult.size(); ++i) {
                                if (attack_to_destroy[0]->position == mm.catapult[i]) {
                                    map_matrix[code(mm.catapult[i], rad)] = MapCode::CATAPULT;
                                    break;
                                }
                            }
                            attack_to_destroy[0]->position.x = 10 * rad;
                        }
                        else {
                            tanks_can_be_attacked.insert(attack_to_destroy[0]);
                        }
                        wg.send_data({ Action::SHOOT, action });
                        continue;
                    }

                    vector<int> map_dist(map_matrix.size(), -1);
                    queue<int> points_move;
                    Point point_to_move;
                    int max_counter = -1;
                    int code_my_tank = code(curr_tank.position, rad);

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
                        bool is_mod = (map_matrix[curr_code] == MapCode::CATAPULT);

                        if (distance(curr_point, Point(0, 0, 0)) == 3 || distance(curr_point, Point(0, 0, 0)) == (3 + is_mod)) ++counter;
                        for (int j = 0; j < near_gex.size(); ++j) {
                            if (distance(curr_point, near_gex[j]) == 3 || distance(curr_point, near_gex[j]) == (3 + is_mod)) ++counter;
                        }
                        if (counter > max_counter) {
                            max_counter = counter;
                            point_to_move = curr_point;
                        }
                        if (max_counter == 3) break;

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

                    if (curr_tank.position == point_to_move) {
                        // we on place, we shouldnt do anything.
                        continue;
                    }

                    cout << "Max counter for SPG" << endl;
                    cout << "MAX Counter : " << max_counter << endl;

                    if (max_counter == -1) {
                        // safe your tank
                        max_counter = 11;
                        int dist = 2 * rad;

                        for (int j = 0; j < near_gex.size(); ++j) {
                            Point buf = curr_tank.position + near_gex[j];
                            int buf_code = code(buf, rad);
                            if (can_exist(buf, rad) && (map_matrix[buf_code] != MapCode::FRIENDLY_TANK || map_matrix[buf_code] != MapCode::ENEMY_TANK || map_matrix[buf_code] != MapCode::OBSTACLE)) {
                                int index = safe_index(buf, gm, rad, map_matrix, bot.idx, mm);
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
                    action.vehicle_id = curr_tank.tank_id;
                    action.target = point_to_move;
                    map_matrix[code(point_to_move, rad)] = MapCode::FRIENDLY_TANK;
                    map_matrix[code_my_tank] = MapCode::NOTHING;
                    for (int i = 0; i < mm.catapult.size(); ++i) {
                        if (curr_tank.position == mm.catapult[i]) {
                            map_matrix[code_my_tank] = MapCode::CATAPULT;
                            break;
                        }
                    }
                    wg.send_data({ Action::MOVE, action });

                }
                if (curr_tank.vehicle_type.name == 1) { // LT logic
                    if (attack_to_destroy[1]) {
                        //shoot
                        cout << (json)curr_tank.position << " attacked " << (json)attack_to_destroy[1]->position << endl;
                        action.vehicle_id = curr_tank.tank_id;
                        action.target = attack_to_destroy[1]->position;
                        tanks_can_be_attacked.erase(attack_to_destroy[1]);
                        --attack_to_destroy[1]->health;
                        if (attack_to_destroy[1]->health == 0) {
                            map_matrix[code(attack_to_destroy[1]->position, rad)] = MapCode::NOTHING;
                            for (int i = 0; i < mm.catapult.size(); ++i) {
                                if (attack_to_destroy[1]->position == mm.catapult[i]) {
                                    map_matrix[code(mm.catapult[i], rad)] = MapCode::CATAPULT;
                                    break;
                                }
                            }
                            attack_to_destroy[1]->position.x = 10 * rad;
                        }
                        else {
                            tanks_can_be_attacked.insert(attack_to_destroy[1]);
                        }
                        wg.send_data({ Action::SHOOT, action });
                        continue;
                    }

                    int code_my_tank = code(curr_tank.position, rad);
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
                                    }
                                }
                                if (tank_to_kill) break;
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
                        cout << "I can no find tank to kill, so i wait)" << endl;
                        continue;
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
                    bool can_move = false;
                    int code_end = code(point_to_move, rad);
                    while (map_dist[code_end] != INT_MAX) {
                        way.push_back(decode(code_end, rad));
                        code_end = map_dist[code_end];
                    }
                    cout << "Way size for LT : " << way.size() << endl;
                    if (way.size() == 1) {
                        point_to_move = way[0];
                        can_move = true;
                    }
                    if (way.size() == 2) {
                        point_to_move = way[0];
                        can_move = true;
                    }

                    if (way.size() >= 3) {
                        point_to_move = way[way.size() - 3];
                        can_move = true;
                    }

                    if (!can_move) {
                        cout << "I can not move my LT, only wait" << endl;
                    }

                    if (can_move) {
                        cout << " Choosed place to move by LT" << endl;
                        action.vehicle_id = curr_tank.tank_id;
                        action.target = point_to_move;
                        map_matrix[code(point_to_move, rad)] = MapCode::FRIENDLY_TANK;
                        map_matrix[code_my_tank] = MapCode::NOTHING;
                        for (int i = 0; i < mm.catapult.size(); ++i) {
                            if (curr_tank.position == mm.catapult[i]) {
                                map_matrix[code_my_tank] = MapCode::CATAPULT;
                                break;
                            }
                        }
                        wg.send_data({ Action::MOVE, action });
                    }
                }
                if (curr_tank.vehicle_type.name == 2) { // TT tank logic
                    if (attack_to_destroy[2]) {
                        //shoot
                        cout << (json)curr_tank.position << " attacked " << (json)attack_to_destroy[2]->position << endl;
                        action.vehicle_id = curr_tank.tank_id;
                        action.target = attack_to_destroy[2]->position;
                        tanks_can_be_attacked.erase(attack_to_destroy[2]);
                        --attack_to_destroy[2]->health;
                        if (attack_to_destroy[2]->health == 0) {
                            map_matrix[code(attack_to_destroy[2]->position, rad)] = MapCode::NOTHING;
                            for (int i = 0; i < mm.catapult.size(); ++i) {
                                if (attack_to_destroy[2]->position == mm.catapult[i]) {
                                    map_matrix[code(mm.catapult[2], rad)] = MapCode::CATAPULT;
                                    break;
                                }
                            }
                            attack_to_destroy[2]->position.x = 10 * rad;
                        }
                        else {
                            tanks_can_be_attacked.insert(attack_to_destroy[2]);
                        }
                        wg.send_data({ Action::SHOOT, action });
                        continue;
                    }

                    vector<int> map_dist(map_matrix.size(), -1);
                    queue<int> points_move;
                    int code_my_tank = code(curr_tank.position, rad);
                    Point point_to_move(0, 0, 0);
                    Point point_to_move_if_occupied;
                    bool can_reach_center = false;
                    int dist_to_center = 12;

                    if (curr_tank.position == point_to_move) {
                        // we on place, only wait
                        continue;
                    }

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

                    
                    if (can_reach_center) {
                        cout << "Can reach center by TT" << endl;
                        vector<Point> way;
                        int code_end = code(point_to_move, rad);
                        while (map_dist[code_end] != INT_MAX) {
                            way.push_back(decode(code_end, rad));
                            code_end = map_dist[code_end];
                        }
                        point_to_move = way[way.size() - 1];
                    }
                    else {
                        cout << "Can reach only near gex by TT" << endl;
                        vector<Point> way;
                        int code_end = code(point_to_move_if_occupied, rad);
                        while (map_dist[code_end] != INT_MAX) {
                            way.push_back(decode(code_end, rad));
                            code_end = map_dist[code_end];
                        }
                        point_to_move = way[way.size() - 1];
                    }

                    if (!(point_to_move == curr_tank.position)) {
                        cout << " Choosed place to move by TT" << endl;
                        action.vehicle_id = curr_tank.tank_id;
                        action.target = point_to_move;
                        map_matrix[code(point_to_move, rad)] = MapCode::FRIENDLY_TANK;
                        map_matrix[code_my_tank] = MapCode::NOTHING;
                        for (int i = 0; i < mm.catapult.size(); ++i) {
                            if (curr_tank.position == mm.catapult[i]) {
                                map_matrix[code_my_tank] = MapCode::CATAPULT;
                                break;
                            }
                        }
                        wg.send_data({ Action::MOVE, action });
                    }
                    
                }
                if (curr_tank.vehicle_type.name == 3) { // ST tank logic
                    if (attack_to_destroy[3]) {
                        //shoot
                        cout << (json)curr_tank.position << " attacked " << (json)attack_to_destroy[3]->position << endl;
                        action.vehicle_id = curr_tank.tank_id;
                        action.target = attack_to_destroy[3]->position;
                        tanks_can_be_attacked.erase(attack_to_destroy[3]);
                        --attack_to_destroy[3]->health;
                        if (attack_to_destroy[3]->health == 0) {
                            map_matrix[code(attack_to_destroy[3]->position, rad)] = MapCode::NOTHING;
                            for (int i = 0; i < mm.catapult.size(); ++i) {
                                if (attack_to_destroy[3]->position == mm.catapult[i]) {
                                    map_matrix[code(mm.catapult[3], rad)] = MapCode::CATAPULT;
                                    break;
                                }
                            }
                            attack_to_destroy[3]->position.x = 10 * rad;
                        }
                        else {
                            tanks_can_be_attacked.insert(attack_to_destroy[3]);
                        }
                        wg.send_data({ Action::SHOOT, action });
                        continue;
                    }

                    vector<int> map_dist(map_matrix.size(), -1);
                    queue<pair<int, int>> points_move;
                    int code_my_tank = code(curr_tank.position, rad);
                    Point point_to_move;
                    int dist_to_center = 12;

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

                        if (dist < dist_to_center && map_matrix[curr_code]!= MapCode::ENEMY_TANK && map_matrix[curr_code] != MapCode::FRIENDLY_TANK) {
                            point_to_move = curr_point;
                            dist_to_center = dist;
                        }

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
                    }

                    if (can_move) {
                        cout << " Choosed place to move by ST" << endl;
                        action.vehicle_id = curr_tank.tank_id;
                        action.target = point_to_move;
                        map_matrix[code(point_to_move, rad)] = MapCode::FRIENDLY_TANK;
                        map_matrix[code_my_tank] = MapCode::NOTHING;
                        for (int i = 0; i < mm.catapult.size(); ++i) {
                            if (curr_tank.position == mm.catapult[i]) {
                                map_matrix[code_my_tank] = MapCode::CATAPULT;
                                break;
                            }
                        }
                        wg.send_data({ Action::MOVE, action });
                    }

                }
                if (curr_tank.vehicle_type.name == 4) { // SPG_AT tank logic
                    if (attack_to_destroy[4]) {
                        //shoot
                        cout << (json)curr_tank.position << " attacked " << (json)attack_to_destroy[4]->position << endl;
                        action.vehicle_id = curr_tank.tank_id;
                        action.target = attack_to_destroy[4]->position;
                        tanks_can_be_attacked.erase(attack_to_destroy[4]);
                        --attack_to_destroy[4]->health;
                        if (attack_to_destroy[4]->health == 0) {
                            map_matrix[code(attack_to_destroy[4]->position, rad)] = MapCode::NOTHING;
                            for (int i = 0; i < mm.catapult.size(); ++i) {
                                if (attack_to_destroy[4]->position == mm.catapult[i]) {
                                    map_matrix[code(mm.catapult[i], rad)] = MapCode::CATAPULT;
                                    break;
                                }
                            }
                            attack_to_destroy[4]->position.x = 10 * rad;
                        }
                        else {
                            tanks_can_be_attacked.insert(attack_to_destroy[4]);
                        }
                        wg.send_data({ Action::SHOOT, action });
                        continue;
                    }
                    cout << "start AT SPG " << endl;
                    cout << " No shoot" << endl;
                    vector<int> map_dist(map_matrix.size(), -1);
                    queue<int> points_move;
                    Point point_to_move;
                    int max_counter = -1;
                    int code_my_tank = code(curr_tank.position, rad);

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
                        bool is_mod = (map_matrix[curr_code] == MapCode::CATAPULT);
                        Tank buf_attacker;
                        buf_attacker.position = curr_point;
                        buf_attacker.player_id = 1;
                        buf_attacker.vehicle_type.name = Tanks::AT_SPG;
                        Tank buf_base;
                        buf_base.position = Point(0, 0, 0);
                        buf_base.player_id = 2;

                        if (can_attack_tank(buf_attacker, buf_base, map_matrix, rad, is_mod)) ++counter;
                        for (int j = 0; j < near_gex.size(); ++j) {
                            buf_base.position = near_gex[j];
                            if (can_attack_tank(buf_attacker, buf_base, map_matrix, rad, is_mod)) ++counter;
                        }

                        if (counter > max_counter) {
                            max_counter = counter;
                            point_to_move = curr_point;
                        }
                        if (max_counter == 6) break;

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

                    if (curr_tank.position == point_to_move) {
                        // we on place, we shouldnt do anything.
                        continue;
                    }

                    cout << "MAX Counter : " << max_counter << endl;

                    if (max_counter == -1) {
                        // safe your tank
                        max_counter = 11;
                        int dist = 2 * rad;

                        for (int j = 0; j < near_gex.size(); ++j) {
                            Point buf = curr_tank.position + near_gex[j];
                            int buf_code = code(buf, rad);
                            if (can_exist(buf, rad)) {
                                int index = safe_index(buf, gm, rad, map_matrix, bot.idx, mm);
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
                    action.vehicle_id = curr_tank.tank_id;
                    action.target = point_to_move;
                    map_matrix[code(point_to_move, rad)] = MapCode::FRIENDLY_TANK;
                    map_matrix[code_my_tank] = MapCode::NOTHING;
                    for (int i = 0; i < mm.catapult.size(); ++i) {
                        if (curr_tank.position == mm.catapult[i]) {
                            map_matrix[code_my_tank] = MapCode::CATAPULT;
                            break;
                        }
                    }
                    wg.send_data({ Action::MOVE, action });
                }
            }
            cout << "********************************************************************************************************************" << endl;
            wg.send_data({ Action::TURN });
        }
    }
    cout << "Winner : " << GameState::get().winner << endl;
    cout << data.code << " " << data.json_data << endl;
    data = wg.send_data({Action::LOGOUT});
    cout << data.code << " " << data.json_data << endl;
    wg.end_work();
    cout << "Time elapsed: " << t.elapsed() << endl;
    system("pause");
    render_thread.join();
    return 0;
}