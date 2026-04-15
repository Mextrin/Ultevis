#include <iostream>
#include <string>

#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
    #include <chrono>
    #include <cctype>
    #include <cerrno>
    #include <cstdint>
    #include <cstdlib>
    #include <cstring>
    #include <optional>
    typedef int socklen_t;
    #ifdef _MSC_VER
        typedef int ssize_t;
    #endif
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

#ifdef _WIN32
namespace {

struct ParsedPacket {
    bool rightHandVisible = false;
    bool leftHandVisible = false;
    float rightHandX = 0.0f;
    float leftHandY = 0.0f;
    std::optional<std::uint64_t> sentAtMs;
};

struct DebugWindow {
    std::uint64_t datagramsSeen = 0;
    std::uint64_t invalidDatagrams = 0;
    std::uint64_t queuedDrops = 0;
    std::uint64_t appliedPackets = 0;
    std::uint64_t ageSamples = 0;
    std::int64_t totalAgeMs = 0;
    std::int64_t maxAgeMs = 0;
    std::chrono::steady_clock::time_point nextPrint =
        std::chrono::steady_clock::now() + std::chrono::seconds(2);
};

const char* findValueStart(const std::string& json, const std::string& key)
{
    const auto pos = json.find("\"" + key + "\":");
    if (pos == std::string::npos)
        return nullptr;

    std::size_t valuePos = pos + key.size() + 3;
    if (valuePos >= json.size())
        return nullptr;

    while (valuePos < json.size() && std::isspace(static_cast<unsigned char>(json[valuePos])))
        ++valuePos;

    return valuePos < json.size() ? json.c_str() + valuePos : nullptr;
}

bool parseUInt64Field(const std::string& json, const std::string& key, std::uint64_t& value)
{
    const char* start = findValueStart(json, key);
    if (start == nullptr)
        return false;

    errno = 0;
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(start, &end, 10);

    if (start == end || errno == ERANGE)
        return false;

    value = static_cast<std::uint64_t>(parsed);
    return true;
}

std::optional<ParsedPacket> parsePacket(const std::string& json)
{
    ParsedPacket packet;
    try {
        packet.rightHandVisible = parseBool(json, "rightHandVisible");
        packet.leftHandVisible = parseBool(json, "leftHandVisible");
        packet.rightHandX = parseFloat(json, "rightHandX");
        packet.leftHandY = parseFloat(json, "leftHandY");
    } catch (const std::exception&) {
        return std::nullopt;
    }

    std::uint64_t sentAtMs = 0;
    if (parseUInt64Field(json, "sentAtMs", sentAtMs))
        packet.sentAtMs = sentAtMs;

    return packet;
}

std::int64_t monotonicNowMs()
{
    using Clock = std::chrono::steady_clock;
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        Clock::now().time_since_epoch()).count();
}

void maybePrintDebugStats(DebugWindow& window, bool debugEnabled)
{
    if (!debugEnabled)
        return;

    const auto now = std::chrono::steady_clock::now();
    if (now < window.nextPrint)
        return;

    std::cout << "[UDP] datagrams=" << window.datagramsSeen
              << " applied=" << window.appliedPackets
              << " invalid=" << window.invalidDatagrams
              << " stale=" << window.queuedDrops;

    if (window.ageSamples > 0) {
        const auto averageAge = static_cast<double>(window.totalAgeMs) /
            static_cast<double>(window.ageSamples);
        std::cout << " avgAgeMs=" << averageAge
                  << " maxAgeMs=" << window.maxAgeMs;
    }

    std::cout << '\n';

    window = {};
    window.nextPrint = now + std::chrono::seconds(2);
}

} // namespace
#endif

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

#ifdef _WIN32
    u_long mode = 1;
    if (ioctlsocket(sock, FIONBIO, &mode) != 0) {
        std::cerr << "Failed to set UDP socket to non-blocking mode\n";
        closesocket(sock);
        WSACleanup();
        return;
    }

    const bool debugEnabled = std::getenv("ULTEVIS_DEBUG_UDP") != nullptr;
    DebugWindow debugWindow;
    char buf[1024];

    while (true) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sock, &readSet);

        timeval timeout{};
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        const int ready = select(0, &readSet, nullptr, nullptr, &timeout);
        if (ready == 0) {
            maybePrintDebugStats(debugWindow, debugEnabled);
            continue;
        }

        if (ready < 0) {
            std::cerr << "UDP select failed\n";
            break;
        }

        std::optional<ParsedPacket> newestPacket;

        while (true) {
            ssize_t len = recvfrom(sock, buf, sizeof(buf) - 1, 0, nullptr, nullptr);
            if (len < 0) {
                const int error = WSAGetLastError();
                if (error == WSAEWOULDBLOCK)
                    break;

                std::cerr << "UDP receive failed\n";
                closesocket(sock);
                WSACleanup();
                return;
            }

            buf[len] = '\0';
            debugWindow.datagramsSeen++;

            if (auto packet = parsePacket(std::string(buf, static_cast<std::size_t>(len)))) {
                if (newestPacket.has_value())
                    debugWindow.queuedDrops++;
                newestPacket = std::move(packet);
            } else {
                debugWindow.invalidDatagrams++;
            }
        }

        if (newestPacket.has_value()) {
            state->rightHandVisible.store(newestPacket->rightHandVisible);
            state->leftHandVisible.store(newestPacket->leftHandVisible);
            state->rightHandX.store(newestPacket->rightHandX);
            state->leftHandY.store(newestPacket->leftHandY);

            debugWindow.appliedPackets++;

            if (newestPacket->sentAtMs.has_value()) {
                const auto ageMs = monotonicNowMs() - static_cast<std::int64_t>(*newestPacket->sentAtMs);
                if (ageMs >= 0 && ageMs < 60000) {
                    debugWindow.ageSamples++;
                    debugWindow.totalAgeMs += ageMs;
                    if (ageMs > debugWindow.maxAgeMs)
                        debugWindow.maxAgeMs = ageMs;
                }
            }
        }

        maybePrintDebugStats(debugWindow, debugEnabled);
    }
#else
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

        std::cout << "R: " << state->rightHandVisible.load()
                  << "  x=" << state->rightHandX.load()
                  << "  y=" << state->leftHandY.load() << "\n";
    }
#endif

    #ifdef _WIN32
        closesocket(sock);
        WSACleanup();
    #else
        close(sock);
    #endif
}
