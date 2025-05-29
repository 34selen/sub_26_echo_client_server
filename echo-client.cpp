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
        ssize_t received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (received <= 0)
        {
            printErr("recv");
            break;
        }
        buffer[received] = '\0';
        std::cout << buffer << std::flush;
    }
    std::cout << "\nConnection closed by server\n";
    close(sock);
    std::exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: echo-client <server-ip> <port>\n";
        std::cerr << "Example: echo-client 127.0.0.1 9000\n";
        return EXIT_FAILURE;
    }

    const char *serverIp = argv[1];
    const char *serverPort = argv[2];

    struct addrinfo hints = {}, *info;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(serverIp, serverPort, &hints, &info) != 0)
    {
        printErr("getaddrinfo");
        return EXIT_FAILURE;
    }

    int clientSocket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if (clientSocket < 0)
    {
        printErr("socket");
        freeaddrinfo(info);
        return EXIT_FAILURE;
    }

    if (connect(clientSocket, info->ai_addr, info->ai_addrlen) != 0)
    {
        printErr("connect");
        close(clientSocket);
        freeaddrinfo(info);
        return EXIT_FAILURE;
    }

    freeaddrinfo(info);

    std::thread listenerThread(handleReceive, clientSocket);
    listenerThread.detach();

    std::string input;
    while (std::getline(std::cin, input))
    {
        input += '\n';
        if (send(clientSocket, input.c_str(), input.length(), 0) <= 0)
        {
            printErr("send");
            break;
        }
    }

    close(clientSocket);
    return 0;
}
