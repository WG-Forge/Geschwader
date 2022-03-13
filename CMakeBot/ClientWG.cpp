#include "ClientWG.h"

void ClientWG::start_work() {
    addrinfo *result, *ptr;
    addrinfo hints = {};

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
    }

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    iResult = getaddrinfo(DEFAULT_IP, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
    }

    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (connectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
        }

        iResult = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(connectSocket);
            connectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (connectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
    }
}

Query ClientWG::send_data(const Query& data) {
    Query res;

    uint32_t size = data.json_data.size();

    memcpy((void*)sendbuf, &data.code, sizeof(data.code));
    memcpy((void*)&sendbuf[4], &size, sizeof(size));
    memcpy((void*)&sendbuf[8], data.json_data.c_str(), size);

    iResult = send(connectSocket, sendbuf, size + 2 * sizeof(uint32_t), 0);
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
    }
    //Sleep(200);
    //iResult = recv(connectSocket, recvbuf, DEFAULT_BUFLEN, 0);
    iResult = 0;
    do {
        iResult += receive_data();
    } while (iResult < 8);

    uint32_t need_recv;
    memcpy(&need_recv, recvbuf + 4, 4);
    need_recv += 8;

    while (iResult != need_recv) {
        iResult += receive_data();
    }

    recvbuf[iResult] = 0;
    memcpy(&res.code, recvbuf, 4);
    res.json_data = recvbuf + 8;

    /*if (res.code != Result::OKEY) {
        std::cout << "we have error, check logs" << std::endl;
        std::cout << res.code << " | " << res.json_data << std::endl;
        std::cout << "Need break : ";
        std::string str;
        std::cin >> str;
        if (str == "yes") {
            exit(0);
        }
    }*/
    return res;
}

void ClientWG::end_work() {
    if(connectSocket != INVALID_SOCKET) closesocket(connectSocket);
    WSACleanup();
}
