#ifndef EVENTS_HPP
#define EVENTS_HPP
#include <concepts>
#include <queue>
#include <unordered_map>
#include <string>
#include <memory>
#include <functional>

namespace pi
{
    class transaction
    {
    public:
        enum class type
        {
            fetch,
            store
        };

        transaction(type type, std::string resource);
    private:
        type m_type;
        std::string m_resource;
    };

    class resource_interface
    {
    public:
        virtual void perform(transaction t) = 0;
    private:
    };

    template<typename StoreSig, typename FetchSig>
    class bidirectional_resource : public resource_interface
    {
    public:
        bidirectional_resource(std::function<StoreSig> store_fn, std::function<FetchSig> fetch_fn)
            : m_store(std::move(store_fn)), m_fetch(std::move(fetch_fn))
        {}

        void perform(transaction t) override;

    private:
        std::function<StoreSig> m_store;
        std::function<FetchSig> m_fetch;
    };


    class event_manager
    {
    public:

        void emit(event e);

        template<std::derived_from<resource_interface> R>
        void register_resource(R resource);

        void dispatch();

    private:
        std::queue<transaction> m_transactions;
        std::unordered_map<std::string, std::unique_ptr<resource_interface>> m_handler;
    };
}

#endif //EVENTS_HPP