#ifndef LOCAL_SERVER_H
#define LOCAL_SERVER_H

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>

struct HttpResponse {
    int statusCode = 200;
    std::string contentType = "application/json";
    std::string body;
};

class LocalServer {
public:
    using EmptyHandler = std::function<HttpResponse()>;
    using BodyHandler = std::function<HttpResponse(const std::string& body)>;

    LocalServer();
    ~LocalServer();

    bool start(int port);
    void stop();

    bool isRunning() const;

    void setHealthHandler(EmptyHandler handler);
    void setGetSettingsHandler(EmptyHandler handler);
    void setUpdateSettingsHandler(BodyHandler handler);
    void setPrintHandler(BodyHandler handler);

private:
    void serverLoop();
    void handleClient(std::uintptr_t clientSocket);

    int port_ = 0;
    std::uintptr_t serverSocket_ = 0;

    std::atomic<bool> running_ = false;
    std::thread serverThread_;

    EmptyHandler healthHandler_;
    EmptyHandler getSettingsHandler_;
    BodyHandler updateSettingsHandler_;
    BodyHandler printHandler_;
};

#endif