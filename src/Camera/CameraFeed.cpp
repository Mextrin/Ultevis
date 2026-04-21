#include <iostream>
#include <string>
#include <atomic>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
    typedef int ssize_t;
#else
    #include <sys/socket.h>
    #include <sys/time.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <errno.h>
#endif

#include "Core/GlobalState.h"

static float parseFloat(const std::string& json, const std::string& key)
{
    auto pos = json.find("\"" + key + "\":");
    if (pos == std::string::npos) return 0.0f;
    pos = json.find(':', pos) + 1;
    return std::stof(json.substr(pos, 10));
}

static bool parseBool(const std::string& json, const std::string& key)
{
    auto pos = json.find("\"" + key + "\":");
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos) + 1;
    while (pos < json.size() && json[pos] == ' ') ++pos;
    return json.substr(pos, 4) == "true";
}

void startCameraFeed(GlobalState* state, std::atomic<bool>* stopFlag)
{
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { std::cerr << "CameraFeed: socket creation failed\n"; return; }

    // 100 ms receive timeout so we can poll the stop flag
    struct timeval tv { 0, 100000 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(5005);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "CameraFeed: bind failed\n";
#ifdef _WIN32
        closesocket(sock); WSACleanup();
#else
        close(sock);
#endif
        return;
    }

    std::cout << "CameraFeed: listening on UDP port 5005\n";

    char buf[1024];
    while (!stopFlag || !stopFlag->load()) {
        ssize_t len = recvfrom(sock, buf, sizeof(buf) - 1, 0, nullptr, nullptr);
        if (len < 0) {
#ifdef _WIN32
            if (WSAGetLastError() == WSAETIMEDOUT) continue;
#else
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
#endif
            break; // real error
        }
        buf[len] = '\0';
        const std::string json(buf);

        state->rightHandVisible.store(parseBool(json, "rightHandVisible"));
        state->leftHandVisible.store(parseBool(json,  "leftHandVisible"));
        state->rightHandX.store(parseFloat(json, "rightHandX"));
        state->leftHandY.store(parseFloat(json,  "leftHandY"));

        // Drum hits
        if (parseBool(json, "leftDrumHit")) {
            state->leftDrumType.store(static_cast<int>(parseFloat(json, "leftDrumType")));
            state->leftDrumVelocity.store(static_cast<int>(parseFloat(json, "leftDrumVelocity")));
            state->leftDrumHit.store(true);
        }
        if (parseBool(json, "rightDrumHit")) {
            state->rightDrumType.store(static_cast<int>(parseFloat(json, "rightDrumType")));
            state->rightDrumVelocity.store(static_cast<int>(parseFloat(json, "rightDrumVelocity")));
            state->rightDrumHit.store(true);
        }
    }

#ifdef _WIN32
    closesocket(sock); WSACleanup();
#else
    close(sock);
#endif
}
