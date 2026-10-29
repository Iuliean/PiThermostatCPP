#ifndef MIDDLEWARES_AUTHORIZE_HPP
#define MIDDLEWARES_AUTHORIZE_HPP

#include "crow/middleware.h"
#include "crow/middlewares/session.h"

namespace pi::web::middleware
{
    struct authorize : crow::ILocalMiddleware
    {
        struct context {};

        authorize(std::string password)
            : crow::ILocalMiddleware(),
              m_password(std::move(password))
        {}

        ~authorize() {}

        template<typename AllCtx>
        void before_handle(crow::request& request, crow::response& response, [[maybe_unused]]context& ctx, [[maybe_unused]]AllCtx& global_ctx)
        {
            auto authorize_it = request.headers.find("Authorize");
            auto& session_ctx = global_ctx.template get<crow::SessionMiddleware<crow::InMemoryStore>>();

            if (session_ctx.template get<bool>("logged-in"))
                return;

            if (authorize_it == request.headers.end())
            {
                response.code = 401;
                response.end();
                return;
            }

            if (authorize_it->second != m_password)
            {
                response.code = 401;
                response.end();
                return;
            }

            session_ctx.set("logged-in", true);
        }

        void after_handle([[maybe_unused]]crow::request& req, [[maybe_unused]]crow::response& res, [[maybe_unused]]context& ctx)
        {

        }
    private:
        std::string m_password;
    };
}

#endif //MIDDLEWARES_AUTHORIZE_HPP
