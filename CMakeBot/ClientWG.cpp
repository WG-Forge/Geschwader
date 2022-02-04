#include "ClientWG.h"
int ClientWG::start_work() {
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    iResult = getaddrinfo("92.223.34.102", "443", &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }
    freeaddrinfo(result);
    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }
    return 0;
}
int ClientWG::send_data(Query& data) {
    int size = data.json_data.size();
    memcpy((void*)sendbuf, &data.code, sizeof(data.code));
    memcpy((void*)&sendbuf[4], &size, sizeof(size));
    memcpy((void*)&sendbuf[8], data.json_data.c_str(), size);
    iResult = send(ConnectSocket, sendbuf, size + 2 * sizeof(int), 0);
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }
    //printf("Bytes Sent: %ld\n", iResult);
    Sleep(300);
    iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
    if (iResult > 0) {
        //printf("Bytes received: %d\n", iResult);
        recvbuf[iResult] = 0;
        memcpy(&data.code, recvbuf, sizeof(int));
        data.json_data = recvbuf + 8;
        if (data.code != Result::OKEY) {
            std::cout << "we have error, check logs" << std::endl;
            std::cout << data.code << " | " << data.json_data << std::endl;
            std::cout << "Need break : ";
            std::string str;
            std::cin >> str;
            if (str == "yes") {
                exit(0);
            }
        }
    }
    else if (iResult == 0)
        printf("Connection closed\n");
    else
        printf("recv failed with error: %d\n", WSAGetLastError());
    Sleep(200);
    return 0;
}
int ClientWG::end_work() {
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }
    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}
