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

        void play(Ts... x) override { this->ac_->displayOffSequence(); }

    protected:
        AirCon *ac_;
    };    

    template <typename... Ts>
    class AirConDisplayOnAction : public Action<Ts...>
    {
    public:
        explicit AirConDisplayOnAction(AirCon *ac) : ac_(ac) {}

        void play(Ts... x) override { this->ac_->displayOnSequence(); }

    protected:
        AirCon *ac_;
    };

    template <typename... Ts>
    class AirConSendTestPacketAction : public Action<Ts...>
    {
    public:
        explicit AirConSendTestPacketAction(AirCon *ac) : ac_(ac) {}
        void set_data_template(std::function<std::vector<uint8_t>(Ts...)> func) {
            this->data_func_ = func;
            this->static_ = false;
        }        
        void set_data_static(const std::vector<uint8_t> &data) {
            this->data_static_ = data;
            this->static_ = true;
        }

        void play(Ts... x) override {
            if (this->static_) {
                this->ac_->sendTestPacket(this->data_static_);
            } else {
                auto val = this->data_func_(x...);
                this->ac_->sendTestPacket(val);
            }
        }

    protected:
        AirCon *ac_;
        bool static_{false};
        std::function<std::vector<uint8_t>(Ts...)> data_func_{};
        std::vector<uint8_t> data_static_{};
    };    

} // namespace aux_ac
} // namespace esphome