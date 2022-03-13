#include "JSONParser.h"

using std::make_shared;

PlayerSend::operator json() const{
	json j{};
	j["name"] = name;
	j["password"] = password;
	j["game"] = game;
	j["num_turns"] = num_turns;
	j["num_players"] = num_players;
	j["is_observer"] = is_observer;
	return j;
}

Player::Player(json j) {
	idx = j["idx"].get<int>();
	name = j["name"].get<string>();
	is_observer = j["is_observer"].get<bool>();
}

PlayerSend::PlayerSend(string _name, string _password = "", string _game = "",
	int _num_turns = 45, int _num_players = 1, bool _is_observer = false):
	name(_name), game(_game), password(_password), num_turns(_num_turns),
	num_players(_num_players), is_observer(_is_observer) {}

Point::operator json() const {
	json j{};
	j["x"] = x;
	j["y"] = y;
	j["z"] = z;
	return j;
}

WinPoints::WinPoints(json j) {
	capture = j["capture"].get<int>();
	kill = j["kill"].get<int>();
}
