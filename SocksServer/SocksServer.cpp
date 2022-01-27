// SocksServer.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#include <iostream>
#include <nlohmann/json.hpp>
#include "ClientWG.h"
#include "JSONParser.h"
#pragma comment (lib, "Ws2_32.lib")

int main()
{
    ClientWG wg;
    Query data;
    PlayerSend pl{"Danik"};
    GameState gm;
    Map mm;
    data.code = Action::LOGIN;
    data.json_data = pl.to_json();
    wg.start_work();
    Sleep(1000);
    wg.send_data(data);
    Sleep(1000);
    std::cout << data.code << " " << data.json_data << std::endl;
    data.code = Action::MAP;
    data.json_data = "";
    wg.send_data(data);
    std::cout << data.code << " " << data.json_data << std::endl;
    mm.from_json(data.json_data);
    Sleep(1000);
    data.code = Action::GAME_STATE;
    data.json_data = "";
    wg.send_data(data);
    std::cout << data.code << " " << data.json_data << std::endl;
    gm.from_json(data.json_data);
    Sleep(1000);
    data.code = Action::LOGOUT;
    data.json_data = "";
    wg.send_data(data);
    std::cout << data.code << " " << data.json_data << std::endl;
    wg.end_work();
    system("pause");
    return 0;
}