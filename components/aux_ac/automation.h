#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "aux_ac.h"

namespace esphome {
namespace aux_ac {

    template <typename... Ts>
    class AirConDisplayOffAction : public Action<Ts...>
    {
    public:
        explicit AirConDisplayOffAction(AirCon *ac) : ac_(ac) {}

        void play(Ts... x) override { this->ac_->displaySequence(AC_DISPLAY_OFF); }

    protected:
        AirCon *ac_;
    };    

    template <typename... Ts>
    class AirConDisplayOnAction : public Action<Ts...>
    {
    public:
        explicit AirConDisplayOnAction(AirCon *ac) : ac_(ac) {}

        void play(Ts... x) override { this->ac_->displaySequence(AC_DISPLAY_ON); }

    protected:
        AirCon *ac_;
    };    

} // namespace aux_ac
} // namespace esphome