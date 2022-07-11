#include"cookie.h"

#include <random>

#define HOUR 3600

static const std::vector<char> alphabet = {
    '0','1','2','3','4','5','6','7','8','9',
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'
    }; 

static std::default_random_engine generator (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
static std::uniform_int_distribution<int> distrubution(0,60);

std::vector<Cookie> Cookie::cookies;
std::mutex Cookie::cookiesMutex;
unsigned int Cookie::lifetime;


Cookie::Cookie()
{
    this->token = Cookie::generateToken();
    this->timeOfCreation = std::chrono::high_resolution_clock::now();
}

bool Cookie::isExpired()const
{
    std::chrono::time_point<std::chrono::high_resolution_clock> now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<float>(now - this->timeOfCreation).count() > Cookie::lifetime ? true : false;
}

std::string Cookie::toString()const
{
    return "authToken=" + this->token + "; Max-Age=" + std::to_string(Cookie::lifetime);
}

const Cookie& Cookie::generateCookie()
{
    Cookie::cookiesMutex.lock();
    Cookie::cookies.emplace_back();    
    Cookie::cookiesMutex.unlock();
    
    return Cookie::cookies.back();
}

bool Cookie::verifyCookie(const std::string& token)
{
    Cookie::cookiesMutex.lock();
    for(const Cookie& cookie : Cookie::cookies)
    {
        bool comp =  cookie.token == token;

        if(cookie.token == token && !cookie.isExpired())
        {
            Cookie::cookiesMutex.unlock();
            return true;
        }
    }
    Cookie::cookiesMutex.unlock();
    return false;
}

void Cookie::cookieCleaner()
{
    Cookie::cookiesMutex.lock();
    int end = 0;

    for (std::vector<Cookie>::iterator it = Cookie::cookies.begin(); it != Cookie::cookies.end(); it++)
    {    
        if(it->isExpired())
            end += 1;
        else
            break;
    }

    if(end != 0)
        Cookie::cookies.erase(Cookie::cookies.begin(),Cookie::cookies.begin()+end);
   
    Cookie::cookiesMutex.unlock();
}

//Private
std::string Cookie::generateToken()
{    
    std::string token = "";
    bool ready = false;

    while (!ready)
    {
        for(int i = 0; i < alphabet.size(); i++)
        {
            token += alphabet[distrubution(generator)];
        }
        if(!Cookie::isCopy(token))
        {
            ready = true;
        }
        else
            token = "";
    }
    return token;
}

bool Cookie::isCopy(const std::string& token)
{
    for(const Cookie& cookie : Cookie::cookies)
    {
        if(cookie.token == token)
            return true;
    }

    return false;
}

