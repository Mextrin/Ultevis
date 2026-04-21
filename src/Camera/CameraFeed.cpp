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
static std::string parseValue(const std::string& json, const std::string& key) {
    auto pos = json.find("\"" + key + "\":");
    if (pos == std::string::npos) return {};
    pos = json.find(':', pos) + 1;
    while (pos < json.size() && json[pos] == ' ') pos++;

    const auto end = json.find_first_of(",}", pos);
    return json.substr(pos, end - pos);
}

static float parseFloat(const std::string& json, const std::string& key) {
    const auto value = parseValue(json, key);
    if (value.empty()) return 0.0f;
    return std::stof(value);
}

static bool parseBool(const std::string& json, const std::string& key) {
    const auto value = parseValue(json, key);
    return value == "true";
}

static int parseInt(const std::string& json, const std::string& key, int fallback = 0) {
    const auto value = parseValue(json, key);
    if (value.empty()) return fallback;
    return std::stoi(value);
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
        state->leftDrumHit.store(parseBool(json, "leftDrumHit"));
        state->rightDrumHit.store(parseBool(json, "rightDrumHit"));
        state->leftDrumType.store(parseInt(json, "leftDrumType", state->leftDrumType.load()));
        state->rightDrumType.store(parseInt(json, "rightDrumType", state->rightDrumType.load()));
        state->leftDrumVelocity.store(parseInt(json, "leftDrumVelocity", state->leftDrumVelocity.load()));
        state->rightDrumVelocity.store(parseInt(json, "rightDrumVelocity", state->rightDrumVelocity.load()));

        std::cout << "R: " << state->rightHandVisible.load()
                  << "  x=" << state->rightHandX.load()
                  << "  y=" << state->leftHandY.load() << "\n";
    }

    #ifdef _WIN32
        closesocket(sock);
        WSACleanup();
    #else
        close(sock);
    #endif
}
