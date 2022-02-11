#include "CMakeBot.h"

class Timer {
private:
    using clock_t = std::chrono::high_resolution_clock;
    using second_t = std::chrono::duration<double, std::ratio<1> >;
    std::chrono::time_point<clock_t> m_beg;

public:
    Timer() : m_beg(clock_t::now()) {}

    void reset() {
        m_beg = clock_t::now();
    }

    double elapsed() const {
        return std::chrono::duration_cast<second_t>(clock_t::now() - m_beg).count();
    }
};

using std::string;
using std::vector;
using std::cout;
using std::cin;
using std::map;
using std::set;
using std::queue;
using std::endl;

vector<Point> near_gex{ Point(1, -1,  0), Point(1,  0, -1), Point(0, 1, -1),
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

vector<Point> find_min_way(Point start, const vector<Point>& end, int rad,
    const vector<vector<int>>& map_matrix, const vector<Tank>& friends) {
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
            Point buf = curr + near_gex[i];
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
    int min_d = INT_MAX;
    for (int j = 0; min_d == INT_MAX && j > -3; j--) {
        for (int i = 0; i < end.size(); ++i) {
            if (map_dist[end[i].x + rad][end[i].y + rad].second != -1 && map_matrix[end[i].x + rad][end[i].y + rad] == j &&
                map_dist[end[i].x + rad][end[i].y + rad].second < min_d) {
                min_d = map_dist[end[i].x + rad][end[i].y + rad].second;
                base = end[i];
            }
        }
    }
    if (min_d == INT_MAX) {
        for (int i = 0; i < friends.size(); ++i) {
            if (distance(friends[i].position, Point(0, 0, 0)) == 2 &&
                map_dist[friends[i].position.x + rad][friends[i].position.y + rad].second < min_d) {
                min_d = map_dist[end[i].x + rad][end[i].y + rad].second;
                base = friends[i].position;
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
    string name, game;
    std::cout << "Name : ";
    std::cin >> name;
    std::cout << "Game : ";
    std::cin >> game;
    PlayerSend pl{ name, "", game, 45, 3, false };
    GameState gm;
    Map mm;
    DataAction action;
    Timer t;

    data = wg.send_data({ Action::LOGIN, pl });
    std::cout << data.code << " " << data.json_data << std::endl;
    PlayerGet bot(json::parse(data.json_data));
    data = wg.send_data({ Action::MAP });
    std::cout << data.code << " " << data.json_data << std::endl;
    mm.from_json(json::parse(data.json_data));
    int rad = mm.size - 1;
    while (true) {
        try {
            data = wg.send_data({ Action::GAME_STATE });
            gm.from_json(json::parse(data.json_data));
        }
        catch (const std::exception& e) {
            cout << "We have exception : " << e.what() << endl;
            std::cout << data.code << " " << data.json_data << std::endl;
            system("pause");
        }
        if (gm.finished) break;
        if (gm.players.size() == 3 && gm.current_player_idx == bot.idx) {
            cout << "start my work" << endl;
            vector<int> can_be_attacked;
            for (int i = 0; i < gm.players.size(); i++) {
                if (gm.players[i].idx != bot.idx && can_attack(bot.idx, gm.players[i].idx, gm)) {
                    can_be_attacked.push_back(gm.players[i].idx);
                }
            }
            vector<vector<int>> map_matrix(2 * rad + 1, vector<int>(2 * rad + 1, 0));
            for (auto it : gm.vehicles) {
                for (auto tank : it.second) {
                    map_matrix[tank.position.x + rad][tank.position.y + rad] =
                        tank.player_id == bot.idx ? -2 : -1;
                }
            }
            for (auto bot_tank : gm.vehicles[bot.idx]) {
                Tank* victim = nullptr;
                cout << "try attack" << endl;
                for (int i = 0; i < can_be_attacked.size(); ++i) {
                    for (int j = 0; j < gm.vehicles[can_be_attacked[i]].size(); ++j) {
                        if (distance(bot_tank.position, gm.vehicles[can_be_attacked[i]][j].position) == 2) {
                            if (victim == nullptr
                                || victim->health > gm.vehicles[can_be_attacked[i]][j].health
                                || (victim->health == gm.vehicles[can_be_attacked[i]][j].health
                                    && victim->vehicle_type.health < gm.vehicles[can_be_attacked[i]][j].vehicle_type.health)) {
                                victim = &gm.vehicles[can_be_attacked[i]][j];
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
                    std::cout << data.code << " " << data.json_data << std::endl;
                    data = wg.send_data(data);
                }
                else if (bot_tank.capture_points == 0) {
                    cout << "try move" << endl;
                    vector<Point> way = find_min_way(bot_tank.position, mm.base, rad, map_matrix, gm.vehicles[bot.idx]);
                    cout << "way length : " << way.size() << endl;
                    Point position_to_move;
                    bool need_move = false;
                    for (int i = 0; i < bot_tank.vehicle_type.speed; ++i) {
                        int counter = way.size() - 1 - i;
                        if (counter < 0) break;
                        Point try_to_move = way[counter];
                        if (map_matrix[try_to_move.x + rad][try_to_move.y + rad] == -1) {
                            for (int i = 0; i < near_gex.size(); ++i) {
                                Point buf = bot_tank.position + near_gex[i];
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
                        std::cout << data.code << " " << data.json_data << std::endl;
                        data = wg.send_data(data);
                    }
                }
            }
            data = { Action::TURN };
            data = wg.send_data(data);
        }
    }
    cout << "Winner : " << gm.winner << std::endl;
    std::cout << data.code << " " << data.json_data << std::endl;
    data = wg.send_data({Action::LOGOUT});
    std::cout << data.code << " " << data.json_data << std::endl;
    wg.end_work();
    std::cout << "Time elapsed: " << t.elapsed() << std::endl;
    system("pause");
    return 0;
}