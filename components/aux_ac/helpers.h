#pragma once

#include <stdint.h>
#include <functional>
#include <list>
#include "esphome/core/optional.h"

namespace esphome
{
    namespace helpers
    {

        /*************************************************************************************************\
        \*************************************************************************************************/
        class TimerInterface
        {
        public:
            virtual bool is_expired() const = 0;
            virtual bool is_enabled() const = 0;
            virtual void start(uint32_t period_ms) = 0;
            virtual void stop() = 0;
            virtual void reset() = 0;
            virtual void set_callback(std::function<void(TimerInterface *)> callback) = 0;
            virtual void trigger_callback() = 0;
        };

        /*************************************************************************************************\
        \*************************************************************************************************/
        using millis_function_t = uint32_t (*)();

        class TimerManager
        {
        public:
            static void set_millis(uint32_t current_time) { TimerManager::_millis = current_time; }
            static uint32_t get_millis() { return TimerManager::_millis; }

            void set_millis_func(millis_function_t millis) { _millis_func = millis; }

            void register_timer(TimerInterface &timer) { _timers.push_back(&timer); }

            void task()
            {
                if (_millis_func != nullptr)
                    _millis = _millis_func();

                for (auto timer : _timers)
                    if (timer->is_enabled() && timer->is_expired())
                        timer->trigger_callback();
            }

        private:
            millis_function_t _millis_func{nullptr};
            static uint32_t _millis;
            std::list<TimerInterface *> _timers;
        };

        /*************************************************************************************************\
        \*************************************************************************************************/
        static void dummy_stopper(TimerInterface *timer) { timer->stop(); }

        class Timer : public TimerInterface
        {
        public:
            Timer() : _callback(dummy_stopper), _period_ms(0) {}

            virtual bool is_expired() const override { return TimerManager::get_millis() - this->_last_trigger_time >= this->_period_ms; }
            virtual bool is_enabled() const override { return this->_period_ms > 0; }

            virtual void start(uint32_t period_ms) override
            {
                this->_period_ms = period_ms;
                this->reset();
            }
            virtual void stop() override { this->_period_ms = 0; }
            virtual void reset() override { this->_last_trigger_time = TimerManager::get_millis(); }

            virtual void set_callback(std::function<void(TimerInterface *)> callback) override { this->_callback = callback; }
            virtual void trigger_callback() override
            {
                this->_callback((TimerInterface *)this);
                this->reset();
            }

        private:
            std::function<void(TimerInterface *)> _callback = nullptr;
            uint32_t _period_ms;
            uint32_t _last_trigger_time;
        };

        /*********************************************************************************************\
        \*********************************************************************************************/
        template <typename T>
        bool update_property(T &property, const T &value, bool &flag)
        {
            if (property != value)
            {
                property = value;
                flag = true;
                return true;
            }
            return false;
        }

        template <typename T>
        bool update_property(optional<T> &property, const T &value, bool &flag)
        {
            if (property != value)
            {
                property = value;
                flag = true;
                return true;
            }
            return false;
        }

    } // namespace helpers
} // namespace esphome
