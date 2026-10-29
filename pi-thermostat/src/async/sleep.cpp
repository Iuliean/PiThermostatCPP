#include "async/sleep.hpp"

namespace pi::async
{
    /**********
    * PRIVATE *
    **********/

    sleep_timer::sleep_timer(timer_t timer_id, std::shared_ptr<coro::event> event)
        : m_event(event),
          m_timer_id(timer_id)
    { }

    sleep_timer::~sleep_timer()
    {
        timer_delete(m_timer_id);
    }

    void sleep_timer::timer_entry(__sigval_t val)
    {
        auto shared_obj_ptr = reinterpret_cast<std::shared_ptr<coro::event>*>(val.sival_ptr);
        auto event_ptr = *shared_obj_ptr;
        event_ptr->set();
        delete shared_obj_ptr;
    };
}