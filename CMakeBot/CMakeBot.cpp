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

    int rad = Map::get().rad;

    Graphics g(1000, 1000);
    //g.set_active(false);
    //thread render_thread(&Graphics::update, &g);
    while (true) {
        data = wg.send_data({ Action::GAME_STATE });
        GameState::get().update(json::parse(data.json_data));
        if (GameState::get().finished) break;
        if (GameState::get().players.size() == pl.num_players && GameState::get().current_player_idx == bot.idx) {
            g.update();
            cout << "start my work" << endl;
            data = wg.send_data({ Action::MAP });
            Map::get().update(json::parse(data.json_data));

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
            for (int i = 0; i < Map::get().catapult.size(); ++i) {
                map_matrix[code(Map::get().catapult[i], rad)] = MapCode::CATAPULT;
            }


            set<Tank*, decltype(&Tank::cmp)> tanks_can_be_attacked(&Tank::cmp);
            set<pair<int, Tank*>, decltype(&Tank::cmp2)> how_many_can_attack(&Tank::cmp2);

            for (int i = 0; i < can_be_attacked.size(); ++i) {
                for (int j = 0; j < GameState::get().vehicles[can_be_attacked[i]].size(); ++j) {
                    tanks_can_be_attacked.insert(GameState::get().vehicles[can_be_attacked[i]][j].get());
                }
            }
            for (auto& my_tank : GameState::get().vehicles[bot.idx]) {
                int counter = 0;
                for (auto& target : tanks_can_be_attacked) {
                    if (my_tank->can_attack(*target, map_matrix)) ++counter;
                }
                how_many_can_attack.insert(make_pair(counter, my_tank.get()));
            }

            vector<Tank*> attack_to_destroy(5, nullptr);

            for (auto& target : tanks_can_be_attacked) {
                int counter_health = target->health;
                for (int i = 0; i < GameState::get().vehicles[bot.idx].size(); ++i) {
                    if (attack_to_destroy[i]) continue;
                    if (GameState::get().vehicles[bot.idx][i]->can_attack(*target, map_matrix)) {
                        --counter_health;
                    }
                }

                if (counter_health > 0) continue;

                counter_health = target->health;
                for (auto& [points, my_tank] : how_many_can_attack) {
                    if (attack_to_destroy[(int)my_tank->type]) continue;
                    if (my_tank->can_attack(*target, map_matrix)) {
                        attack_to_destroy[(int)my_tank->type] = target;
                        --counter_health;
                    }
                    if (counter_health == 0) break;
                }
            }

            for (const auto& [id, vehicles] : GameState::get().vehicles) {
                for (const auto& vehicle : vehicles) {
                    map_matrix[code(vehicle->position, rad)] = (id == bot.idx) ? MapCode::FRIENDLY_TANK : MapCode::ENEMY_TANK;
                }
            }

            for (auto& curr_tank : GameState::get().vehicles[bot.idx]) {
                data = curr_tank->step(map_matrix, tanks_can_be_attacked, attack_to_destroy);
                if (data.code != -1) wg.send_data(data);
            }
            cout << "********************************************************************************************************************" << endl;
        }
        wg.send_data({ Action::TURN });
    }
    cout << "Winner : " << GameState::get().winner << endl;
    cout << data.code << " " << data.json_data << endl;
    data = wg.send_data({Action::LOGOUT});
    cout << data.code << " " << data.json_data << endl;
    wg.end_work();
    cout << "Time elapsed: " << t.elapsed() << endl;
    system("pause");
    //render_thread.join();
    return 0;
}