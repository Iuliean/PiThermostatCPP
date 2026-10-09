#ifndef ERROR_HPP
#define ERROR_HPP
#include <source_location>
#include <type_traits>
#include <string>
#include <utility>
#include <format>
#include <concepts>
#include <expected>

#define RETURN_IF_UNEXPECTED(e) do { \
    auto res = e; \
    if (!res) return res; }while(false) \

#define RETURN_IF_EXPECTED(e) do { \
    auto res = e; \
    if (res) return res; }while(false) \

#define RETURN_VALUE_IF_EXPECTED_ELSE_FORWARD(e, value) do { \
    auto res = e; \
    if (res) return value; else return std::unexpected{std::move(res).error()};} while(false) \


namespace pi
{

    using errno_t = int;
    template<typename Kind>
        requires (std::is_enum_v<Kind> || std::same_as<Kind, errno_t>)
    class error
    {
    public:
        constexpr error(Kind kind, std::string message, std::source_location source = std::source_location::current()) noexcept
            : m_err_kind(kind),
              m_message(std::move(message)),
              m_source(std::move(source))
        { }

        constexpr error(Kind kind, std::string_view message, std::source_location source = std::source_location::current())
            : m_err_kind(kind),
              m_message(message),
              m_source(std::move(source))
        { }

        constexpr error(Kind kind, const char* message, std::source_location source = std::source_location::current())
            : error(kind, std::string_view(message), source)
        { }

        constexpr ~error() {};

        constexpr Kind kind() const noexcept { return m_err_kind; }

        constexpr decltype(auto) message(this auto&& self )
        {
            return std::forward_like<decltype(self)>(self.m_message);
        }

        constexpr std::source_location source()const noexcept { return m_source; }

        static std::unexpected<error<Kind>> make_error(Kind kind, auto&& message, std::source_location source = std::source_location::current()) noexcept
        {
            return std::unexpected<error<Kind>>{
                std::in_place,
                kind,
                std::forward<decltype(message)&&>(message),
                std::move(source)
            };
        }
    private:
        Kind m_err_kind;
        std::string m_message;
        std::source_location m_source;
    };

    using os_error = error<errno_t>;
}

namespace std
{
    template<typename Kind>
    struct formatter<pi::error<Kind>> : public formatter<std::string>
    {
        template<typename FmtContext>
        FmtContext::iterator format(const pi::error<Kind>& err, FmtContext &ctx) const
        {
            return std::format_to(ctx.out(), "{}:{}: err{{ code{}:{} }}", err.source().file_name(), err.source().line(), std::to_underlying(err.kind()), err.message()); 
        }
    };
}

#endif //ERROR_HPP