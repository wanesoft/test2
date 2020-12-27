
#include <event2/event.h>

#include <array>
#include <iostream>
#include <thread>
#include <arpa/inet.h>

static char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen)
{
    switch(sa->sa_family) {
        case AF_INET:
            inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), s, maxlen);
            break;
        case AF_INET6:
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), s, maxlen);
            break;
        default:
            assert(0);
    }
    sprintf(s, "%s:%d", s, ntohs(((struct sockaddr_in *)sa)->sin_port));
    return s;
}

static evutil_socket_t new_bind_socket() {

    evutil_socket_t udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    int optval = 1;
    int resSetSockopt = setsockopt(udpSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (resSetSockopt != 0) {
        std::cerr << strerror(errno) << '\n';
    } else {
        std::cout << "Result of setsockopt(): " << resSetSockopt << '\n';
    }

    sockaddr_in locAddr{0};
    locAddr.sin_addr.s_addr = INADDR_ANY;
    locAddr.sin_port = htons(3129);
    locAddr.sin_family = AF_INET;
    if (auto resBind = bind(udpSocket, (sockaddr *) &locAddr, sizeof(locAddr)); resBind != 0) {
        std::cerr << strerror(errno) << '\n';
        exit(EXIT_FAILURE);
    }

    return udpSocket;
}

void simplereadcb(evutil_socket_t fd, short what, void *data) {

    std::array<uint8_t, 64*1024> buff{};

    int recvRes = recv(fd, buff.data(), buff.size(), MSG_DONTWAIT);
    std::cout << "Simple receive " << recvRes << " bytes from fd: " << fd <<  '\n';
}

void readcb(evutil_socket_t fd, short what, void *data) {

    sockaddr_in addr{0};
    socklen_t addrLen = sizeof(addr);

    std::array<uint8_t, 64*1024> buff{};
    char addrStrBuff[1024] = {0};

    int recvRes = recvfrom(fd, buff.data(), buff.size(), MSG_DONTWAIT, (sockaddr *)&addr, &addrLen);
    std::cout << "Receive " << recvRes << " bytes from " << get_ip_str((sockaddr *)&addr, addrStrBuff, 1024)
              << " and fd: " << fd <<  '\n';

    int resConn;
    if (resConn = connect(fd,(sockaddr *)&addr, addrLen); resConn != 0) {
        std::cerr << strerror(errno) << '\n';
    } else {
        std::cout << "Result of connect(): " << resConn << '\n';
    }

    auto base = (event_base *)data;
    auto udpEvent = event_new(base, fd, EV_TIMEOUT | EV_READ | EV_PERSIST, &simplereadcb, base);
    event_add(udpEvent, nullptr);

    evutil_socket_t udpSocket = new_bind_socket();
    auto udpEventNew = event_new(base, udpSocket, EV_TIMEOUT | EV_READ, &readcb, base);
    event_add(udpEventNew, nullptr);
}

int main() {

    evutil_socket_t udpSocket = new_bind_socket();

    auto base = event_base_new();

    auto udpEvent = event_new(base, udpSocket, EV_TIMEOUT | EV_READ, &readcb, base);
    event_add(udpEvent, nullptr);

    std::thread t([&]() {
        event_base_dispatch(base);
    });

    t.join();
    return 0;
}