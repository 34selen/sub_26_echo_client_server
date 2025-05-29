#include <iostream>
#include <thread>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

void printErr(const std::string &context)
{
    std::cerr << "[Error] " << context << ": " << std::strerror(errno) << '\n';
}

void handleReceive(int sock)
{
    constexpr size_t BUFFER_SIZE = 65536;
    char buffer[BUFFER_SIZE];
    std::cout << "Connected to server\n";
    while (true)
    {
        ssize_t received = recv(sock, buffer, BUFFER_SIZE - 1, 0); // 소켓으로 값받기
        if (received <= 0)
        {
            printErr("recv");
            break;
        }
        buffer[received] = '\0';           // 문자열로 처리위한 널
        std::cout << buffer << std::flush; // 화면에 출력
    }
    std::cout << "\nConnection closed by server\n";
    close(sock);
    std::exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 3) // 인자 3개인지 검사
    {
        std::cerr << "Usage: echo-client <server-ip> <port>\n";
        std::cerr << "Example: echo-client 127.0.0.1 9000\n";
        return EXIT_FAILURE;
    }

    const char *serverIp = argv[1];
    const char *serverPort = argv[2];

    struct addrinfo hints = {}, *info;
    hints.ai_family = AF_INET;       // ipv4만
    hints.ai_socktype = SOCK_STREAM; // tcp만만

    if (getaddrinfo(serverIp, serverPort, &hints, &info) != 0) // ip,port가 실제로 있는지 확인인
    {
        printErr("getaddrinfo");
        return EXIT_FAILURE;
    }

    int clientSocket = socket(info->ai_family, info->ai_socktype, info->ai_protocol); // 소켓생성
    if (clientSocket < 0)
    {
        printErr("socket");
        freeaddrinfo(info);
        return EXIT_FAILURE;
    }

    if (connect(clientSocket, info->ai_addr, info->ai_addrlen) != 0) // 연결하고 실패하면 if문문
    {
        printErr("connect");
        close(clientSocket);
        freeaddrinfo(info);
        return EXIT_FAILURE;
    }

    freeaddrinfo(info); // 해제제

    std::thread listenerThread(handleReceive, clientSocket); // 듣는건 새로운 스레드
    listenerThread.detach();

    std::string input;
    while (std::getline(std::cin, input))
    {
        input += '\n';
        if (send(clientSocket, input.c_str(), input.length(), 0) <= 0) // 입력값보내기기
        {
            printErr("send");
            break;
        }
    }

    close(clientSocket);
    return 0;
}
