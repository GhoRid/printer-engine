#include "local_server.h"

#include <utility>

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace {

constexpr std::size_t MAX_REQUEST_SIZE = 2 * 1024 * 1024;

struct HttpRequest {
    std::string method;
    std::string path;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

std::string toLower(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        }
    );

    return value;
}

std::string trim(const std::string& value)
{
    const std::size_t start = value.find_first_not_of(" \t\r\n");

    if (start == std::string::npos) {
        return "";
    }

    const std::size_t end = value.find_last_not_of(" \t\r\n");

    return value.substr(start, end - start + 1);
}

std::string getStatusText(int statusCode)
{
    switch (statusCode) {
        case 200:
            return "OK";

        case 204:
            return "No Content";

        case 400:
            return "Bad Request";

        case 404:
            return "Not Found";

        case 405:
            return "Method Not Allowed";

        case 413:
            return "Payload Too Large";

        case 500:
            return "Internal Server Error";

        case 503:
            return "Service Unavailable";

        default:
            return "OK";
    }
}

bool parseRequest(
    const std::string& rawRequest,
    HttpRequest& request
)
{
    const std::size_t headerEnd = rawRequest.find("\r\n\r\n");

    if (headerEnd == std::string::npos) {
        return false;
    }

    std::string headerSection = rawRequest.substr(0, headerEnd);

    request.body = rawRequest.substr(headerEnd + 4);

    std::istringstream stream(headerSection);

    std::string requestLine;

    if (!std::getline(stream, requestLine)) {
        return false;
    }

    requestLine = trim(requestLine);

    std::istringstream requestLineStream(requestLine);

    std::string version;

    if (!(requestLineStream >> request.method >> request.path >> version)) {
        return false;
    }

    std::string line;

    while (std::getline(stream, line)) {
        line = trim(line);

        if (line.empty()) {
            continue;
        }

        const std::size_t colon = line.find(':');

        if (colon == std::string::npos) {
            continue;
        }

        std::string key = toLower(
            trim(line.substr(0, colon))
        );

        std::string value = trim(
            line.substr(colon + 1)
        );

        request.headers[key] = value;
    }

    return true;
}

std::string buildResponse(const HttpResponse& response)
{
    std::ostringstream stream;

    stream
        << "HTTP/1.1 "
        << response.statusCode
        << ' '
        << getStatusText(response.statusCode)
        << "\r\n";

    stream
        << "Content-Type: "
        << response.contentType
        << "\r\n";

    stream
        << "Content-Length: "
        << response.body.size()
        << "\r\n";

    stream << "Access-Control-Allow-Origin: *\r\n";
    stream << "Access-Control-Allow-Methods: GET, POST, PUT, OPTIONS\r\n";
    stream << "Access-Control-Allow-Headers: Content-Type\r\n";
    stream << "Access-Control-Allow-Private-Network: true\r\n";
    stream << "Connection: close\r\n";
    stream << "\r\n";
    stream << response.body;

    return stream.str();
}

bool sendAll(
    SOCKET socketHandle,
    const std::string& data
)
{
    std::size_t totalSent = 0;

    while (totalSent < data.size()) {
        const int sent = send(
            socketHandle,
            data.data() + totalSent,
            static_cast<int>(data.size() - totalSent),
            0
        );

        if (sent == SOCKET_ERROR || sent == 0) {
            return false;
        }

        totalSent += static_cast<std::size_t>(sent);
    }

    return true;
}

HttpResponse makeJsonResponse(
    int statusCode,
    const std::string& body
)
{
    HttpResponse response;

    response.statusCode = statusCode;
    response.contentType = "application/json";
    response.body = body;

    return response;
}

} // namespace

LocalServer::LocalServer() = default;

LocalServer::~LocalServer()
{
    stop();
}

bool LocalServer::start(int port)
{
    if (running_) {
        return true;
    }

    if (port <= 0 || port > 65535) {
        return false;
    }

    WSADATA wsaData{};

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }

    SOCKET socketHandle = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (socketHandle == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }

    BOOL reuseAddress = TRUE;

    setsockopt(
        socketHandle,
        SOL_SOCKET,
        SO_REUSEADDR,
        reinterpret_cast<const char*>(&reuseAddress),
        sizeof(reuseAddress)
    );

    sockaddr_in address{};

    address.sin_family = AF_INET;
    address.sin_port = htons(
        static_cast<u_short>(port)
    );

    if (
        inet_pton(
            AF_INET,
            "127.0.0.1",
            &address.sin_addr
        ) != 1
    ) {
        closesocket(socketHandle);
        WSACleanup();

        return false;
    }

    if (
        bind(
            socketHandle,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)
        ) == SOCKET_ERROR
    ) {
        closesocket(socketHandle);
        WSACleanup();

        return false;
    }

    if (
        listen(
            socketHandle,
            SOMAXCONN
        ) == SOCKET_ERROR
    ) {
        closesocket(socketHandle);
        WSACleanup();

        return false;
    }

    port_ = port;

    serverSocket_ = static_cast<std::uintptr_t>(
        socketHandle
    );

    running_ = true;

    serverThread_ = std::thread(
        &LocalServer::serverLoop,
        this
    );

    std::cout
        << "Local server started: http://127.0.0.1:"
        << port_
        << '\n';

    return true;
}

void LocalServer::stop()
{
    if (!running_) {
        return;
    }

    running_ = false;

    SOCKET socketHandle = static_cast<SOCKET>(
        serverSocket_
    );

    if (socketHandle != INVALID_SOCKET) {
        shutdown(
            socketHandle,
            SD_BOTH
        );

        closesocket(socketHandle);
    }

    serverSocket_ = 0;

    if (serverThread_.joinable()) {
        serverThread_.join();
    }

    WSACleanup();

    std::cout << "Local server stopped\n";
}

bool LocalServer::isRunning() const
{
    return running_;
}

void LocalServer::setHealthHandler(
    EmptyHandler handler
)
{
    healthHandler_ = std::move(handler);
}

void LocalServer::setGetSettingsHandler(
    EmptyHandler handler
)
{
    getSettingsHandler_ = std::move(handler);
}

void LocalServer::setUpdateSettingsHandler(
    BodyHandler handler
)
{
    updateSettingsHandler_ = std::move(handler);
}

void LocalServer::setPrintHandler(
    BodyHandler handler
)
{
    printHandler_ = std::move(handler);
}

void LocalServer::serverLoop()
{
    SOCKET socketHandle = static_cast<SOCKET>(
        serverSocket_
    );

    while (running_) {
        SOCKET clientSocket = accept(
            socketHandle,
            nullptr,
            nullptr
        );

        if (clientSocket == INVALID_SOCKET) {
            if (running_) {
                std::cerr
                    << "Local server accept failed\n";
            }

            break;
        }

        handleClient(
            static_cast<std::uintptr_t>(
                clientSocket
            )
        );
    }
}

void LocalServer::handleClient(
    std::uintptr_t clientSocketValue
)
{
    SOCKET clientSocket = static_cast<SOCKET>(
        clientSocketValue
    );

    std::string rawRequest;

    char buffer[4096];

    std::size_t expectedSize = 0;

    bool headerParsed = false;

    while (rawRequest.size() < MAX_REQUEST_SIZE) {
        const int received = recv(
            clientSocket,
            buffer,
            sizeof(buffer),
            0
        );

        if (received <= 0) {
            break;
        }

        rawRequest.append(
            buffer,
            received
        );

        if (!headerParsed) {
            const std::size_t headerEnd =
                rawRequest.find("\r\n\r\n");

            if (headerEnd != std::string::npos) {
                headerParsed = true;

                expectedSize = headerEnd + 4;

                std::string headerSection =
                    rawRequest.substr(
                        0,
                        headerEnd
                    );

                std::istringstream headerStream(
                    headerSection
                );

                std::string line;

                while (std::getline(headerStream, line)) {
                    const std::size_t colon =
                        line.find(':');

                    if (colon == std::string::npos) {
                        continue;
                    }

                    std::string key = toLower(
                        trim(
                            line.substr(
                                0,
                                colon
                            )
                        )
                    );

                    std::string value = trim(
                        line.substr(
                            colon + 1
                        )
                    );

                    if (key == "content-length") {
                        try {
                            expectedSize +=
                                static_cast<std::size_t>(
                                    std::stoull(value)
                                );
                        }
                        catch (...) {
                            expectedSize =
                                rawRequest.size();
                        }
                    }
                }
            }
        }

        if (
            headerParsed &&
            rawRequest.size() >= expectedSize
        ) {
            break;
        }
    }

    if (rawRequest.size() >= MAX_REQUEST_SIZE) {
        HttpResponse response = makeJsonResponse(
            413,
            R"({"error":"request_too_large"})"
        );

        sendAll(
            clientSocket,
            buildResponse(response)
        );

        closesocket(clientSocket);

        return;
    }

    HttpRequest request;

    if (!parseRequest(rawRequest, request)) {
        HttpResponse response = makeJsonResponse(
            400,
            R"({"error":"invalid_request"})"
        );

        sendAll(
            clientSocket,
            buildResponse(response)
        );

        closesocket(clientSocket);

        return;
    }

    HttpResponse response;

    try {
        if (request.method == "OPTIONS") {
            response.statusCode = 204;
            response.contentType = "text/plain";
            response.body = "";
        }
        else if (
            request.method == "GET" &&
            request.path == "/health"
        ) {
            if (healthHandler_) {
                response = healthHandler_();
            }
            else {
                response = makeJsonResponse(
                    200,
                    R"({"status":"ok"})"
                );
            }
        }
        else if (
            request.method == "GET" &&
            request.path == "/settings"
        ) {
            if (getSettingsHandler_) {
                response =
                    getSettingsHandler_();
            }
            else {
                response = makeJsonResponse(
                    503,
                    R"({"error":"settings_handler_not_ready"})"
                );
            }
        }
        else if (
            request.method == "PUT" &&
            request.path == "/settings"
        ) {
            if (updateSettingsHandler_) {
                response =
                    updateSettingsHandler_(
                        request.body
                    );
            }
            else {
                response = makeJsonResponse(
                    503,
                    R"({"error":"settings_handler_not_ready"})"
                );
            }
        }
        else if (
            request.method == "POST" &&
            request.path == "/print"
        ) {
            if (printHandler_) {
                response =
                    printHandler_(
                        request.body
                    );
            }
            else {
                response = makeJsonResponse(
                    503,
                    R"({"error":"print_handler_not_ready"})"
                );
            }
        }
        else {
            response = makeJsonResponse(
                404,
                R"({"error":"not_found"})"
            );
        }
    }
    catch (...) {
        response = makeJsonResponse(
            500,
            R"({"error":"internal_server_error"})"
        );
    }

    sendAll(
        clientSocket,
        buildResponse(response)
    );

    closesocket(clientSocket);
}

#else

LocalServer::LocalServer() = default;

LocalServer::~LocalServer()
{
    stop();
}

bool LocalServer::start(int)
{
    return false;
}

void LocalServer::stop()
{
    running_ = false;
}

bool LocalServer::isRunning() const
{
    return false;
}

void LocalServer::setHealthHandler(
    EmptyHandler handler
)
{
    healthHandler_ = std::move(handler);
}

void LocalServer::setGetSettingsHandler(
    EmptyHandler handler
)
{
    getSettingsHandler_ = std::move(handler);
}

void LocalServer::setUpdateSettingsHandler(
    BodyHandler handler
)
{
    updateSettingsHandler_ = std::move(handler);
}

void LocalServer::setPrintHandler(
    BodyHandler handler
)
{
    printHandler_ = std::move(handler);
}

void LocalServer::serverLoop()
{
}

void LocalServer::handleClient(
    std::uintptr_t
)
{
}

#endif