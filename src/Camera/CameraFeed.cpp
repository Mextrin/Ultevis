#include <iostream>
#include <string>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
    typedef int ssize_t;
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

#include "Core/GlobalState.h"

// Minimal JSON field parser — no library needed for this simple payload
static float parseFloat(const std::string& json, const std::string& key) {
    auto pos = json.find("\"" + key + "\":");
    if (pos == std::string::npos) return 0.0f;
    pos = json.find(':', pos) + 1;
    return std::stof(json.substr(pos, 10));
}

static bool parseBool(const std::string& json, const std::string& key) {
    auto pos = json.find("\"" + key + "\":");
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos) + 1;
    while (json[pos] == ' ') pos++;
    return json.substr(pos, 4) == "true";
}

void startCameraFeed(GlobalState* state) {
#ifdef _WIN32
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { std::cerr << "Socket creation failed\n"; return; }

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(5005);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Bind failed\n";
        #ifdef _WIN32
            closesocket(sock);
            WSACleanup();
        #else
            close(sock);
        #endif
        return;
    }

    std::cout << "Listening for hand data on UDP port 5005...\n";

    char buf[1024];
    while (true) {
        ssize_t len = recvfrom(sock, buf, sizeof(buf) - 1, 0, nullptr, nullptr);
        if (len < 0) break;
        buf[len] = '\0';

        std::string json(buf);

        state->rightHandVisible.store(parseBool(json, "rightHandVisible"));
        state->leftHandVisible.store(parseBool(json,  "leftHandVisible"));
        state->rightHandX.store(parseFloat(json, "rightHandX"));
        state->leftHandY.store(parseFloat(json,  "leftHandY"));

        // std::cout << "R: " << state->rightHandVisible.load()
        //           << "  x=" << state->rightHandX.load()
        //           << "  y=" << state->leftHandY.load() << "\n";
    }

    #ifdef _WIN32
        closesocket(sock);
        WSACleanup();
    #else
        close(sock);
    #endif
}