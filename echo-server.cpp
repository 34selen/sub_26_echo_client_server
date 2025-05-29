#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>
#include <atomic>

bool echo = false;
bool broadcast = false;
uint16_t port = 0;
std::vector<std::pair<int, int>> clients;
std::mutex mtx;
std::atomic<int> nextId{0};

void myerror(const char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, std::strerror(errno));
}

bool parse(int argc, char *argv[])
{
    if (argc < 2 || argc > 4)
        return false;
    bool gotPort = false;
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i][0] == '-')
        {
            for (size_t j = 1; j < strlen(argv[i]); ++j)
            {
                char c = argv[i][j];
                if (c == 'e')
                    echo = true;
                else if (c == 'b')
                    broadcast = true;
                else
                    return false;
            }
        }
        else if (!gotPort)
        {
            port = std::atoi(argv[i]);
            gotPort = true;
        }
        else
        {
            return false;
        }
    }
    if (!gotPort || (broadcast && !echo))
        return false;
    return true;
}

void client_handler(int sd)
{
    int myId = nextId.fetch_add(1) + 1;
    {
        std::lock_guard<std::mutex> lock(mtx);
        clients.emplace_back(sd, myId);
    }
    printf("Client %d connected\n", myId);

    constexpr int BUFSIZE = 65536;
    char buf[BUFSIZE];
    while (true)
    {
        ssize_t r = recv(sd, buf, BUFSIZE - 1, 0);
        if (r <= 0)
        {
            myerror("recv");
            break;
        }
        buf[r] = '\0';
        printf("Received from Client %d: %.*s", myId, (int)r, buf);

        if (echo && !broadcast)
        {
            if (send(sd, buf, r, 0) <= 0)
                myerror("send");
            printf("Echo: %.*s", (int)r, buf);
        }
        else if (broadcast)
        {
            std::lock_guard<std::mutex> lock(mtx);
            for (auto &p : clients)
            {
                if (send(p.first, buf, r, 0) <= 0)
                    myerror("send");
            }
            printf("Broadcast: %.*s", (int)r, buf);
        }
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        clients.erase(std::remove_if(clients.begin(), clients.end(),
                                     [sd](const std::pair<int, int> &p)
                                     { return p.first == sd; }),
                      clients.end());
    }
    close(sd);
    printf("Client %d disconnected\n", myId);
}

int main(int argc, char *argv[])
{
    if (!parse(argc, argv))
    {
        printf("syntax: echo-server <port> [-e[-b]]\n");
        printf("sample: echo-server 1234 -e -b \n");
        return 1;
    }

    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd == -1)
    {
        myerror("socket");
        return 1;
    }
    int opt = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sd, (sockaddr *)&addr, sizeof(addr)) == -1)
    {
        myerror("bind");
        return 1;
    }
    if (listen(sd, 5) == -1)
    {
        myerror("listen");
        return 1;
    }
    printf("echo-server started on port %u (echo=%s, broadcast=%s)\n", port, echo ? "ON" : "OFF", broadcast ? "ON" : "OFF");

    while (true)
    {
        sockaddr_in cliAddr;
        socklen_t len = sizeof(cliAddr);
        int csd = accept(sd, (sockaddr *)&cliAddr, &len);
        if (csd == -1)
        {
            myerror("accept");
            continue;
        }
        std::thread(client_handler, csd).detach();
    }
    close(sd);
    return 0;
}
