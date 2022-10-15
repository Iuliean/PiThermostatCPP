#pragma once
#include <atomic>
#include <chrono>
#include <string>
#include <vector>
#include <mutex>
#include <memory>

class Cookie
{
private:
    static std::vector<std::shared_ptr<Cookie>> cookies;
    static std::mutex cookiesMutex;
    
    mutable std::mutex m_objectMutex;
    
    std::string m_token;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_timeOfCreation;

public:
    static std::atomic<unsigned int> lifetime;
    using  CookieIter = typename std::vector<std::shared_ptr<Cookie>>::iterator;
    using  CookieIter_const = typename std::vector<std::shared_ptr<Cookie>>::const_iterator;

public:
    Cookie();
    bool isExpired()const;
    std::string toString()const;
    inline std::string token()const
    {
        std::lock_guard<std::mutex> l(m_objectMutex);
        return m_token;
    }

    static std::weak_ptr<const Cookie> generateCookie();
    static bool verifyCookie(const std::string& token);
    static void cookieCleaner();
private:
    static std::string generateToken();
    static bool isCopy(const std::string& token);
};