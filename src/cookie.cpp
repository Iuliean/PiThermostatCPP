#include"cookie.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <random>
#include <set>
#include <vector>

#define HOUR 3600

static const std::vector<char> alphabet = {
    '0','1','2','3','4','5','6','7','8','9',
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'
    }; 

static std::default_random_engine generator (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
static std::uniform_int_distribution<int> distrubution(0,60);

std::vector<std::shared_ptr<Cookie>> Cookie::cookies;
std::mutex Cookie::cookiesMutex;
std::atomic<unsigned int> Cookie::lifetime;

Cookie::Cookie()
{
    std::lock_guard<std::mutex> l(m_objectMutex);
    m_token = Cookie::generateToken();
    m_timeOfCreation = std::chrono::high_resolution_clock::now();
}

bool Cookie::isExpired()const
{
    std::lock_guard<std::mutex> l(m_objectMutex);
    std::chrono::time_point<std::chrono::high_resolution_clock> now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<float>(now - m_timeOfCreation).count() > Cookie::lifetime ? true : false;
}

std::string Cookie::toString()const
{
    std::lock_guard<std::mutex> l (m_objectMutex);
    return "authToken=" + m_token + "; Max-Age=" + std::to_string(Cookie::lifetime);
}

std::weak_ptr<const Cookie> Cookie::generateCookie()
{
    std::lock_guard<std::mutex> l(cookiesMutex);
    cookies.emplace_back(std::make_shared<Cookie>());

    return cookies.back();
}

bool Cookie::verifyCookie(const std::string& token)
{
    std::lock_guard<std::mutex> l (cookiesMutex); 
    for(std::shared_ptr<const Cookie> cookie : Cookie::cookies)
    {        
        if(cookie->token() == token && !cookie->isExpired())
            return true;
    }
 
    return false;
}

void Cookie::cookieCleaner()
{
    std::lock_guard<std::mutex> l(cookiesMutex);
    int end = 0;

    for (CookieIter_const it = Cookie::cookies.begin(); it != Cookie::cookies.end(); it++)
    {    
        if(it->get()->isExpired())
            end += 1;
        else
            break;
    }

    if(end != 0)
        Cookie::cookies.erase(Cookie::cookies.begin(),Cookie::cookies.begin()+end);
}

//Private
std::string Cookie::generateToken()
{    
    std::string token = "";
    
    do
    {
        for(int i = 0; i < alphabet.size(); i++)
        {
            token += alphabet[distrubution(generator)];
        }
    }while(Cookie::isCopy(token));

    return token;
}

bool Cookie::isCopy(const std::string& token)
{
    for(std::shared_ptr<const Cookie> cookie: Cookie::cookies)
    {
        if(cookie->token() == token)
            return true;
    }

    return false;
}

