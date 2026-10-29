#ifndef UTILS_HPP
#define UTILS_HPP

#include "nlohmann/adl_serializer.hpp"
#include "nlohmann/detail/conversions/to_json.hpp"
#include "nlohmann/json.hpp"

namespace nlohmann
{
    /**************
    * SERIALIZERS *
    **************/

    template <typename ...Args>
    struct adl_serializer<std::variant<Args...>> {

        static constexpr void to_json(json& json, const std::variant<Args...>& value)
        {
            std::visit([&json](auto& value) {
                ::nlohmann::to_json(json, value);
            },
            value);
        }

        static constexpr void to_json(json& json, std::variant<Args...>&& value)
        {
            std::visit([&json](auto&& value) {
                ::nlohmann::to_json(json, std::move(value));
            },
            value);
        }

        static void from_json([[maybe_unused]]const json& j, [[maybe_unused]]auto value)
        {
            static_assert(false, "Not implemented");
        }
    };

    template <typename T>
    struct adl_serializer<std::optional<T>>
    {

        static constexpr void to_json(json& json, const std::optional<T>& value)
        {
            if (!value)
                json = nullptr;
            else
                json = *value;
        }

        static constexpr void to_json(json& json, std::optional<T>&& value)
        {
            if (!value)
                json = nullptr;
            else
                json = std::move(*value);
        }

        static void from_json([[maybe_unused]]const json& j, [[maybe_unused]]auto value)
        {
            static_assert(false, "Not implemented");
        }
    };

}

#endif //UTILS_HPP