// SocksServer.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <queue>
#include <utility>
#include <nlohmann/json.hpp>
#include "ClientWG.h"
#include "JSONParser.h"
#pragma comment (lib, "Ws2_32.lib")
using std::string;
using std::vector;
using std::cout;
using std::cin;
using std::map;
using std::set;
using std::queue;
using std::endl;
vector<Point>  near_gex{ Point(1, -1,  0), Point(+1,  0, -1), Point(0, 1, -1),
   Point(-1, 1,  0), Point(-1,  0, 1), Point(0, -1, 1) };
int code(Point p, int rad) {
    return (p.x + rad) * (2 * rad + 1) + (p.y + rad);
}
Point decode(int value, int rad) {
    Point buf;
    buf.x = value / (2 * rad + 1) - rad;
    buf.y = value % (2 * rad + 1) - rad;
    buf.z = 0 - buf.x - buf.y;
    return buf;
}
int distance(const Point& x1, const Point& x2) {
    return max(max(abs(x1.x - x2.x), abs(x1.y - x2.y)), abs(x1.z - x2.z));
}
bool can_attack(int player, int target, GameState& gm) {
    if (std::find(gm.attack_matrix[target].begin(), gm.attack_matrix[target].end(), player) != gm.attack_matrix[target].end()) {
        return true;
    }
    for (int i = 0; i < gm.players.size(); ++i) {
        if (gm.players[i].idx == player || gm.players[i].idx == target) continue;
        if (std::find(gm.attack_matrix[gm.players[i].idx].begin(), gm.attack_matrix[gm.players[i].idx].end(), target) != gm.attack_matrix[gm.players[i].idx].end()) {
            return false;
        }
    }
    return true;
}
vector<Point> find_min_way(Point start, const vector<Point>& end, int rad, const vector<vector<int>>& map_matrix, const vector<Tank>& friends) {
    queue<int> qu;
    vector<vector<std::pair<int, int>>> map_dist(2 * rad + 1, vector<std::pair<int, int>>(2 * rad + 1));
    for (int i = 0; i < 2 * rad + 1; ++i) {
        for (int j = 0; j < 2 * rad + 1; ++j) {
            if (map_matrix[i][j] == -1 || map_matrix[i][j] == -2) {
                map_dist[i][j].first = -1;
            }
            else {
                map_dist[i][j].first = 0;
            }
            map_dist[i][j].second = -1;
        }
    }
    map_dist[start.x + rad][start.y + rad].second = 0;
    qu.push(code(start, rad));
    while (!qu.empty()) {
        Point curr = decode(qu.front(), rad);
        qu.pop();
        for (int i = 0; i < near_gex.size(); ++i) {
            Point buf(curr.x + near_gex[i].x, curr.y + near_gex[i].y, curr.z + near_gex[i].z);
            int dist = max(max(abs(buf.x), abs(buf.y)), abs(buf.z));
            if (dist <= rad) {
                if (map_dist[buf.x + rad][buf.y + rad].first == 0) {
                    map_dist[buf.x + rad][buf.y + rad].first = code(curr, rad);
                    map_dist[buf.x + rad][buf.y + rad].second = map_dist[curr.x + rad][curr.y + rad].second + 1;
                    qu.push(code(buf, rad));
                }
                else if (dist <= 1 && map_dist[buf.x + rad][buf.y + rad].first == -1) {
                    map_dist[buf.x + rad][buf.y + rad].first = code(curr, rad);
                    map_dist[buf.x + rad][buf.y + rad].second = map_dist[curr.x + rad][curr.y + rad].second + 1;
                }
            }
        }
    }
    Point base;
    int min_d = -1;
    for (int i = 0; i < end.size(); ++i) {
        if (map_dist[end[i].x + rad][end[i].y + rad].second != -1 && map_matrix[end[i].x + rad][end[i].y + rad] == 0) {
            if (min_d == -1 || map_dist[end[i].x + rad][end[i].y + rad].second < min_d) {
                min_d = map_dist[end[i].x + rad][end[i].y + rad].second;
                base = end[i];
            }
        }
    }
    if (min_d == -1) {
        for (int i = 0; i < end.size(); ++i) {
            if (map_dist[end[i].x + rad][end[i].y + rad].second != -1 && map_matrix[end[i].x + rad][end[i].y + rad] == -1) {
                if (min_d == -1 || map_dist[end[i].x + rad][end[i].y + rad].second < min_d) {
                    min_d = map_dist[end[i].x + rad][end[i].y + rad].second;
                    base = end[i];
                }
            }
        }
    }
    if (min_d == -1) {
        for (int i = 0; i < end.size(); ++i) {
            if (map_dist[end[i].x + rad][end[i].y + rad].second != -1 && map_matrix[end[i].x + rad][end[i].y + rad] == -2) {
                if (min_d == -1 || map_dist[end[i].x + rad][end[i].y + rad].second < min_d) {
                    min_d = map_dist[end[i].x + rad][end[i].y + rad].second;
                    base = end[i];
                }
            }
        }
    }
    if (min_d == -1) {
        for (int i = 0; i < friends.size(); ++i) {
            if (distance(friends[i].position, Point(0, 0, 0)) == 2) {
                if (min_d == -1 || map_dist[friends[i].position.x + rad][friends[i].position.y + rad].second < min_d) {
                    min_d = map_dist[end[i].x + rad][end[i].y + rad].second;
                    base = friends[i].position;
                }
            }
        }
    }
    vector<Point> way;
    Point buf = base;
    while (map_dist[buf.x + rad][buf.y + rad].second != 0) {
        way.push_back(buf);
        buf = decode(map_dist[buf.x + rad][buf.y + rad].first, rad);
    }
    return way;
}
int main()
{
    ClientWG wg;
    wg.start_work();
    Query data;
    string name;
    std::cout << "Name : ";
    std::cin >> name;
    std::cout << "Game : ";
    string game = "";
    std::cin >> game;
    PlayerSend pl{name, "", game, 45, 3};
    GameState gm;
    Map mm;
    DataAction action;
    data.code = Action::LOGIN;
    data.json_data = pl.to_json();
    std::cout << data.code << " " << data.json_data << std::endl;
    wg.send_data(data);
    std::cout << data.code << " " << data.json_data << std::endl;
    PlayerGet bot(data.json_data);
    data.code = Action::MAP;
    data.json_data = "";
    wg.send_data(data);
    std::cout << data.code << " " << data.json_data << std::endl;
    mm.from_json(data.json_data);
    int rad = mm.size - 1;
    while (true) {
        try {
            data.code = Action::GAME_STATE;
            data.json_data = "";
            wg.send_data(data);
            gm.from_json(data.json_data);
        }
        catch (const std::exception& e) {
            cout << "We have exception : " << e.what() << endl;
            cout << data.json_data.size() << endl;
            std::cout << data.code << " " << data.json_data << std::endl;
            system("pause");
        }
        if (gm.finished) break;
        if (gm.players.size() == 3 && gm.current_player_idx == bot.idx) {
            cout << "start my work" << endl;
            vector<int> can_be_attacked;
            for (int i = 0; i < gm.players.size(); i++) {
                if (gm.players[i].idx == bot.idx) continue;
                if (can_attack(bot.idx, gm.players[i].idx, gm)) {
                    can_be_attacked.push_back(gm.players[i].idx);
                }
            }
            vector<vector<int>> map_matrix(2 * rad + 1, vector<int>(2 * rad + 1, 0));
            for (auto it = gm.vehicles.begin(); it != gm.vehicles.end(); ++it) {
                for (auto tank : it->second) {
                    if (tank.player_id == bot.idx) {
                        map_matrix[tank.position.x + rad][tank.position.y + rad] = -2;
                    }
                    else {
                        map_matrix[tank.position.x + rad][tank.position.y + rad] = -1;
                    }
                }
            }
            for (auto bot_tank : gm.vehicles[bot.idx]) {
                Tank* attack_tank = nullptr;
                cout << "try attack" << endl;
                for (int i = 0; i < can_be_attacked.size(); ++i) {
                    for (int j = 0; j < gm.vehicles[can_be_attacked[i]].size(); ++j) {
                        if (distance(bot_tank.position, gm.vehicles[can_be_attacked[i]][j].position) == 2) {
                            if (attack_tank == nullptr 
                                || attack_tank->health > gm.vehicles[can_be_attacked[i]][j].health
                                || (attack_tank->health == gm.vehicles[can_be_attacked[i]][j].health
                                    && attack_tank->vehicle_type.health < gm.vehicles[can_be_attacked[i]][j].vehicle_type.health)) {
                                attack_tank = &gm.vehicles[can_be_attacked[i]][j];
                            }
                        }
                    }
                }
                if (attack_tank != nullptr) {
                    cout << "good attacked" << endl;
                    cout << "Bot tank : " << bot_tank.position.to_json() << endl;
                    cout << "Attack tank : " << attack_tank->position.to_json() << endl;
                    data.code = Action::SHOOT;
                    action.vehicle_id = bot_tank.tank_id;
                    action.target = attack_tank->position;
                    data.json_data = action.to_json();
                    if (attack_tank->health == 1) {
                        map_matrix[attack_tank->position.x + rad][attack_tank->position.y + rad] = 0;
                        attack_tank->position.x = 10 * rad;
                    }
                    --attack_tank->health;
                    std::cout << data.code << " " << data.json_data << std::endl;
                    wg.send_data(data);
                }
                else {
                    cout << "try move" << endl;
                    if (bot_tank.capture_points != 0) continue;
                    vector<Point> way = find_min_way(bot_tank.position, mm.base, rad, map_matrix, gm.vehicles[bot.idx]);
                    cout << "way has size : " << way.size() << endl;
                    Point position_to_move;
                    bool need_move = false;
                    for (int i = 0; i < bot_tank.vehicle_type.speed; ++i) {
                        int counter = way.size() - 1 - i;
                        if (counter < 0) break;
                        Point try_to_move = way[counter];
                        if (map_matrix[try_to_move.x + rad][try_to_move.y + rad] != 0) {
                            if (map_matrix[try_to_move.x + rad][try_to_move.y + rad] == -1) {
                                for (int i = 0; i < near_gex.size(); ++i) {
                                    Point buf(bot_tank.position.x + near_gex[i].x, bot_tank.position.y + near_gex[i].y, bot_tank.position.z + near_gex[i].z);
                                    int dist = max(max(abs(buf.x), abs(buf.y)), abs(buf.z));
                                    if (dist <= rad && map_matrix[buf.x + rad][buf.y + rad] == 0) {
                                        position_to_move = buf;
                                        need_move = true;
                                        break;
                                    }
                                }
                            }
                        }
                        position_to_move = try_to_move;
                        need_move = true;
                    }
                    if (need_move) {
                        data.code = Action::MOVE;
                        action.vehicle_id = bot_tank.tank_id;
                        action.target = position_to_move;
                        data.json_data = action.to_json();
                        map_matrix[position_to_move.x + rad][position_to_move.y + rad] = -2;
                        map_matrix[bot_tank.position.x + rad][bot_tank.position.y + rad] = 0;
                        std::cout << data.code << " " << data.json_data << std::endl;
                        wg.send_data(data);
                    }
                }
            }
            data.code = Action::TURN;
            data.json_data = "";
            std::cout << data.code << " " << data.json_data << std::endl;
            wg.send_data(data);
        }
    }
    cout << "Winner : " << gm.winner << std::endl;
    std::cout << data.code << " " << data.json_data << std::endl;
    data.code = Action::LOGOUT;
    data.json_data = "";
    wg.send_data(data);
    std::cout << data.code << " " << data.json_data << std::endl;
    wg.end_work();
    system("pause");
    return 0;
}