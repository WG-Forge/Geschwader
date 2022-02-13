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

const vector<Point> near_gex{ Point(1, -1,  0), Point(1,  0, -1), Point(0, 1, -1),
   Point(-1, 1,  0), Point(-1,  0, 1), Point(0, -1, 1) };

//assigns unique index for point
int code(Point p, int rad) {
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

int distance(const Point& x1, const Point& x2) {
    return max(max(abs(x1.x - x2.x), abs(x1.y - x2.y)), abs(x1.z - x2.z));
}

bool can_attack(int player, int target, GameState& gm) {
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

vector<Point> find_min_way(Point start, const vector<Point>& end, vector<Point>& obstacles,
    int rad, const vector<vector<int>>& map_matrix, const vector<Tank>& friends) {
    vector<pair<int, int>> qu;

    //matrix that holds point's code and distance from start
    vector<vector<pair<int, int>>> map_dist(2 * rad + 1, vector<pair<int, int>>(2 * rad + 1, { 0, INT_MAX }));

    for (int i = 0; i < 2 * rad + 1; ++i) {
        for (int j = 0; j < 2 * rad + 1; ++j) {
            if (map_matrix[i][j] < 0) map_dist[i][j].first = -1;
        }
    }

    map_dist[start.x + rad][start.y + rad].second = 0;
    qu.push_back({ 0,code(start, rad) });

    while (!qu.empty()) {
        Point curr = decode(qu.front().second, rad);
        qu.erase(qu.begin());
        int currDist = distance(curr, end[0]);
        for (const auto& gex : near_gex) {
            Point buf = curr + gex;
            int dist = distance(buf, end[0]);
            //if buf isn't an obstacle and closer to end add buf to queue for further processing
            //in case if point is target it won't be processed
            if (dist <= currDist && find(obstacles.begin(), obstacles.end(), buf) == obstacles.end()) {
                if (map_dist[buf.x + rad][buf.y + rad].first == 0) {
                    map_dist[buf.x + rad][buf.y + rad].first = code(curr, rad);
                    map_dist[buf.x + rad][buf.y + rad].second = map_dist[curr.x + rad][curr.y + rad].second + 1;
                    qu.push_back({ map_dist[buf.x + rad][buf.y + rad].second, code(buf, rad) });
                }
                else if (dist <= 0 && map_dist[buf.x + rad][buf.y + rad].first == -1) {
                    map_dist[buf.x + rad][buf.y + rad].first = code(curr, rad);
                    map_dist[buf.x + rad][buf.y + rad].second = map_dist[curr.x + rad][curr.y + rad].second + 1;
                }
            }
        }
        //sort points to prioritize ones which are closer to end
        sort(qu.begin(), qu.end(), [](const pair<int, int>& a, const pair<int, int>& b) {return a.first < b.first; });

        if (qu.size() != 0 && qu[0].first <= 0) break;
    }

    //find end point closest to start
    Point base;
    int min_d = INT_MAX;
    for (const auto& point : end) {
        if (map_matrix[point.x + rad][point.y + rad] == 0 && map_dist[point.x + rad][point.y + rad].second < min_d) {
            min_d = map_dist[point.x + rad][point.y + rad].second;
            base = point;
        }
    }
    if (min_d == INT_MAX) {
        for (int i = 0; i < friends.size(); ++i) {
            if (distance(friends[i].position, end[0]) == 2 &&
                map_dist[friends[i].position.x + rad][friends[i].position.y + rad].second < min_d) {
                min_d = map_dist[end[i].x + rad][end[i].y + rad].second;
                base = friends[i].position;
            }
        }
    }

    //select path to end
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
    cout << "Name : ";
    cin >> name;
    cout << "Game : ";
    cin >> game;
    PlayerSend pl{ name, "", game, 45, 3, false };
    GameState gm;
    Map mm;
    DataAction action;
    Timer t;

    //login and initialize player
    data = wg.send_data({ Action::LOGIN, pl });
    cout << data.code << " " << data.json_data << endl;
    Player bot(json::parse(data.json_data));

    data = wg.send_data({ Action::MAP });
    cout << data.code << " " << data.json_data << endl;
    mm.from_json(json::parse(data.json_data));

    int rad = mm.size - 1;

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
                if (player.idx != bot.idx && can_attack(bot.idx, player.idx, gm)) {
                    can_be_attacked.push_back(player.idx);
                }
            }

            vector<vector<int>> map_matrix(2 * rad + 1, vector<int>(2 * rad + 1, 0));
            for (auto vehicles : gm.vehicles) {
                for (auto tank : vehicles.second) {
                    map_matrix[tank.position.x + rad][tank.position.y + rad] =
                        tank.player_id == bot.idx ? -2 : -1;
                }
            }

            for (auto& bot_tank : gm.vehicles[bot.idx]) {
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
            }
            data = wg.send_data({ Action::TURN });
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