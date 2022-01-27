#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#define WIN32_LEAN_AND_MEAN
#define DEFAULT_BUFLEN 9192
#define DEFAULT_PORT "443"
struct Query {
    int code;
    std::string json_data;
};
enum Action
{
    LOGIN = 1,
    LOGOUT = 2,
    MAP = 3,
    GAME_STATE = 4,
    GAME_ACTIONS = 5,
    TURN = 6,
    CHAT = 100,
    MOVE = 101,
    SHOOT = 102
};
enum Result
{
    OKEY = 0,
    BAD_COMMAND = 1,
    ACCESS_DENIED = 2,
    INAPPROPRIATE_GAME_STATE = 3,
    TIMEOUT = 4,
    INTERNAL_SERVER_ERROR = 500
};
class ClientWG
{
public:
    int start_work();
    int send_data(Query& data);
    int end_work();
private:
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    int iResult;
    char sendbuf[200];
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
};

