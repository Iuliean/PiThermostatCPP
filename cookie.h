#pragma once
#include <chrono>
#include <string>
#include <vector>
#include <mutex>

class Cookie
{
private:
    static std::vector<Cookie> cookies;
    static std::mutex cookiesMutex;
public:
    static unsigned int lifetime;

    std::string token;
    std::chrono::time_point<std::chrono::high_resolution_clock> timeOfCreation;

public:
    Cookie();
    bool isExpired()const;
    std::string toString()const;

    static const Cookie& generateCookie();
    static bool verifyCookie(const std::string& token);
    static void cookieCleaner();
private:
    static std::string generateToken();
    static bool isCopy(const std::string& token);
};