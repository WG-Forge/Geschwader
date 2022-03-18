#pragma once

#include <json.hpp>
#include <string>
#include <vector>
#include <set>

#include "Point.h"
#include "utility.h"
#include "ClientWG.h"

using std::string;
using std::set;
using std::vector;
using std::pair;
using nlohmann::json;

enum class TankType {
    SPG = 0,
    LT = 1,
    HT = 2,
    MT = 3,
    AT_SPG = 4,
};

struct Tank {
    TankType type;
    int max_health;
    int speed;
    int player_id;
    int tank_id;
    int health;
    Point spawn_position;
    Point position;
    int capture_points;
    int shoot_range_bonus;

    static bool cmp(const Tank* a, const Tank* b) {
        if (a->capture_points == b->capture_points) {
            if (a->health == b->health) {
                if (a->max_health == b->max_health) {
                    if (a->speed == b->speed)
                        return a->tank_id > b->tank_id;
                    return a->speed > b->speed;
                }
                return a->max_health > b->max_health;
            }
            return a->health < b->health;
        }
        return a->capture_points > b->capture_points;
    };

    static bool cmp2(const pair<int, Tank*> a, const pair<int, Tank*> b) {
        if (a.first == b.first) {
            if (a.second->health == a.second->health) {
                return a.second->tank_id > b.second->tank_id;
            }
            return a.second->health < a.second->health;
        }
        return a.first < b.first;
    };

    Query step(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy);
    virtual bool can_attack(const Tank& target, const vector<MapCode>& map_matrix) = 0;

    Tank() = default;
    Tank(json j, string tank_id_, TankType type, int max_health, int speed);
protected:
    virtual Query try_move(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy) = 0;
    virtual Query try_attack(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy) = 0;
};

struct SPG : public Tank {
    SPG() = default;
	SPG(json j, string id) : Tank(j, id, TankType::SPG, 1, 1) {}

    bool can_attack(const Tank& target, const vector<MapCode>& map_matrix) override;
private:
    Query try_move(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy) override;
    Query try_attack(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy) override;
};

struct AT_SPG : public Tank {
    AT_SPG() = default;
	AT_SPG(json j, string id) : Tank(j, id, TankType::AT_SPG, 2, 1) {}

    bool can_attack(const Tank& target, const vector<MapCode>& map_matrix) override;
private:
    Query try_move(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy) override;
    Query try_attack(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy) override;
};

struct Light_tank : public Tank {
    Light_tank() = default;
	Light_tank(json j, string id) : Tank(j, id, TankType::LT, 1, 3) {}

    bool can_attack(const Tank& target, const vector<MapCode>& map_matrix) override;
private:
    Query try_move(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy) override;
    Query try_attack(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy) override;
};

struct Medium_tank : public Tank {
    Medium_tank() = default;
	Medium_tank(json j, string id) : Tank(j, id, TankType::MT, 2, 2) {}

    bool can_attack(const Tank& target, const vector<MapCode>& map_matrix) override;
private:
    Query try_move(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy) override;
    Query try_attack(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy) override;
};

struct Heavy_tank : public Tank {
    Heavy_tank() = default;
	Heavy_tank(json j, string id) : Tank(j, id, TankType::HT, 3, 1) {}

    bool can_attack(const Tank& target, const vector<MapCode>& map_matrix) override;
private:
    Query try_move(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy) override;
    Query try_attack(vector<MapCode>& map_matrix, set<Tank*, decltype(&Tank::cmp)>& tanks_can_be_attacked, vector<Tank*>& attack_to_destroy) override;
};