#ifndef ERROR_HPP
#define ERROR_HPP
#include <type_traits>
#include <string>
#include <utility>

namespace pi
{
    template<typename Kind>
        requires std::is_enum_v<Kind>
    class error
    {
    public:
        constexpr error(Kind kind, std::string message) noexcept
            : m_err_kind(kind),
              m_message(std::move(message))
        { }
        constexpr ~error() = default;

        constexpr Kind kind() const noexcept { return m_err_kind; }

        constexpr decltype(auto) message(this auto&& self )
        {
            return std::forward_like<decltype(self)>(self.m_message);
        }

    private:
        Kind m_err_kind;
        std::string m_message;
    };
}

#endif //ERROR_HPP