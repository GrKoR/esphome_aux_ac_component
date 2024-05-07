// Custom ESPHome component for AUX-based air conditioners
// Need some soldering skills
// Source code and detailed instructions are available on github: https://github.com/GrKoR/esphome_aux_ac_component
/// немного переработанная версия старого компонента
#pragma once

#include <Arduino.h>
#include <stdarg.h>

#include "esphome.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

// весь функционал сохранения пресетов прячу под дефайн
//  #define PRESETS_SAVING
#ifdef PRESETS_SAVING
#ifdef ESP32
#include "esphome/core/preferences.h"
#else
#warning "Saving presets does not work with ESP8266"
#endif
#endif

namespace esphome
{
    namespace aux_ac
    {

        static const char *const TAG = "AirCon";

        using climate::ClimateFanMode;
        using climate::ClimateMode;
        using climate::ClimatePreset;
        using climate::ClimateSwingMode;
        using climate::ClimateTraits;

//****************************************************************************************************************************************************
//**************************************************** Packet logger configuration *******************************************************************
//****************************************************************************************************************************************************
// v.0.2.9: замена директиве HOLMS
#ifdef HOLMS
#undef HOLMS
#warning "HOLMS was deprecated in v.0.2.9. Use HOLMES_x instead (see below)."
#endif

// Директива HOLMES_WORKS позволяет включить (true) или выключить (false) вывод пакетов в лог
// Причём отключение вывода пакетов не затронет вывод остальных данных
#define HOLMES_WORKS true

// Директива HOLMES_BYTE_FORMAT задаёт формат вывод каждого байта пакета в лог в формате sprintf.
// Для вывода в шестнадцатиричном виде с двумя знаками, задайте "%02X".
// Для вывода в десятичном виде с тремя знаками, задайте "%03d".
#define HOLMES_BYTE_FORMAT "%02X"

// Директива HOLMES_FILTER_LEN обеспечивает фильтрацию вывода пакетов в лог.
// Все корректные пакеты, длина тела которых короче HOLMES_FILTER_LEN, будут проигнорированы.
// Все корректные пакеты, длина тела которых HOLMES_FILTER_LEN и более, попадут в лог.
// Все данные, не являющиеся корректными пакетами, попадут в лог в любом случае. Это нужно для целей отладки.
// В протоколе встречаются пакеты с телом следующей длины: 0, 1, 2, 4, 8, 15, 23
#define HOLMES_FILTER_LEN 0

// Директива HOLMES_DELIMITER позволяет задать разделитель байт при выводе в лог
// Для "классического" вывода задайте " "
// Для вывода "под Excel" задайте ";"
#define HOLMES_DELIMITER " "

// Директивы HOLMES_x_BRACKET_OPEN и HOLMES_x_BRACKET_CLOSE задают открывающую и
// закрывающую скобки для заголовка и CRC.
// Если вместо скобок указать "", то в логе скобок не будет.
#define HOLMES_HEADER_BRACKET_OPEN "["
#define HOLMES_HEADER_BRACKET_CLOSE "]"
#define HOLMES_CRC_BRACKET_OPEN "["
#define HOLMES_CRC_BRACKET_CLOSE "]"

        //****************************************************************************************************************************************************
        //************************************************* Constants for ESPHome integration ****************************************************************
        //****************************************************************************************************************************************************
        class Constants
        {
        public:
            static const std::string AC_FIRMWARE_VERSION;

            static const std::string MUTE;
            static const std::string TURBO;
            static const std::string CLEAN;
            static const std::string HEALTH;
            static const std::string ANTIFUNGUS;

            /// минимальная и максимальная температура в градусах Цельсия, ограничения самого кондиционера
            static const float AC_MIN_TEMPERATURE;
            static const float AC_MAX_TEMPERATURE;
            /// шаг изменения целевой температуры, градусы Цельсия
            static const float AC_TEMPERATURE_STEP;

            /// минимальное и максимальное значение мощности инвертора при установке ограничений
            static const uint8_t AC_MIN_INVERTER_POWER_LIMIT;
            static const uint8_t AC_MAX_INVERTER_POWER_LIMIT;

            // периодичность опроса кондиционера на предмет изменения состояния
            // изменение параметров с пульта не сообщается в UART, поэтому надо запрашивать состояние, чтобы быть в курсе
            // значение в миллисекундах
            static const uint32_t AC_STATES_REQUEST_INTERVAL;

            // границы допустимого диапазона таймаута загрузки пакета
            // таймаут загрузки - через такое количиство миллисекунд конечный автомат перейдет из
            // состояния ACSM_RECEIVING_PACKET в ACSM_IDLE, если пакет не будет загружен
            static const uint32_t AC_PACKET_TIMEOUT_MAX;
            static const uint32_t AC_PACKET_TIMEOUT_MIN;
        };

        // AUX_AC_FIRMWARE_VERSION will be defined by the ESPHome code generator at compile time
        const std::string Constants::AC_FIRMWARE_VERSION = AUX_AC_FIRMWARE_VERSION;

        // custom fan modes
        const std::string Constants::MUTE = "mute";
        const std::string Constants::TURBO = "turbo";

        // custom presets
        const std::string Constants::CLEAN = "Clean";
        const std::string Constants::HEALTH = "Health";
        const std::string Constants::ANTIFUNGUS = "Antifungus";

        // params
        const float Constants::AC_MIN_TEMPERATURE = 16.0;
        const float Constants::AC_MAX_TEMPERATURE = 32.0;
        const float Constants::AC_TEMPERATURE_STEP = 0.5;
        // AUX_AC_MIN_INVERTER_POWER_LIMIT and AUX_AC_MAX_INVERTER_POWER_LIMIT will be defined by the ESPHome code generator at compile time
        const uint8_t Constants::AC_MIN_INVERTER_POWER_LIMIT = AUX_AC_MIN_INVERTER_POWER_LIMIT;
        const uint8_t Constants::AC_MAX_INVERTER_POWER_LIMIT = AUX_AC_MAX_INVERTER_POWER_LIMIT;
        const uint32_t Constants::AC_STATES_REQUEST_INTERVAL = 7000;
        // таймаут загрузки пакета
        // По расчетам выходит:
        //      - получение и обработка посимвольно не должна длиться дольше 600 мсек.
        //      - получение и обработка пакетов целиком не должна длиться дольше 150 мсек.
        // Мы будем обрабатывать пакетами, поэтому 150.
        // Растягивать приём пакетов очередью команд нельзя, так как кондиционер иногда посылает
        // информационные пакеты без запроса. Такие пакеты будут рушить последовательность команд,
        // команды будут теряться. От такой коллизии мы не защищены в любом случае. Но чем меньше таймаут,
        // тем меньше шансов на коллизию.
        // Из этих соображений выбраны границы диапазона (_MIN и _MAX значения).
        // AUX_AC_PACKET_TIMEOUT_MAX and AUX_AC_PACKET_TIMEOUT_MIN will be defined by the ESPHome code generator at compile time
        const uint32_t Constants::AC_PACKET_TIMEOUT_MAX = AUX_AC_PACKET_TIMEOUT_MAX;
        const uint32_t Constants::AC_PACKET_TIMEOUT_MIN = AUX_AC_PACKET_TIMEOUT_MIN;

        //****************************************************************************************************************************************************
        //********************************************************* ОСНОВНЫЕ СТРУКТУРЫ ***********************************************************************
        //****************************************************************************************************************************************************
        class AirCon;

        // состояния конечного автомата компонента
        enum acsm_state : uint8_t
        {
            ACSM_IDLE = 0,         // ничего не делаем, ждем, на что бы среагировать
            ACSM_RECEIVING_PACKET, // находимся в процессе получения пакета, никакие отправки в этом состоянии невозможны
            ACSM_PARSING_PACKET,   // разбираем полученный пакет
            ACSM_SENDING_PACKET,   // отправляем пакет сплиту
        };

// структура пакета описана тут:
// https://github.com/GrKoR/AUX_HVAC_Protocol#packet_structure
#define AC_HEADER_SIZE 8

// стандартно длина пакета не более 34 байт
// но встретилось исключение Royal Clima (как минимум, модель CO-D xxHNI) - у них 35 байт
// поэтому буффер увеличен
#define AC_BUFFER_SIZE 35

// типы пакетов
// https://github.com/GrKoR/AUX_HVAC_Protocol#packet_types
#define AC_PTYPE_PING 0x01 // ping-пакет
#define AC_PTYPE_CMD 0x06  // команда сплиту
#define AC_PTYPE_INFO 0x07 // информационный пакет
#define AC_PTYPE_INIT 0x09 // инициирующий пакет
#define AC_PTYPE_UNKN 0x0b // какой-то странный пакет

// типы команд
// смотреть тут: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_type_cmd
#define AC_CMD_SET_PARAMS 0x01   // команда установки параметров кондиционера
#define AC_CMD_STATUS_SMALL 0x11 // маленький пакет статуса кондиционера
#define AC_CMD_STATUS_BIG 0x21   // большой пакет статуса кондиционера
// TODO: Нужно посмотреть, где используется AC_CMD_STATUS_PERIODIC, и изменить логику.
// на сегодня уже известно, что периодически рассылаются команды в диапазоне 0x20..0x2F
#define AC_CMD_STATUS_PERIODIC 0x2C // иногда встречается

// значения байтов в пакетах
#define AC_PACKET_START_BYTE 0xBB // Стартовый байт любого пакета 0xBB, других не встречал
#define AC_PACKET_ANSWER 0x80     // признак ответа wifi-модуля

        // заголовок пакета
        // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_header
        struct packet_header_t
        {
            uint8_t start_byte = AC_PACKET_START_BYTE;
            uint8_t _unknown1;
            uint8_t packet_type;
            uint8_t wifi;
            uint8_t ping_answer_01;
            uint8_t _unknown2;
            uint8_t body_length;
            uint8_t _unknown3;
        };

        // CRC пакета
        // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_crc
        union packet_crc_t
        {
            uint16_t crc16;
            uint8_t crc[2];
        };

        // структура пекета[)
        struct packet_t
        {
            uint32_t msec; // значение millis в момент определения корректности пакета
            packet_header_t *header;
            packet_crc_t *crc;
            uint8_t *body;       // указатель на первый байт тела; можно приведением типов указателей обращаться к отдельным битам как к полям соответсвующей структуры
            uint8_t bytesLoaded; // количество загруженных в пакет байт, включая CRC
            uint8_t data[AC_BUFFER_SIZE];
        };

        // тело ответа на пинг
        // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_type_ping
        struct packet_ping_answer_body_t
        {
            uint8_t byte_1C = 0x1C;
            uint8_t byte_27 = 0x27;
            uint8_t zero1 = 0;
            uint8_t zero2 = 0;
            uint8_t zero3 = 0;
            uint8_t zero4 = 0;
            uint8_t zero5 = 0;
            uint8_t zero6 = 0;
        };

        // тело большого информационного пакета
        // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21
        struct packet_big_info_body_t
        {
            // байт 0 тела (байт 8 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b08
            uint8_t byte_01 = 0x01;

            // байт 1 тела (байт 9 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b09
            uint8_t cmd_answer;

            // байт 2 тела (байт 10 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b10
            uint8_t reserv20 : 2;
            bool is_inverter_periodic : 1; // флаг периодического пакета инверторного кондиционера
            uint8_t reserv23 : 2;
            bool is_inverter : 1; // флаг инвертора
            uint8_t reserv26 : 2;

            // байт 3 тела (байт 11 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b11
            bool power : 1;
            bool sleep : 1;
            bool v_louver : 1;
            bool h_louver : 1;
            bool louvers_on : 1;
            uint8_t mode : 3;
            //   #define AC_BIG_MASK_MODE  b11100000
            //   enum { AC_BIG_MODE_DRY  = 0x40,
            //          AC_BIG_MODE_COOL = 0x20,
            //          AC_BIG_MODE_HEAT = 0x80,
            //          AC_BIG_MODE_FAN  = 0xC0}
            //   #define AC_BIG_MASK_POWER       b00000001
            //   #define AC_BIG_MASK_LOUVERS_ON  b00010000
            //   #define AC_BIG_MASK_LOUVERS_H   b00000100
            //   #define AC_BIG_MASK_LOUVERS_L   b00001000
            //   #define AC_BIG_MASK_SLEEP       b00000010
            //   #define AC_BIG_MASK_COOL        b00100000

            // байт 4 тела (байт 12 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b12
            uint8_t reserv40 : 4;
            bool needDefrost : 1;
            bool defrostMode : 1;
            bool reserv46 : 1;
            bool clean : 1;
            // Для кондея старт-стоп
            //    x xx
            // C5 1100 0101
            // C4 1100 0100
            // 85 1000 0101
            // 84 1000 0100
            // 3D 0011 1101
            // 3C 0011 1100
            // 25 0010 0101
            // 24 0010 0100
            //  5 0000 0101
            //  4 0000 0100

            // байт 5 тела (байт 13 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b13
            uint8_t realFanSpeed : 3; // реальная (не заданная) скорость вентилятора
            uint8_t reserv53 : 5;
            // в дежурных пакетах тут похоже что-то другое

            // байт 6 тела (байт 14 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b14

            bool reserv60 : 1;
            uint8_t fanPWM : 7; // ШИМ вентилятора

            // байт 7 тела (байт 15 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b15
            uint8_t ambient_temperature_int;

            // байт 8 тела (байт 16 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b16
            uint8_t zero3; // не расшифрован, у кого-то всегда 0x00, у кого-то повторяет значение байта 17 пакета. Непонятно.

            // байт 9 тела (байт 17 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b17
            uint8_t in_temperature_int; // какая-то температура, детали см. в описании на гитхабе

            // байт 10 тела (байт 18 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b18
            uint8_t zero4; // не расшифрован

            // байт 11 тела (байт 19 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b19
            uint8_t zero5; // всегда 0x00 или 0x64

            // байт 12 тела (байт 20 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b20
            uint8_t outdoor_temperature; // Внешняя температура; формула T - 0x20

            // байт 13 тела (байт 21 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b21
            uint8_t out_temperature_int; // похоже на температуру обратки, T - 0x20

            // байт 14 тела (байт 22 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b22
            uint8_t compressor_temperature_int; // от режима не зависит, растет при включении инвертора; температура двигателя?

            // байт 15 тела (байт 23 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b23
            uint8_t zero9; // не расшифрован, подробнее в описании

            // байт 16 тела (байт 24 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b24
            uint8_t inverter_power; // мощность инвертора (от 0 до 100) в %

            // байт 17 тела (байт 25 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b25
            uint8_t zero11; // не расшифрован, подробнее в описании.

            // байт 18 тела (байт 26 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b26
            uint8_t zero12; // не расшифрован, подробнее в описании.

            // байт 19 тела (байт 27 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b27
            uint8_t zero13; // не расшифрован, подробнее в описании.

            // байт 20 тела (байт 28 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b28
            uint8_t zero14; // не расшифрован, подробнее в описании.

            // байт 21 тела (байт 29 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b29
            uint8_t zero15; // всегда 0x00

            // байт 22 тела (байт 30 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b30
            uint8_t zero16; // всегда 0x00

            // байт 23 тела (байт 31 пакета)
            // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_21_b31
            uint8_t ambient_temperature_frac : 4; // дробная часть комнатной температуры воздуха с датчика на внутреннем блоке сплит-системы
            uint8_t reserv234 : 1;
            bool unknown : 1; // для `Royal Clima 18HNI` в этом бите `1`. Не понятно, что это значит. У других сплитов такое не встречалось.
            uint8_t reserv236 : 2;
        };

        // тело малого информационного пакета
        // https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11
        struct packet_small_info_body_t
        {
            // байт 8 пакета: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b08
            uint8_t byte_01;

            // байт 9 пакета: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b09
            uint8_t cmd_answer;

            // байт 10 пакета: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b10
            // uint8_t target_temp_int_and_v_louver;
            uint8_t v_louver : 3;
            uint8_t target_temp_int : 5;

            // байт 11 пакета: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b11
            uint8_t h_louver;

            // байт 12 пакета: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b12
            // uint8_t target_temp_frac;
            uint8_t ir_timer : 6;
            bool reserv126 : 1;
            bool target_temp_frac_bool : 1;

            // байт 13 пакета: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b13
            uint8_t fan_speed;

            // байт 14 пакета: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b14
            uint8_t fan_turbo_and_mute;

            // байт 15 пакета: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b15
            uint8_t mode;

            // байт 16 пакета: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b16
            uint8_t zero1; // всегда 0x00

            // байт 17 пакета: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b17
            uint8_t zero2; // всегда 0x00

            // байт 18 пакета: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b18
            uint8_t status;

            // байт 19 пакета: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b19
            uint8_t zero3; // всегда 0x00

            // байт 20 пакета: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b20
            uint8_t display_and_mildew;

            // байт 21 пакета: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b21
            uint8_t inverter_power_limitation_value : 7;
            bool inverter_power_limitation_enable : 1;

            // байт 22 пакета: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b22
            uint8_t target_temp_frac_dec;
        };

//****************************************************************************************************************************************************
//*************************************************** ПАРАМЕТРЫ РАБОТЫ КОНДИЦИОНЕРА ******************************************************************
//****************************************************************************************************************************************************
// для всех параметров ниже вариант X_UNTOUCHED = 0xFF означает, что этот параметр команды должен остаться тот, который уже установлен

// питание кондиционера
#define AC_POWER_MASK 0b00100000
        enum ac_power : uint8_t
        {
            AC_POWER_OFF = 0x00,
            AC_POWER_ON = 0x20,
            AC_POWER_UNTOUCHED = 0xFF
        };

// режим очистки кондиционера, включается (или должен включаться) при AC_POWER_OFF
#define AC_CLEAN_MASK 0b00000100
        enum ac_clean : uint8_t
        {
            AC_CLEAN_OFF = 0x00,
            AC_CLEAN_ON = 0x04,
            AC_CLEAN_UNTOUCHED = 0xFF
        };

// для включения ионизатора нужно установить второй бит в байте
// по результату этот бит останется установленным, но кондиционер еще и установит первый бит
#define AC_HEALTH_MASK 0b00000010
        enum ac_health : uint8_t
        {
            AC_HEALTH_OFF = 0x00,
            AC_HEALTH_ON = 0x02,
            AC_HEALTH_UNTOUCHED = 0xFF
        };

// Статус ионизатора. Если бит поднят, то обнаружена ошибка ключения ионизатора
#define AC_HEALTH_STATUS_MASK 0b00000001
        enum ac_health_status : uint8_t
        {
            AC_HEALTH_STATUS_OFF = 0x00,
            AC_HEALTH_STATUS_ON = 0x01,
            AC_HEALTH_STATUS_UNTOUCHED = 0xFF
        };

// целевая температура
#define AC_TEMP_TARGET_INT_PART_MASK 0b11111000
#define AC_TEMP_TARGET_FRAC_PART_MASK 0b10000000

// задержка отключения кондиционера
#define AC_TIMER_MINUTES_MASK 0b00111111
#define AC_TIMER_HOURS_MASK 0b00011111

// Temperature unit of measurement
// Air conditioner works with Celsius but can convert it to Fahrenheit for display on the LED screen.
#define AC_TEMPERATURE_UNIT_MASK 0b00000010
        enum ac_temperature_unit : uint8_t
        {
            AC_TEMPERATURE_UNIT_CELSIUS = 0x00,
            AC_TEMPERATURE_UNIT_FAHRENHEIT = 0x02,
            AC_TEMPERATURE_UNIT_UNTOUCHED = 0xFF
        };

// включение таймера сна
#define AC_TIMER_MASK 0b01000000
        enum ac_timer : uint8_t
        {
            AC_TIMER_OFF = 0x00,
            AC_TIMER_ON = 0x40,
            AC_TIMER_UNTOUCHED = 0xFF
        };

// основные режимы работы кондиционера
#define AC_MODE_MASK 0b11100000
        enum ac_mode : uint8_t
        {
            AC_MODE_AUTO = 0x00,
            AC_MODE_COOL = 0x20,
            AC_MODE_DRY = 0x40,
            AC_MODE_HEAT = 0x80,
            AC_MODE_FAN = 0xC0,
            AC_MODE_UNTOUCHED = 0xFF
        };

// Ночной режим (SLEEP). Комбинируется только с режимами COOL и HEAT. Автоматически выключается через 7 часов.
// COOL: температура +1 градус через час, еще через час дополнительные +1 градус, дальше не меняется.
// HEAT: температура -2 градуса через час, еще через час дополнительные -2 градуса, дальше не меняется.
// Восстанавливается ли температура через 7 часов при отключении режима - не понятно.
#define AC_SLEEP_MASK 0b00000100
        enum ac_sleep : uint8_t
        {
            AC_SLEEP_OFF = 0x00,
            AC_SLEEP_ON = 0x04,
            AC_SLEEP_UNTOUCHED = 0xFF
        };

// Вертикальные жалюзи. В протоколе зашита возможность двигать ими по всякому, но должна быть такая возможность на уровне железа.
#define AC_LOUVERV_MASK 0b00000111
        enum ac_louver_V : uint8_t
        {
            AC_LOUVERV_SWING_UPDOWN = 0x00,
            AC_LOUVERV_SWING_TOP = 0x01,
            AC_LOUVERV_SWING_MIDDLE_ABOVE = 0x02,
            AC_LOUVERV_SWING_MIDDLE = 0x03,
            AC_LOUVERV_SWING_MIDDLE_BELOW = 0x04,
            AC_LOUVERV_SWING_BOTTOM = 0x05,
            // 0x06 ничего не даёт, протестировано
            AC_LOUVERV_OFF = 0x07,
            AC_LOUVERV_UNTOUCHED = 0xFF
        };

// Горизонтальные жалюзи. В протоколе зашита возможность двигать ими по всякому, но должна быть такая возможность на уровне железа.
// горизонтальные жалюзи выставлять в определенное положение не вышло, протестировано.
#define AC_LOUVERH_MASK 0b11100000
        enum ac_louver_H : uint8_t
        {
            AC_LOUVERH_SWING_LEFTRIGHT = 0x00,
            AC_LOUVERH_OFF_AUX = 0x20,         // 0b00100000
            AC_LOUVERH_OFF_ALTERNATIVE = 0xE0, // 0b11100000 - по коду везде кроме проверок использую его, так как у него все три бита в 1
            AC_LOUVERH_UNTOUCHED = 0xFF
        };

        struct ac_louver
        {
            ac_louver_H louver_h;
            ac_louver_V louver_v;
        };

// скорость вентилятора
#define AC_FANSPEED_MASK 0b11100000
        enum ac_fanspeed : uint8_t
        {
            AC_FANSPEED_HIGH = 0x20,
            AC_FANSPEED_MEDIUM = 0x40,
            AC_FANSPEED_LOW = 0x60,
            AC_FANSPEED_AUTO = 0xA0,
            AC_FANSPEED_UNTOUCHED = 0xFF
        };

// TURBO работает только в режимах COOL и HEAT
#define AC_FANTURBO_MASK 0b01000000
        enum ac_fanturbo : uint8_t
        {
            AC_FANTURBO_OFF = 0x00,
            AC_FANTURBO_ON = 0x40,
            AC_FANTURBO_UNTOUCHED = 0xFF
        };

// MUTE работает только в режиме FAN. В режиме COOL кондей команду принимает, но MUTE не устанавливается
#define AC_FANMUTE_MASK 0b10000000
        enum ac_fanmute : uint8_t
        {
            AC_FANMUTE_OFF = 0x00,
            AC_FANMUTE_ON = 0x80,
            AC_FANMUTE_UNTOUCHED = 0xFF
        };

        // реальная скорость вентилятора
        enum ac_realFan : uint8_t
        {
            AC_REAL_FAN_OFF = 0x00,
            AC_REAL_FAN_MUTE = 0x01,
            AC_REAL_FAN_LOW = 0x02,
            AC_REAL_FAN_MID = 0x04,
            AC_REAL_FAN_HIGH = 0x06,
            AC_REAL_FAN_TURBO = 0x07,
            AC_REAL_FAN_UNTOUCHED = 0xFF
        };

// включение-выключение дисплея на корпусе внутреннего блока
#define AC_DISPLAY_MASK 0b00010000
        enum ac_display : uint8_t
        {
            AC_DISPLAY_OFF = 0x00,
            AC_DISPLAY_ON = 0x10,
            AC_DISPLAY_UNTOUCHED = 0xFF
        };

// включение-выключение функции "Антиплесень".
// По факту: после выключения сплита он оставляет минут на 5 открытые жалюзи и глушит вентилятор. Уличный блок при этом гудит и тарахтит.
// Возможно, прогревается теплообменник для высыхания. Через некоторое время внешний блок замолкает и сплит закрывает жалюзи.
#define AC_MILDEW_MASK 0b00001000
        enum ac_mildew : uint8_t
        {
            AC_MILDEW_OFF = 0x00,
            AC_MILDEW_ON = 0x08,
            AC_MILDEW_UNTOUCHED = 0xFF
        };

// маска счетчика минут прошедших с последней команды
// https://github.com/GrKoR/AUX_HVAC_Protocol#packet_cmd_11_b12
// GK: define убрал, т.к. считаю, что сбрасывать счетчик не надо.
// #define AC_MIN_COUNTER_MASK     0b00111111

// включение-выключение функции "Ограничение мощности".
#define AC_POWLIMSTAT_MASK 0b10000000
        enum ac_powLim_state : uint8_t
        {
            AC_POWLIMSTAT_OFF = 0x00,
            AC_POWLIMSTAT_ON = 0x80,
            AC_POWLIMSTAT_UNTOUCHED = 0xFF
        };

// маски ограничения мощности для инверторного кондиционера
#define AC_POWLIMVAL_MASK 0b01111111
#define AC_POWLIMVAL_UNTOUCHED 0xFF

        // положение вертикальных жалюзи для фронтенда
        enum ac_vlouver_frontend : uint8_t
        {
            AC_VLOUVER_FRONTEND_SWING = 0x00,
            AC_VLOUVER_FRONTEND_STOP = 0x01,
            AC_VLOUVER_FRONTEND_TOP = 0x02,
            AC_VLOUVER_FRONTEND_MIDDLE_ABOVE = 0x03,
            AC_VLOUVER_FRONTEND_MIDDLE = 0x04,
            AC_VLOUVER_FRONTEND_MIDDLE_BELOW = 0x05,
            AC_VLOUVER_FRONTEND_BOTTOM = 0x06,
        };

/** команда для кондиционера
 *
 * ВАЖНО! В коде используется копирование команд простым присваиванием.
 * Если в структуру будут введены указатели, то копирование надо будет изменить!
 */

//*****************************************************************************
// структура для сохранения настроек, специально вынесено в макрос, чтобы использовать в нескольких местах
// сделано Brokly для того, чтобы поведение wifi-модуля походило на ИК-пульт (для каждого режима сохранялись свои настройки температуры и прочего)
#define AC_COMMAND_BASE         \
    float temp_target;          \
    ac_power power;             \
    ac_clean clean;             \
    ac_health health;           \
    ac_mode mode;               \
    ac_temperature_unit t_unit; \
    ac_sleep sleep;             \
    ac_louver louver;           \
    ac_fanspeed fanSpeed;       \
    ac_fanturbo fanTurbo;       \
    ac_fanmute fanMute;         \
    ac_display display;         \
    ac_mildew mildew;           \
    ac_timer timer;             \
    uint8_t timer_hours;        \
    uint8_t timer_minutes;      \
    bool temp_target_matter

// чистый размер этой структуры 20 байт, скорее всего из-за выравнивания, она будет больше
// из-за такого приема нужно контролировать размер копируемых данных руками
#define AC_COMMAND_BASE_SIZE 20

#if defined(PRESETS_SAVING)
        // структура для сохранения данных
        struct ac_save_command_t
        {
            AC_COMMAND_BASE;
        };

        // номера сохранений пресетов
        enum store_pos : uint8_t
        {
            POS_MODE_AUTO = 0,
            POS_MODE_COOL,
            POS_MODE_DRY,
            POS_MODE_HEAT,
            POS_MODE_FAN,
            POS_MODE_OFF
        };
#endif
        //*****************************************************************************

        struct ac_command_t
        {
            AC_COMMAND_BASE;
            ac_health_status health_status;
            float temp_ambient;                      // внутренняя температура
            int8_t temp_outdoor;                     // внешняя температура
            int8_t temp_inbound;                     // температура входящая
            int8_t temp_outbound;                    // температура исходящая
            int8_t temp_compressor;                  // температура компрессора
            ac_realFan realFanSpeed;                 // текущая скорость вентилятора
            uint8_t inverter_power;                  // мощность инвертора
            bool defrost;                            // режим разморозки внешнего блока (накопление тепла + прогрев испарителя)
            ac_powLim_state power_lim_state;         // статус ограничения мощности инвертора
            uint8_t  power_lim_value;                // значение ограничения мощности инвертора
        };

        typedef ac_command_t ac_state_t; // текущее состояние параметров кондея можно хранить в таком же формате, как и комманды

        // Структура для хранения последних полученных от сплита информационных пакетов в сыром виде
        // Нужно до тех пор, пока весь функционал не разберем в структуру статуса.
        // Используем для проверки реакции сплита на команды (так отлавливаем разные версии протокола общения wifi-модуля с кондиционером)
        // Каждый пакет имеет поле msec. Если оно равно нулю, значит пакеты еще не принимались. По этому же полю можно смотреть, как давно
        // принималась информация от кондиционера, делать вывод об отвале и рапортовать об ошибке.
        struct ac_last_raw_data
        {
            packet_t last_small_info_packet;
            packet_t last_big_info_packet;
        };

//****************************************************************************************************************************************************
//************************************************ КОНЕЦ ПАРАМЕТРОВ РАБОТЫ КОНДИЦИОНЕРА **************************************************************
//****************************************************************************************************************************************************

/*****************************************************************************************************************************************************
 *                                      структуры и типы для последовательности команд
 *****************************************************************************************************************************************************
 *
 * Последовательность команд позволяет выполнить несколько последовательных команд с контролем получаемых в ответ пакетов.
 * Если требуется, в получаемых в ответ пакетах можно контролировать значение любых байт.
 * Для входящего пакета байт, значение которого не проверяется, должен быть установлен в AC_SEQUENCE_ANY_BYTE.
 * Контроль возможен только для входящих пакетов, исходящие отправляются "как есть".
 *
 * Для исходящих пакетов значения CRC могут не рассчитываться, контрольная сумма будет рассчитана автоматически.
 * Для входящих пакетов значение CRC также можно не рассчитывать, установив байты CRC в AC_SEQUENCE_ANY_BYTE,
 * так как контроль CRC для получаемых пакетов выполняется автоматически при получении.
 *
 * Для входящих пакетов в последовательности можно указать таймаут. Если таймаут равен 0, то используется значение AC_SEQUENCE_DEFAULT_TIMEOUT.
 * Если в течение указанного времени подходящий пакет не будет получен, то последовательность прерывается с ошибкой.
 * Пинг-пакеты в последовательности игнорируются.
 *
 * Пауза в последовательности задается значением timeout элемента AC_DELAY. Никакие другие параметры такого элемента можно не заполнять.
 *
 **/
// максимальная длина последовательности; больше вроде бы не требовалось
#define AC_SEQUENCE_MAX_LEN 0x0F

// дефолтный таймаут входящего пакета в миллисекундах
// если для входящего пакета в последовательности указан таймаут 0, то используется значение по-умолчанию
// если нужный пакет не поступил в течение указанного времени, то последовательность прерывается с ошибкой
#define AC_SEQUENCE_DEFAULT_TIMEOUT 580 // Brokly: пришлось увеличить с 500 до 580

        enum sequence_item_type_t : uint8_t
        {
            AC_SIT_NONE = 0x00,  // пустой элемент последовательности
            AC_SIT_DELAY = 0x01, // пауза в последовательности на нужное количество миллисекунд
            AC_SIT_FUNC = 0x02   // рабочий элемент последовательности
        };

        // тип пакета в массиве последовательности
        // информирует о том, что за пакет лежит в поле packet элемента последовательности
        enum sequence_packet_type_t : uint8_t
        {
            AC_SPT_CLEAR = 0x00,           // пустой пакет
            AC_SPT_RECEIVED_PACKET = 0x01, // полученный пакет
            AC_SPT_SENT_PACKET = 0x02      // отправленный пакет
        };

        /** элемент последовательности
         *  Поля item_type, func, timeout и cmd устанавливаются ручками и задают параметры выполнения шага последовательности.
         *  Поля msec, packet_type и packet заполняются движком при обработке последовательности.
         **/
        struct sequence_item_t
        {
            sequence_item_type_t item_type; // тип элемента последовательности
            bool (AirCon::*func)();         // указатель на функцию, отрабатывающую шаг последовательности
            uint16_t timeout;               // допустимый таймаут в ожидании пакета (применим только для входящих пакетов)
            ac_command_t cmd;               // новое состояние сплита, нужно для передачи кондиционеру команд
            //******* поля ниже заполняются функциями обработки последовательности ***********
            uint32_t msec;                      // время старта текущего шага последовательности (для входящего пакета и паузы)
            sequence_packet_type_t packet_type; // тип пакета (входящий, исходящий или вовсе не пакет)
            packet_t packet;                    // данные пакета
        };
        /*****************************************************************************************************************************************************/

        class AirCon : public esphome::Component, public esphome::climate::Climate
        {
        private:
#if defined(PRESETS_SAVING)
            // массив для сохранения данных глобальных персетов
            ac_save_command_t global_presets[POS_MODE_OFF + 1];

            // тут будем хранить данные глобальных пресетов во флеше
            // ВНИМАНИЕ на данный момент 22.05.22 ESPHOME 20022.5.0 имеет ошибку
            // траблтикет:   https://github.com/esphome/issues/issues/3298
            // из-за этого сохранение в энергонезависимую память не работает !!!
            ESPPreferenceObject storage = global_preferences->make_preference<ac_save_command_t[POS_MODE_OFF + 1]>(this->get_object_id_hash(), true);

            // настройка-ключ, для включения сохранения - восстановления настроек каждого
            // режима работы в отдельности, то есть каждый режим работы имеет свои настройки
            // температуры, шторок, скорости вентилятора, пресетов
            bool _store_settings = false;
            // флаги для сохранения пресетов
            bool _new_command_set = false; // флаг отправки новой команды, необходимо сохранить данные пресета, если разрешено
#endif

            // время последнего запроса статуса у кондея
            uint32_t _dataMillis;
            // периодичность обновления статуса кондея, по дефолту AC_STATES_REQUEST_INTERVAL
            uint32_t _update_period = Constants::AC_STATES_REQUEST_INTERVAL;

            // надо ли отображать текущий режим работы внешнего блока
            // в режиме нагрева, например, кондиционер может как греть воздух, так и работать в режиме вентилятора, если целевая темпреатура достигнута
            // по дефолту показываем
            bool _show_action = true;

            // как отрабатывается включание-выключение дисплея.
            // если тут false, то 1 в соответствующем бите включает дисплей, а 0 выключает.
            // если тут true, то 1 потушит дисплей, а 0 включит.
            bool _display_inverted = false;

            // in optimistic mode, the entity states are updated immediately after receiving a command
            // from Home Assistant/ESPHome
            bool _optimistic = true;

            // флаг типа кондиционера. инвертор - true,  ON/OFF - false, начальная установка false
            // в таком режиме точность и скорость определения реального состояния системы для инвертора,
            // будет работать, но будет ниже, переменная устанавливается при первом получении большого пакета;
            // если эта переменная установлена, то режим работы не инверторного кондиционера будет распознаваться
            // как "в простое" (IDLE)
            bool _is_inverter = false;

            // поддерживаемые кондиционером опции
            std::set<ClimateMode> _supported_modes{};
            std::set<ClimateSwingMode> _supported_swing_modes{};
            std::set<ClimatePreset> _supported_presets{};
            std::set<std::string> _supported_custom_presets{};
            std::set<std::string> _supported_custom_fan_modes{};

            // The capabilities of the climate device
            // Шаблон параметров отображения виджета
            esphome::climate::ClimateTraits _traits;

            // состояние конечного автомата
            acsm_state _ac_state = ACSM_IDLE;

            // текущее состояние задаваемых пользователем параметров системы
            ac_state_t _current_ac_state;

            // флаг подключения к UART
            bool _hw_initialized = false;
            // указатель на UART, по которому общаемся с кондиционером
            esphome::uart::UARTComponent *_ac_serial;

            // UART wrappers: peek
            int peek()
            {
                uint8_t data;
                if (!_ac_serial->peek_byte(&data))
                    return -1;
                return data;
            }

            // UART wrappers: read
            int read()
            {
                uint8_t data;
                if (!_ac_serial->read_byte(&data))
                    return -1;
                return data;
            }

            // флаг обмена пакетами с кондиционером (если проходят пинги, значит есть коннект)
            bool _has_connection = false;

            // входящий и исходящий пакеты
            packet_t _inPacket;
            packet_t _outPacket;

            // пакет для тестирования всякой фигни
            packet_t _outTestPacket;

            // таймаут загрузки пакета, по дефолту минимальный
            uint32_t _packet_timeout = Constants::AC_PACKET_TIMEOUT_MIN;

            // сырые данные последних полученных большого и маленького информационных пакетов
            ac_last_raw_data _last_raw_data;

            // нормализация показаний температуры, приведение в диапазон
            float _temp_target_normalise(float temp)
            {
                auto traits = this->get_traits();
                float temp_min = traits.get_visual_min_temperature();
                float temp_max = traits.get_visual_max_temperature();
                if (temp < temp_min)
                    temp = temp_min;
                if (temp > temp_max)
                    temp = temp_max;
                if (temp < Constants::AC_MIN_TEMPERATURE)
                    temp = Constants::AC_MIN_TEMPERATURE;
                if (temp > Constants::AC_MAX_TEMPERATURE)
                    temp = Constants::AC_MAX_TEMPERATURE;
                return temp;
            }

            // нормализация лимита ограничения мощности инвертора, приведение в диапазон
            uint8_t _power_limitation_value_normalise(uint8_t power_limitation_value)
            {
                if (power_limitation_value < Constants::AC_MIN_INVERTER_POWER_LIMIT)
                    power_limitation_value = Constants::AC_MIN_INVERTER_POWER_LIMIT;
                if (power_limitation_value > Constants::AC_MAX_INVERTER_POWER_LIMIT)
                    power_limitation_value = Constants::AC_MAX_INVERTER_POWER_LIMIT;
                return power_limitation_value;
            }

            // последовательность пакетов текущий шаг в последовательности
            sequence_item_t _sequence[AC_SEQUENCE_MAX_LEN];
            uint8_t _sequence_current_step;

            // флаг успешного выполнения стартовой последовательности команд
            bool _startupSequenceComplete = false;

            // очистка последовательности команд
            void _clearSequence()
            {
                for (uint8_t i = 0; i < AC_SEQUENCE_MAX_LEN; i++)
                {
                    _sequence[i].item_type = AC_SIT_NONE;
                    _sequence[i].func = nullptr;
                    _sequence[i].timeout = 0;
                    _sequence[i].msec = 0;
                    _sequence[i].packet_type = AC_SPT_CLEAR;
                    _clearPacket(&_sequence[i].packet);
                    _clearCommand(&_sequence[i].cmd);
                }
                _sequence_current_step = 0;
            }

            // проверяет, есть ли свободные шаги в последовательности команд
            bool _hasFreeSequenceStep()
            {
                return (_getNextFreeSequenceStep() < AC_SEQUENCE_MAX_LEN);
            }

            // возвращает индекс первого пустого шага последовательности команд
            uint8_t _getNextFreeSequenceStep()
            {
                for (size_t i = 0; i < AC_SEQUENCE_MAX_LEN; i++)
                {
                    if (_sequence[i].item_type == AC_SIT_NONE)
                    {
                        return i;
                    }
                }
                // если свободных слотов нет, то возвращаем значение за пределом диапазона
                return AC_SEQUENCE_MAX_LEN;
            }

            // возвращает количество свободных шагов в последовательности
            uint8_t _getFreeSequenceSpace()
            {
                return (AC_SEQUENCE_MAX_LEN - _getNextFreeSequenceStep());
            }

            // добавляет шаг в последовательность команд
            // возвращает false, если не нашлось места для шага
            bool _addSequenceStep(const sequence_item_type_t item_type, bool (AirCon::*func)() = nullptr, ac_command_t *cmd = nullptr, uint16_t timeout = AC_SEQUENCE_DEFAULT_TIMEOUT)
            {
                if (!_hasFreeSequenceStep())
                    return false; // если места нет, то уходим
                if (item_type == AC_SIT_NONE)
                    return false; // глупость какая-то, уходим
                if ((item_type == AC_SIT_FUNC) && (func == nullptr))
                    return false; // должна быть передана функция для такого типа шага
                if ((item_type != AC_SIT_DELAY) && (item_type != AC_SIT_FUNC))
                {
                    // какой-то неизвестный тип
                    _debugMsg(F("_addSequenceStep: unknown sequence item type = %u"), ESPHOME_LOG_LEVEL_DEBUG, __LINE__, item_type);
                    return false;
                }

                uint8_t step = _getNextFreeSequenceStep();

                _sequence[step].item_type = item_type;

                // если задержка нулевая, то присваиваем дефолтную задержку
                if (timeout == 0)
                    timeout = AC_SEQUENCE_DEFAULT_TIMEOUT;
                _sequence[step].timeout = timeout;

                _sequence[step].func = func;
                if (cmd != nullptr)
                    _sequence[step].cmd = *cmd; // так как в структуре команды только простые типы, то можно вот так присваивать

                return true;
            }

            // добавляет в последовательность шаг с задержкой
            bool _addSequenceDelayStep(uint16_t timeout)
            {
                return this->_addSequenceStep(AC_SIT_DELAY, nullptr, nullptr, timeout);
            }

            // добавляет в последовательность функциональный шаг
            bool _addSequenceFuncStep(bool (AirCon::*func)(), ac_command_t *cmd = nullptr, uint16_t timeout = AC_SEQUENCE_DEFAULT_TIMEOUT)
            {
                return this->_addSequenceStep(AC_SIT_FUNC, func, cmd, timeout);
            }

            // выполняет всю логику очередного шага последовательности команд
            void _doSequence()
            {
                if (!hasSequence())
                    return;

                // если шаг уже максимальный из возможных
                if (_sequence_current_step >= AC_SEQUENCE_MAX_LEN)
                {
                    // значит последовательность закончилась, надо её очистить
                    // при очистке последовательности будет и _sequence_current_step обнулён
                    _debugMsg(F("Sequence [step %u]: maximum step reached"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, _sequence_current_step);
                    _clearSequence();
                    return;
                }

                // смотрим тип текущего элемента в последовательности
                switch (_sequence[_sequence_current_step].item_type)
                {
                case AC_SIT_FUNC:
                {
                    // если указатель на функцию пустой, то прерываем последовательность
                    if (_sequence[_sequence_current_step].func == nullptr)
                    {
                        _debugMsg(F("Sequence [step %u]: function pointer is NULL, sequence broken"), ESPHOME_LOG_LEVEL_WARN, __LINE__, _sequence_current_step);
                        _clearSequence();
                        return;
                    }

                    // сохраняем время начала паузы
                    if (_sequence[_sequence_current_step].msec == 0)
                    {
                        _sequence[_sequence_current_step].msec = millis();
                        _debugMsg(F("Sequence [step %u]: step started"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, _sequence_current_step);
                    }

                    // если таймаут не указан, берем значение по-умолчанию
                    if (_sequence[_sequence_current_step].timeout == 0)
                        _sequence[_sequence_current_step].timeout = AC_SEQUENCE_DEFAULT_TIMEOUT;

                    // если время вышло, то отчитываемся в лог и очищаем последовательность
                    if (millis() - _sequence[_sequence_current_step].msec >= _sequence[_sequence_current_step].timeout)
                    {
                        _debugMsg(F("Sequence  [step %u]: step timed out (it took %u ms instead of %u ms)"), ESPHOME_LOG_LEVEL_WARN, __LINE__, _sequence_current_step, millis() - _sequence[_sequence_current_step].msec, _sequence[_sequence_current_step].timeout);
                        _clearSequence();
                        return;
                    }

                    // можно вызывать функцию
                    // она самомтоятельно загружает отправляемые/полученные пакеты в packet последовательности
                    // а также самостоятельно увеличивает счетчик шагов последовательности _sequence_current_step
                    // единственное исключение - таймауты
                    if (!(this->*_sequence[_sequence_current_step].func)())
                    {
                        _debugMsg(F("Sequence  [step %u]: error was occur in step function"), ESPHOME_LOG_LEVEL_WARN, __LINE__, _sequence_current_step, millis() - _sequence[_sequence_current_step].msec);
                        _clearSequence();
                        return;
                    }
                    break;
                }

                case AC_SIT_DELAY:
                { // это пауза в последовательности
                    // пауза задается параметром timeout элемента последовательности
                    // начало паузы сохраняется в параметре msec

                    // сохраняем время начала паузы
                    if (_sequence[_sequence_current_step].msec == 0)
                    {
                        _sequence[_sequence_current_step].msec = millis();
                        _debugMsg(F("Sequence [step %u]: begin delay (%u ms)"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, _sequence_current_step, _sequence[_sequence_current_step].timeout);
                    }

                    // если время вышло, то переходим на следующий шаг
                    if (millis() - _sequence[_sequence_current_step].msec >= _sequence[_sequence_current_step].timeout)
                    {
                        _debugMsg(F("Sequence  [step %u]: delay culminated (plan = %u ms, fact = %u ms)"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, _sequence_current_step, _sequence[_sequence_current_step].timeout, millis() - _sequence[_sequence_current_step].msec);
                        _sequence_current_step++;
                    }
                    break;
                }

                case AC_SIT_NONE: // шаги закончились
                default:          // или какой-то мусор в последовательности
                    // надо очистить последовательность и уходить
                    _debugMsg(F("Sequence [step %u]: sequence complete"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, _sequence_current_step);
                    _clearSequence();
                    break;
                }
            }

            // заполняет структуру команды нейтральными значениями
            void _clearCommand(ac_command_t *cmd)
            {
                cmd->clean = AC_CLEAN_UNTOUCHED;
                cmd->display = AC_DISPLAY_UNTOUCHED;
                cmd->fanMute = AC_FANMUTE_UNTOUCHED;
                cmd->fanSpeed = AC_FANSPEED_UNTOUCHED;
                cmd->fanTurbo = AC_FANTURBO_UNTOUCHED;
                cmd->health = AC_HEALTH_UNTOUCHED;
                cmd->health_status = AC_HEALTH_STATUS_UNTOUCHED;
                cmd->louver.louver_h = AC_LOUVERH_UNTOUCHED;
                cmd->louver.louver_v = AC_LOUVERV_UNTOUCHED;
                cmd->mildew = AC_MILDEW_UNTOUCHED;
                cmd->mode = AC_MODE_UNTOUCHED;
                cmd->t_unit = AC_TEMPERATURE_UNIT_UNTOUCHED;
                cmd->power = AC_POWER_UNTOUCHED;
                cmd->sleep = AC_SLEEP_UNTOUCHED;
                cmd->timer = AC_TIMER_UNTOUCHED;
                cmd->timer_hours = 0;
                cmd->timer_minutes = 0;
                cmd->temp_target = 0;
                cmd->temp_target_matter = false;
                cmd->temp_ambient = 0;
                cmd->temp_outdoor = 0;
                cmd->temp_inbound = 0;
                cmd->temp_outbound = 0;
                cmd->temp_compressor = 0;
                cmd->realFanSpeed = AC_REAL_FAN_UNTOUCHED;
                cmd->inverter_power = 0;
                cmd->defrost = false;
                cmd->power_lim_state = AC_POWLIMSTAT_UNTOUCHED;
                cmd->power_lim_value = AC_POWLIMVAL_UNTOUCHED;
            };

            // очистка буфера размером AC_BUFFER_SIZE
            void _clearBuffer(uint8_t *buf)
            {
                memset(buf, 0, AC_BUFFER_SIZE);
            }

            // очистка структуры пакета по указателю
            void _clearPacket(packet_t *pckt)
            {
                if (pckt == nullptr)
                {
                    _debugMsg(F("Clear packet error: pointer is NULL!"), ESPHOME_LOG_LEVEL_ERROR, __LINE__);
                    return;
                }
                pckt->crc = nullptr;
                pckt->header = (packet_header_t *)(pckt->data); // заголовок же всегда стартует с начала пакета
                pckt->msec = 0;
                pckt->bytesLoaded = 0;
                pckt->body = nullptr;
                _clearBuffer(pckt->data);
            }

            // очистка входящего пакета
            void _clearInPacket()
            {
                _clearPacket(&_inPacket);
            }

            // очистка исходящего пакета
            void _clearOutPacket()
            {
                _clearPacket(&_outPacket);
                _outPacket.header->start_byte = AC_PACKET_START_BYTE; // для исходящего сразу ставим стартовый байт
                _outPacket.header->wifi = AC_PACKET_ANSWER;           // для исходящего пакета сразу ставим признак ответа
            }

            // копирует пакет из одной структуры в другую с корректным переносом указателей на заголовки и т.п.
            bool _copyPacket(packet_t *dest, packet_t *src)
            {
                if (dest == nullptr)
                    return false;
                if (src == nullptr)
                    return false;

                dest->msec = src->msec;
                dest->bytesLoaded = src->bytesLoaded;
                memcpy(dest->data, src->data, AC_BUFFER_SIZE);
                dest->header = (packet_header_t *)&dest->data;
                if (dest->header->body_length > 0)
                    dest->body = &dest->data[AC_HEADER_SIZE];
                dest->crc = (packet_crc_t *)&dest->data[AC_HEADER_SIZE + dest->header->body_length];

                return true;
            }

            // устанавливает состояние конечного автомата
            // можно и напрямую устанавливать переменную, но для целей отладки лучше так
            void _setStateMachineState(acsm_state state = ACSM_IDLE)
            {
                if (_ac_state == state)
                    return; // состояние не меняется

                _ac_state = state;

                switch (state)
                {
                case ACSM_IDLE:
                    _debugMsg(F("State changed to ACSM_IDLE."), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);
                    break;

                case ACSM_RECEIVING_PACKET:
                    _debugMsg(F("State changed to ACSM_RECEIVING_PACKET."), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);
                    break;

                case ACSM_PARSING_PACKET:
                    _debugMsg(F("State changed to ACSM_PARSING_PACKET."), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);
                    break;

                case ACSM_SENDING_PACKET:
                    _debugMsg(F("State changed to ACSM_SENDING_PACKET."), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);
                    break;

                default:
                    _debugMsg(F("State changed to ACSM_IDLE by default. Given state is %02X."), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, state);
                    _ac_state = ACSM_IDLE;
                    break;
                }
            }

            // состояние конечного автомата: IDLE
            void _doIdleState()
            {
                // вначале нужно выполнить очередной шаг последовательности команд
                _doSequence();

                // Если нет входящих данных, значит можно отправить исходящий пакет, если он есть
                if (_ac_serial->available() == 0)
                {
                    // если есть пакет на отправку, то надо отправлять
                    // вначале думал, что сейчас отправка пакетов тут не нужна, т.к. состояние ACSM_SENDING_PACKET устанавливается сразу в парсере пакетов
                    // но потом понял, что у нас пакеты уходят не только когда надо отвечать, но и мы можем быть инициаторами
                    // поэтому вызов отправки тут пригодится
                    if (_outPacket.msec > 0)
                        _setStateMachineState(ACSM_SENDING_PACKET);
                    // больше дел нет - выходим
                    return;
                };

                if (this->peek() == AC_PACKET_START_BYTE)
                {
                    // если во входящий пакет что-то уже загружено, значит это какие-то ошибочные данные или неизвестные пакеты
                    // надо эту инфу вывалить в лог
                    if (_inPacket.bytesLoaded > 0)
                    {
                        _debugMsg(F("Start byte received but there are some unparsed bytes in the buffer:"), ESPHOME_LOG_LEVEL_DEBUG, __LINE__);
                        _debugPrintPacket(&_inPacket, ESPHOME_LOG_LEVEL_DEBUG, __LINE__);
                    }
                    _clearInPacket();
                    _inPacket.msec = millis();
                    _setStateMachineState(ACSM_RECEIVING_PACKET);
                }
                else
                {
                    while (_ac_serial->available() > 0)
                    {
                        // если наткнулись на старт пакета, то выходим из while
                        // если какие-то данные были загружены в буфер, то они будут выгружены в лог при загрузке нового пакета
                        if (this->peek() == AC_PACKET_START_BYTE)
                            break;

                        // читаем байт в буфер входящего пакета
                        _inPacket.data[_inPacket.bytesLoaded] = this->read();
                        _inPacket.bytesLoaded++;

                        // если буфер уже полон, надо его вывалить в лог и очистить
                        if (_inPacket.bytesLoaded >= AC_BUFFER_SIZE)
                        {
                            _debugMsg(F("Some unparsed data on the bus:"), ESPHOME_LOG_LEVEL_DEBUG, __LINE__);
                            _debugPrintPacket(&_inPacket, ESPHOME_LOG_LEVEL_DEBUG, __LINE__);
                            _clearInPacket();
                        }
                    }
                }
            };

            // состояние конечного автомата: ACSM_RECEIVING_PACKET
            void _doReceivingPacketState()
            {
                while (_ac_serial->available() > 0)
                {
                    // если в буфере пакета данных уже под завязку, то надо сообщить о проблеме и выйти
                    if (_inPacket.bytesLoaded >= AC_BUFFER_SIZE)
                    {
                        _debugMsg(F("Receiver: packet buffer overflow!"), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                        _debugPrintPacket(&_inPacket, ESPHOME_LOG_LEVEL_WARN, __LINE__);
                        _clearInPacket();
                        _setStateMachineState(ACSM_IDLE);
                        return;
                    }

                    _inPacket.data[_inPacket.bytesLoaded] = this->read();
                    _inPacket.bytesLoaded++;

                    // данных достаточно для заголовка
                    if (_inPacket.bytesLoaded == AC_HEADER_SIZE)
                    {
                        // указатель заголовка установлен еще при обнулении пакета, его можно не трогать
                        //_inPacket.header = (packet_header_t *)(_inPacket.data);

                        // уже знаем размер пакета и можем установить указатели на тело пакета и CRC
                        _inPacket.crc = (packet_crc_t *)&(_inPacket.data[AC_HEADER_SIZE + _inPacket.header->body_length]);
                        if (_inPacket.header->body_length > 0)
                            _inPacket.body = &(_inPacket.data[AC_HEADER_SIZE]);

                        _debugMsg(F("Header loaded: timestamp = %010u, start byte = %02X, packet type = %02X, body size = %02X"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, _inPacket.msec, _inPacket.header->start_byte, _inPacket.header->packet_type, _inPacket.header->body_length);
                    }

                    // если все байты пакета загружены, надо его распарсить
                    // максимальный по размеру пакет будет упираться в размер буфера. если такой пакет здесь не уйдет на парсинг,
                    // то на следующей итерации будет ошибка о переполнении буфера, которая в начале цикла while
                    if (_inPacket.bytesLoaded == AC_HEADER_SIZE + _inPacket.header->body_length + 2)
                    {
                        _debugMsg(F("Packet loaded: timestamp = %010u, start byte = %02X, packet type = %02X, body size = %02X, crc = [%02X, %02X]."), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, _inPacket.msec, _inPacket.header->start_byte, _inPacket.header->packet_type, _inPacket.header->body_length, _inPacket.crc->crc[0], _inPacket.crc->crc[1]);
                        _debugMsg(F("Loaded %02u bytes for a %u ms."), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, _inPacket.bytesLoaded, (millis() - _inPacket.msec));
                        _debugPrintPacket(&_inPacket, ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);
                        _setStateMachineState(ACSM_PARSING_PACKET);
                        return;
                    }
                }

                // если пакет не загружен, а время вышло, то надо вернуться в IDLE
                if (millis() - _inPacket.msec >= this->_packet_timeout)
                {
                    _debugMsg(F("Receiver: packet timed out!"), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    _debugPrintPacket(&_inPacket, ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    _clearInPacket();
                    _setStateMachineState(ACSM_IDLE);
                    return;
                }
            };

            // состояние конечного автомата: ACSM_PARSING_PACKET
            void _doParsingPacket()
            {
                if (!_checkCRC(&_inPacket))
                {
                    _debugMsg(F("Parser: packet CRC fail!"), ESPHOME_LOG_LEVEL_ERROR, __LINE__);
                    _debugPrintPacket(&_inPacket, ESPHOME_LOG_LEVEL_ERROR, __LINE__);
                    _clearInPacket();
                    _setStateMachineState(ACSM_IDLE);
                    return;
                }

                bool stateChangedFlag = false; // флаг, показывающий, изменилось ли состояние кондиционера
                uint8_t stateByte = 0;         // переменная для временного сохранения текущих параметров сплита для проверки их изменения
                float stateFloat = 0.0;        // переменная для временного сохранения текущих параметров сплита для проверки их изменения

                // вначале выводим полученный пакет в лог, чтобы он шел до информации об ответах и т.п.
                _debugPrintPacket(&_inPacket, ESPHOME_LOG_LEVEL_DEBUG, __LINE__);

                // разбираем тип пакета
                switch (_inPacket.header->packet_type)
                {
                case AC_PTYPE_PING:
                { // ping-пакет, рассылается кондиционером каждые 3 сек.; модуль на него отвечает

                    if (_inPacket.header->body_length != 0)
                    { // у входящего ping-пакета тело должно отсутствовать
                        // если тело есть, то жалуемся в лог
                        _debugMsg(F("Parser: ping packet should not to have body. Received one has body length %02X."), ESPHOME_LOG_LEVEL_WARN, __LINE__, _inPacket.header->body_length);
                        // очищаем пакет
                        _clearInPacket();
                        _setStateMachineState(ACSM_IDLE);
                        break;
                    }

                    _debugMsg(F("Parser: ping packet received"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);
                    // поднимаем флаг, что есть коннект с кондиционером
                    _has_connection = true;

                    // надо отправлять ответ на пинг
                    _clearOutPacket();
                    _outPacket.msec = millis();
                    _outPacket.header->packet_type = AC_PTYPE_PING;
                    _outPacket.header->ping_answer_01 = 0x01; // магия, детали тут: https://github.com/GrKoR/AUX_HVAC_Protocol#packet_type_ping
                    _outPacket.header->body_length = 8;
                    _outPacket.body = &(_outPacket.data[AC_HEADER_SIZE]);

                    // заполняем тело пакета
                    packet_ping_answer_body_t *ping_body;
                    ping_body = (packet_ping_answer_body_t *)(_outPacket.body);
                    ping_body->byte_1C = 0x1C;
                    ping_body->byte_27 = 0x27;

                    // расчет контрольной суммы и прописывание её в пакет
                    _outPacket.crc = (packet_crc_t *)&(_outPacket.data[AC_HEADER_SIZE + _outPacket.header->body_length]);
                    _setCRC16(&_outPacket);
                    _outPacket.bytesLoaded = AC_HEADER_SIZE + _outPacket.header->body_length + 2;

                    _debugMsg(F("Parser: generated ping answer. Waiting for sending."), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);

                    // до отправки пинг-ответа проверяем, не выполнялась ли стартовая последовательность команд
                    // по задумке она выполняется после подключения к кондиционеру после ответа на первый пинг
                    // нужна для максимально быстрого определния текущих параметров кондиционера
                    if (!_startupSequenceComplete)
                    {
                        _startupSequenceComplete = startupSequence();
                    }

                    _setStateMachineState(ACSM_SENDING_PACKET);

                    break;
                }

                case AC_PTYPE_CMD:
                { // команда сплиту; модуль отправляет такие команды, когда что-то хочет от сплита
                    //  сплит такие команды отправлять не должен, поэтому жалуемся в лог
                    _debugMsg(F("Parser: packet type=0x06 received from HVAC. This isn't expected."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    // очищаем пакет
                    _clearInPacket();
                    _setStateMachineState(ACSM_IDLE);
                    break;
                }

                case AC_PTYPE_INFO:
                { // информационный пакет
                    _debugMsg(F("Parser: status packet received"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);
                    // смотрим тип поступившего пакета по второму байту тела
                    // но вначале проверяем, что такое тело вообще есть
                    if ((_inPacket.body == nullptr) || (_inPacket.bytesLoaded < AC_HEADER_SIZE + 4) || (_inPacket.header->body_length < 2))
                    {
                        _debugMsg(F("Parser: packet type=0x07 without body. Error!"), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                        _clearInPacket();
                        _setStateMachineState(ACSM_IDLE);
                        break;
                    }
                    // теперь можно проверять второй байт тела пакета
                    switch (_inPacket.body[1])
                    {
                    case AC_CMD_STATUS_SMALL:
                    { // маленький пакет статуса кондиционера
                        _debugMsg(F("Parser: status packet type = small"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);
                        stateChangedFlag = false;

                        // будем обращаться к телу пакета через указатель на структуру
                        packet_small_info_body_t *small_info_body;
                        small_info_body = (packet_small_info_body_t *)(_inPacket.body);

                        // в малом пакете передается большое количество установленных пользователем параметров работы
                        // stateFloat = 8 + (small_info_body->target_temp_int_and_v_louver >> 3) + 0.5 * (float)(small_info_body->target_temp_frac >> 7);
                        stateFloat = 8.0 + (float)(small_info_body->target_temp_int) + ((small_info_body->target_temp_frac_bool) ? 0.5 : 0.0);
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.temp_target != stateFloat);
                        _current_ac_state.temp_target = stateFloat;
                        _current_ac_state.temp_target_matter = true;

                        // stateByte = small_info_body->target_temp_int_and_v_louver & AC_LOUVERV_MASK;
                        stateByte = small_info_body->v_louver;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.louver.louver_v != (ac_louver_V)stateByte);
                        _current_ac_state.louver.louver_v = (ac_louver_V)stateByte;

                        stateByte = small_info_body->h_louver & AC_LOUVERH_MASK;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.louver.louver_h != (ac_louver_H)stateByte);
                        _current_ac_state.louver.louver_h = (ac_louver_H)stateByte;

                        stateByte = small_info_body->fan_speed & AC_FANSPEED_MASK;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.fanSpeed != (ac_fanspeed)stateByte);
                        _current_ac_state.fanSpeed = (ac_fanspeed)stateByte;

                        stateByte = small_info_body->fan_turbo_and_mute & AC_FANTURBO_MASK;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.fanTurbo != (ac_fanturbo)stateByte);
                        _current_ac_state.fanTurbo = (ac_fanturbo)stateByte;

                        stateByte = small_info_body->fan_turbo_and_mute & AC_FANMUTE_MASK;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.fanMute != (ac_fanmute)stateByte);
                        _current_ac_state.fanMute = (ac_fanmute)stateByte;

                        stateByte = small_info_body->mode & AC_MODE_MASK;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.mode != (ac_mode)stateByte);
                        _current_ac_state.mode = (ac_mode)stateByte;

                        stateByte = small_info_body->mode & AC_TEMPERATURE_UNIT_MASK;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.t_unit != (ac_temperature_unit)stateByte);
                        _current_ac_state.t_unit = (ac_temperature_unit)stateByte;

                        stateByte = small_info_body->mode & AC_SLEEP_MASK;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.sleep != (ac_sleep)stateByte);
                        _current_ac_state.sleep = (ac_sleep)stateByte;

                        stateByte = small_info_body->status & AC_POWER_MASK;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.power != (ac_power)stateByte);
                        _current_ac_state.power = (ac_power)stateByte;

                        stateByte = small_info_body->status & AC_HEALTH_MASK;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.health != (ac_health)stateByte);
                        _current_ac_state.health = (ac_health)stateByte;

                        stateByte = small_info_body->status & AC_HEALTH_STATUS_MASK;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.health_status != (ac_health_status)stateByte);
                        _current_ac_state.health_status = (ac_health_status)stateByte;

                        stateByte = small_info_body->status & AC_CLEAN_MASK;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.clean != (ac_clean)stateByte);
                        _current_ac_state.clean = (ac_clean)stateByte;

                        stateByte = small_info_body->display_and_mildew & AC_DISPLAY_MASK;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.display != (ac_display)stateByte);
                        _current_ac_state.display = (ac_display)stateByte;

                        stateByte = small_info_body->display_and_mildew & AC_MILDEW_MASK;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.mildew != (ac_mildew)stateByte);
                        _current_ac_state.mildew = (ac_mildew)stateByte;
                        
                        stateByte = AC_POWLIMSTAT_ON * small_info_body->inverter_power_limitation_enable;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.power_lim_state != (ac_powLim_state)stateByte);
                        _current_ac_state.power_lim_state = (ac_powLim_state)stateByte;                        
                      
                        stateByte = small_info_body->inverter_power_limitation_value;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.power_lim_value != stateByte);
                        _current_ac_state.power_lim_value = stateByte;
                        
                        // уведомляем об изменении статуса сплита
                        if (stateChangedFlag)
                            stateChanged();

                        break;
                    }

                    case AC_CMD_STATUS_BIG: // большой пакет статуса кондиционера
                    case AC_CMD_STATUS_PERIODIC:
                    { // раз в 10 минут рассылается сплитом, структура аналогична большому пакету статуса
                        // TODO: вроде как AC_CMD_STATUS_PERIODIC могут быть и с другими кодами; пока что другие будут игнорироваться; если это будет критично, надо будет поправить
                        _debugMsg(F("Parser: status packet type = big or periodic"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);
                        stateChangedFlag = false;

                        // будем обращаться к телу пакета через указатель на структуру
                        packet_big_info_body_t *big_info_body;
                        big_info_body = (packet_big_info_body_t *)(_inPacket.body);

                        // тип кондея (инвертор или старт стоп)
                        _is_inverter = big_info_body->is_inverter;

                        // температура воздуха в помещении по версии сплит-системы
                        stateFloat = big_info_body->ambient_temperature_int - 0x20 + (float)(big_info_body->ambient_temperature_frac & 0x0f) / 10.0;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.temp_ambient != stateFloat);
                        _current_ac_state.temp_ambient = stateFloat;

                        // некая температура из наружного блока, скорее всего температура испарителя
                        // GK: фильтрацию тут убрал. Лучше это делать в ESPHome. Для этого у сенсора есть возможности. А тут лучше иметь чистые значения для аналлиза.
                        stateFloat = big_info_body->outdoor_temperature - 0x20;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.temp_outdoor != stateFloat);
                        _current_ac_state.temp_outdoor = stateFloat;

                        // температура входящей магистрали
                        stateFloat = big_info_body->in_temperature_int - 0x20;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.temp_inbound != stateFloat);
                        _current_ac_state.temp_inbound = stateFloat;

                        // температура исходящей магистрали
                        stateFloat = big_info_body->out_temperature_int - 0x20;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.temp_outbound != stateFloat);
                        _current_ac_state.temp_outbound = stateFloat;

                        // температура компрессора внешнего блока
                        stateFloat = big_info_body->compressor_temperature_int - 0x20;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.temp_compressor != stateFloat);
                        _current_ac_state.temp_compressor = stateFloat;

                        // реальная скорость пропеллера
                        stateFloat = big_info_body->realFanSpeed;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.realFanSpeed != (ac_realFan)stateFloat);
                        _current_ac_state.realFanSpeed = (ac_realFan)stateFloat;

                        // мощность инвертора
                        stateFloat = big_info_body->inverter_power;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.inverter_power != stateFloat);
                        _current_ac_state.inverter_power = stateFloat;

                        // режим разморозки
                        // bool temp = (big_info_body->needDefrost && big_info_body->defrostMode);
                        // TODO: need additional info for bit big_info_body->needDefrost
                        // Some HVACs use it but others don't (they use bit 3 instead of bit 4 (needDefrost))
                        bool temp = big_info_body->defrostMode;
                        stateChangedFlag = stateChangedFlag || (_current_ac_state.defrost != temp);
                        _current_ac_state.defrost = temp;

                        // уведомляем об изменении статуса сплита
                        if (stateChangedFlag)
                            stateChanged();

                        break;
                    }

                    case AC_CMD_SET_PARAMS:
                    { // такой статусный пакет присылается кондиционером в ответ на команду установки параметров
                        // в теле пакета нет ничего примечательного
                        // в байтах 2 и 3 тела передается CRC пакета поступившей команды, на которую сплит отвечает
                        // я решил этот момент тут не проверять и не контролировать.
                        // корректную установку параметров можно определить, запросив статус кондиционера сразу после получения этой команды кондея
                        // в настоящий момент проверка сделана в механизме sequences
                        break;
                    }

                    default:
                        _debugMsg(F("Parser: status packet type = unknown (%02X)"), ESPHOME_LOG_LEVEL_WARN, __LINE__, _inPacket.body[1]);
                        break;
                    }
                    _setStateMachineState(ACSM_IDLE);
                    break;
                }

                case AC_PTYPE_INIT: // инициирующий пакет; присылается сплитом, если кнопка HEALTH на пульте нажимается 8 раз; как там и что работает - не разбирался.
                case AC_PTYPE_UNKN: // какой-то странный пакет, отправляемый пультом при инициации и иногда при включении питания... как работает и зачем нужен - не разбирался, сплит на него вроде бы не реагирует
                default:
                    // игнорируем. Для нашего случая эти пакеты не важны
                    _setStateMachineState(ACSM_IDLE);
                    break;
                }

                // если есть последовательность команд, то надо отработать проверку последовательности
                if (hasSequence())
                    _doSequence();

                // после разбора входящего пакета его надо очистить
                _clearInPacket();
            }

            // состояние конечного автомата: ACSM_SENDING_PACKET
            void _doSendingPacketState()
            {
                // если нет исходящего пакета, то выходим
                if ((_outPacket.msec == 0) || (_outPacket.crc == nullptr) || (_outPacket.bytesLoaded == 0))
                {
                    _debugMsg(F("Sender: no packet to send."), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);
                    _setStateMachineState(ACSM_IDLE);
                    return;
                }

                _debugMsg(F("Sender: sending packet."), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);

                _ac_serial->write_array(_outPacket.data, _outPacket.bytesLoaded);
                _ac_serial->flush();

                _debugPrintPacket(&_outPacket, ESPHOME_LOG_LEVEL_DEBUG, __LINE__);
                _debugMsg(F("Sender: %u bytes sent (%u ms)."), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, _outPacket.bytesLoaded, millis() - _outPacket.msec);
                _clearOutPacket();

                _setStateMachineState(ACSM_IDLE);
            };

            /** вывод отладочной информации в лог
             *
             * dbgLevel - уровень сообщения, определен в ESPHome. За счет его использования можно из ESPHome управлять полнотой сведений в логе.
             * msg - сообщение, выводимое в лог
             * line - строка, на которой произошел вызов (удобно при отладке)
             */
            void _debugMsg(const String &msg, uint8_t dbgLevel = ESPHOME_LOG_LEVEL_DEBUG, unsigned int line = 0, ...)
            {
                if (dbgLevel < ESPHOME_LOG_LEVEL_NONE)
                    dbgLevel = ESPHOME_LOG_LEVEL_NONE;
                if (dbgLevel > ESPHOME_LOG_LEVEL_VERY_VERBOSE)
                    dbgLevel = ESPHOME_LOG_LEVEL_VERY_VERBOSE;

                if (line == 0)
                    line = __LINE__; // если строка не передана, берем текущую строку

                va_list vl;
                va_start(vl, line);
                esp_log_vprintf_(dbgLevel, TAG, line, msg.c_str(), vl);
                va_end(vl);
            }

            /** выводим данные пакета в лог для отладки
             *
             * dbgLevel - уровень сообщения, определен в ESPHome. За счет его использования можно из ESPHome управлять полнотой сведений в логе.
             * packet - указатель на пакет для вывода;
             *          если указатель на crc равен nullptr или первый байт в буфере не AC_PACKET_START_BYTE, то считаем, что передан битый пакет
             *          или не пакет вовсе. Для такого выводим только массив байт.
             *          Для нормального пакета данные выводятся с форматированием.
             * line - строка, на которой произошел вызов (удобно при отладке)
             **/
            void _debugPrintPacket(packet_t *packet, uint8_t dbgLevel = ESPHOME_LOG_LEVEL_DEBUG, unsigned int line = __LINE__)
            {
                // определяем, полноценный ли пакет нам передан
                bool notAPacket = false;
                // указатель заголовка всегда установден на начало буфера
                notAPacket = notAPacket || (packet->crc == nullptr);
                notAPacket = notAPacket || (packet->data[0] != AC_PACKET_START_BYTE);

                // если пакет по длине меньше, чем указано в фильтре, то не выводим.
                // если вывод пакетов отключен с помощью директивы HOLMES_WORKS, то тоже не выводим.
                // "не пакеты" выводим всегда, так как от них зависит отладка багов
                if ((!notAPacket) && (packet->header->body_length < HOLMES_FILTER_LEN))
                    return;
                if ((!notAPacket) && (!HOLMES_WORKS))
                    return;

                String st = "";
                char textBuf[11];

                // заполняем время получения пакета
                memset(textBuf, 0, 11);
                sprintf(textBuf, "%010u", packet->msec);
                st = st + textBuf + ": ";

                // формируем преамбулы
                if (packet == &_inPacket)
                {
                    st += "[<=] "; // преамбула входящего пакета
                }
                else if (packet == &_outPacket)
                {
                    st += "[=>] "; // преамбула исходящего пакета
                }
                else
                {
                    st += "[--] "; // преамбула для "непакета"
                }

                // формируем данные
                for (int i = 0; i < packet->bytesLoaded; i++)
                {
                    // для заголовков нормальных пакетов надо отработать скобки (если они есть)
                    if ((!notAPacket) && (i == 0))
                        st += HOLMES_HEADER_BRACKET_OPEN;
                    // для CRC нормальных пакетов надо отработать скобки (если они есть)
                    if ((!notAPacket) && (i == packet->header->body_length + AC_HEADER_SIZE))
                        st += HOLMES_CRC_BRACKET_OPEN;

                    memset(textBuf, 0, 11);
                    sprintf(textBuf, HOLMES_BYTE_FORMAT, packet->data[i]);
                    st += textBuf;

                    // для заголовков нормальных пакетов надо отработать скобки (если они есть)
                    if ((!notAPacket) && (i == AC_HEADER_SIZE - 1))
                        st += HOLMES_HEADER_BRACKET_CLOSE;
                    // для CRC нормальных пакетов надо отработать скобки (если они есть)
                    if ((!notAPacket) && (i == packet->header->body_length + AC_HEADER_SIZE + 2 - 1))
                        st += HOLMES_CRC_BRACKET_CLOSE;

                    st += HOLMES_DELIMITER;
                }

                _debugMsg(st, dbgLevel, line);
            }

            /** расчет CRC16 для блока данных data длиной len
             *      data    - данные для расчета CRC16, указатель на массив байт
             *      len     - длина блока данных для расчета, в байтах
             *
             * возвращаем uint16_t CRC16
             **/
            uint16_t _CRC16(uint8_t *data, uint8_t len)
            {
                uint32_t crc = 0;

                // выделяем буфер для расчета CRC и копируем в него переданные данные
                // это нужно для того, чтобы в случае нечетной длины данных можно было дополнить тело пакета
                // одним нулевым байтом и не попортить загруженный пакет (ведь в загруженном сразу за телом идёт CRC)
                uint8_t _crcBuffer[AC_BUFFER_SIZE];
                memset(_crcBuffer, 0, AC_BUFFER_SIZE);
                memcpy(_crcBuffer, data, len);

                // если длина данных нечетная, то надо сделать четной, дополнив данные в конце нулевым байтом
                // но так как выше буфер заполняли нулями, то отдельно тут присваивать 0x00 нет смысла
                if ((len % 2) == 1)
                    len++;

                // рассчитываем CRC16
                uint32_t word = 0;
                for (uint8_t i = 0; i < len; i += 2)
                {
                    word = (_crcBuffer[i] << 8) + _crcBuffer[i + 1];
                    crc += word;
                }
                crc = (crc >> 16) + (crc & 0xFFFF);
                crc = ~crc;

                return crc & 0xFFFF;
            }

            // расчитываем CRC16 и заполняем эти данные в структуре пакета
            void _setCRC16(packet_t *pack = nullptr)
            {
                // если пакет не указан, то устанавливаем CRC для исходящего пакета
                if (pack == nullptr)
                    pack = &_outPacket;

                packet_crc_t crc;
                crc.crc16 = _CRC16(pack->data, AC_HEADER_SIZE + pack->header->body_length);

                // если забыли указатель на crc установить, то устанавливаем
                if (pack->crc == nullptr)
                    pack->crc = (packet_crc_t *)&(pack->data[AC_HEADER_SIZE + pack->header->body_length]);

                pack->crc->crc[0] = crc.crc[1];
                pack->crc->crc[1] = crc.crc[0];
                return;
            }

            // проверяет CRC пакета по указателю
            bool _checkCRC(packet_t *pack = nullptr)
            {
                // если пакет не указан, то проверяем входящий
                if (pack == nullptr)
                    pack = &_inPacket;
                if (pack->bytesLoaded < AC_HEADER_SIZE)
                {
                    _debugMsg(F("CRC check: incoming packet size error."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    return false;
                }
                // если забыли указатель на crc установить, то устанавливаем
                if (pack->crc == nullptr)
                    pack->crc = (packet_crc_t *)&(pack->data[AC_HEADER_SIZE + pack->header->body_length]);

                packet_crc_t crc;
                crc.crc16 = _CRC16(pack->data, AC_HEADER_SIZE + pack->header->body_length);

                return ((pack->crc->crc[0] == crc.crc[1]) && (pack->crc->crc[1] == crc.crc[0]));
            }

            // заполняет пакет по ссылке командой запроса маленького пакета статуса
            void _fillStatusSmall(packet_t *pack = nullptr)
            {
                // по умолчанию заполняем исходящий пакет
                if (pack == nullptr)
                    pack = &_outPacket;

                // присваиваем параметры пакета
                pack->msec = millis();
                pack->header->start_byte = AC_PACKET_START_BYTE;
                pack->header->wifi = AC_PACKET_ANSWER; // для исходящего пакета ставим признак ответа
                pack->header->packet_type = AC_PTYPE_CMD;
                pack->header->body_length = 2; // тело команды 2 байта
                pack->body = &(pack->data[AC_HEADER_SIZE]);
                pack->body[0] = AC_CMD_STATUS_SMALL;
                pack->body[1] = 0x01; // он всегда 0x01
                pack->bytesLoaded = AC_HEADER_SIZE + pack->header->body_length + 2;

                // рассчитываем и записываем в пакет CRC
                pack->crc = (packet_crc_t *)&(pack->data[AC_HEADER_SIZE + pack->header->body_length]);
                _setCRC16(pack);
            }

            // заполняет пакет по ссылке командой запроса большого пакета статуса
            void _fillStatusBig(packet_t *pack = nullptr)
            {
                // по умолчанию заполняем исходящий пакет
                if (pack == nullptr)
                    pack = &_outPacket;

                // присваиваем параметры пакета
                pack->msec = millis();
                pack->header->start_byte = AC_PACKET_START_BYTE;
                pack->header->wifi = AC_PACKET_ANSWER; // для исходящего пакета ставим признак ответа
                pack->header->packet_type = AC_PTYPE_CMD;
                pack->header->body_length = 2; // тело команды 2 байта
                pack->body = &(pack->data[AC_HEADER_SIZE]);
                pack->body[0] = AC_CMD_STATUS_BIG;
                pack->body[1] = 0x01; // он всегда 0x01
                pack->bytesLoaded = AC_HEADER_SIZE + pack->header->body_length + 2;

                // рассчитываем и записываем в пакет CRC
                pack->crc = (packet_crc_t *)&(pack->data[AC_HEADER_SIZE + pack->header->body_length]);
                _setCRC16(pack);
            }

            /** заполняет пакет по ссылке командой установки параметров
             *
             * указатель на пакет может отсутствовать, тогда заполняется _outPacket
             * указатель на команду также может отсутствовать, тогда используется текущее состояние из _current_ac_state
             * все *__UNTOUCHED параметры заполняются из _current_ac_state
             **/
            void _fillSetCommand(bool clrPacket = false, packet_t *pack = nullptr, ac_state_t *cmd = nullptr)
            {
                // по умолчанию заполняем исходящий пакет
                if (pack == nullptr)
                    pack = &_outPacket;

                // очищаем пакет, если это указано
                if (clrPacket)
                    _clearPacket(pack);

                // заполняем его параметрами из _current_ac_state
                if (cmd != &_current_ac_state)
                    _fillSetCommand(false, pack, &_current_ac_state);

                // если команда не указана, значит выходим
                if (cmd == nullptr)
                    return;

                // команда указана, дополнительно внесем в пакет те параметры, которые установлены в команде
                // присваиваем параметры пакета
                pack->msec = millis();
                pack->header->start_byte = AC_PACKET_START_BYTE;
                pack->header->wifi = AC_PACKET_ANSWER; // для исходящего пакета ставим признак ответа
                pack->header->packet_type = AC_PTYPE_CMD;
                pack->header->body_length = 0x0F; // тело команды 15 (0x0F) байт, как у Small status
                pack->body = &(pack->data[AC_HEADER_SIZE]);
                pack->body[0] = AC_CMD_SET_PARAMS; // устанавливаем параметры
                pack->body[1] = 0x01;              // он всегда 0x01
                pack->bytesLoaded = AC_HEADER_SIZE + pack->header->body_length + 2;

                // целевая температура кондиционера
                if (cmd->temp_target_matter)
                {
                    // устраняем выход за границы диапазона (это ограничение самого кондиционера)
                    cmd->temp_target = _temp_target_normalise(cmd->temp_target);
                    // целая часть температуры
                    pack->body[2] = (pack->body[2] & ~AC_TEMP_TARGET_INT_PART_MASK) | (((uint8_t)(cmd->temp_target) - 8) << 3);

                    // дробная часть температуры
                    if (cmd->temp_target - (uint8_t)(cmd->temp_target) >= 0.5)
                    {
                        pack->body[4] = (pack->body[4] | AC_TEMP_TARGET_FRAC_PART_MASK);
                    }
                    else
                    {
                        pack->body[4] = (pack->body[4] & ~AC_TEMP_TARGET_FRAC_PART_MASK);
                    }
                }

                // значение ограничения мощности инвертора
                if ((cmd->power_lim_value != AC_POWLIMVAL_UNTOUCHED))
                {
                    cmd->power_lim_value = _power_limitation_value_normalise(cmd->power_lim_value);
                    pack->body[13] = (pack->body[13] & ~AC_POWLIMVAL_MASK) | (cmd->power_lim_value & AC_POWLIMVAL_MASK);
                }

                // включение/выключение ограничения мощности инвертора
                if ((cmd->power_lim_state != AC_POWLIMSTAT_UNTOUCHED))
                {
                    pack->body[13] = (pack->body[13] & ~AC_POWLIMSTAT_MASK) | (cmd->power_lim_state & AC_POWLIMSTAT_MASK);
                }

                //  обнулить счетчик минут с последней команды
                // GK: считаю, что так делать не надо. Штатный wifi-модуль не сбрасывает счетчик минут.
                // pack->body[4] &= ~ AC_MIN_COUNTER_MASK ;

                // вертикальные жалюзи
                if (cmd->louver.louver_v != AC_LOUVERV_UNTOUCHED)
                {
                    pack->body[2] = (pack->body[2] & ~AC_LOUVERV_MASK) | cmd->louver.louver_v;
                }

                // горизонтальные жалюзи
                if (cmd->louver.louver_h != AC_LOUVERH_UNTOUCHED)
                {
                    pack->body[3] = (pack->body[3] & ~AC_LOUVERH_MASK) | cmd->louver.louver_h;
                }

                // скорость вентилятора
                if (cmd->fanSpeed != AC_FANSPEED_UNTOUCHED)
                {
                    pack->body[5] = (pack->body[5] & ~AC_FANSPEED_MASK) | cmd->fanSpeed;
                }

                // спец.режимы вентилятора: TURBO
                if (cmd->fanTurbo != AC_FANTURBO_UNTOUCHED)
                {
                    pack->body[6] = (pack->body[6] & ~AC_FANTURBO_MASK) | cmd->fanTurbo;
                }

                // спец.режимы вентилятора: MUTE
                if (cmd->fanMute != AC_FANMUTE_UNTOUCHED)
                {
                    pack->body[6] = (pack->body[6] & ~AC_FANMUTE_MASK) | cmd->fanMute;
                }

                // режим кондея
                if (cmd->mode != AC_MODE_UNTOUCHED)
                {
                    pack->body[7] = (pack->body[7] & ~AC_MODE_MASK) | cmd->mode;
                }
                if (cmd->t_unit != AC_TEMPERATURE_UNIT_UNTOUCHED)
                {
                    pack->body[7] = (pack->body[7] & ~AC_TEMPERATURE_UNIT_MASK) | cmd->t_unit;
                }
                if (cmd->sleep != AC_SLEEP_UNTOUCHED)
                {
                    pack->body[7] = (pack->body[7] & ~AC_SLEEP_MASK) | cmd->sleep;
                }

                // питание вкл/выкл
                if (cmd->power != AC_POWER_UNTOUCHED)
                {
                    pack->body[10] = (pack->body[10] & ~AC_POWER_MASK) | cmd->power;
                }

                // просушка
                if (cmd->clean != AC_CLEAN_UNTOUCHED)
                {
                    pack->body[10] = (pack->body[10] & ~AC_CLEAN_MASK) | cmd->clean;
                }

                // ионизатор
                if (cmd->health != AC_HEALTH_UNTOUCHED)
                {
                    pack->body[10] = (pack->body[10] & ~AC_HEALTH_MASK) | cmd->health;
                }

                // какой то флаг ионизатора
                if (cmd->health_status != AC_HEALTH_STATUS_UNTOUCHED)
                {
                    pack->body[10] = (pack->body[10] & ~AC_HEALTH_STATUS_MASK) | cmd->health_status;
                }

                // дисплей
                if (cmd->display != AC_DISPLAY_UNTOUCHED)
                {
                    pack->body[12] = (pack->body[12] & ~AC_DISPLAY_MASK) | cmd->display;
                }

                // антиплесень
                if (cmd->mildew != AC_MILDEW_UNTOUCHED)
                {
                    pack->body[12] = (pack->body[12] & ~AC_MILDEW_MASK) | cmd->mildew;
                }

                // рассчитываем и записываем в пакет CRC
                pack->crc = (packet_crc_t *)&(pack->data[AC_HEADER_SIZE + pack->header->body_length]);
                _setCRC16(pack);
            }

            // отправка запроса на маленький статусный пакет
            bool sq_requestSmallStatus()
            {
                // если исходящий пакет не пуст, то выходим и ждем освобождения
                if (_outPacket.bytesLoaded > 0)
                    return true;

                _fillStatusSmall(&_outPacket);
                _fillStatusSmall(&_sequence[_sequence_current_step].packet);
                _sequence[_sequence_current_step].packet_type = AC_SPT_SENT_PACKET;

                // Отчитываемся в лог
                _debugMsg(F("Sequence [step %u]: small status request generated:"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, _sequence_current_step);
                _debugPrintPacket(&_outPacket, ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);

                // увеличиваем текущий шаг
                _sequence_current_step++;
                return true;
            }

            // проверка ответа на запрос маленького статусного пакета
            bool sq_controlSmallStatus()
            {
                // если по каким-то причинам нет входящего пакета, значит проверять нам нечего - просто выходим
                if (_inPacket.bytesLoaded == 0)
                    return true;

                // Пинги игнорируем
                if (_inPacket.header->packet_type == AC_PTYPE_PING)
                    return true;

                // сохраняем полученный пакет в последовательность, чтобы на возможных следующих шагах с ним можно было работать
                _copyPacket(&_sequence[_sequence_current_step].packet, &_inPacket);
                _sequence[_sequence_current_step].packet_type = AC_SPT_RECEIVED_PACKET;

                // проверяем ответ
                bool relevant = true;
                relevant = (relevant && (_inPacket.header->packet_type == AC_PTYPE_INFO));
                relevant = (relevant && (_inPacket.header->body_length == 0x0F));
                relevant = (relevant && (_inPacket.body[0] == 0x01));
                relevant = (relevant && (_inPacket.body[1] == AC_CMD_STATUS_SMALL));

                // если пакет подходит...
                if (relevant)
                {
                    // ...значит можно переходить к следующему шагу
                    // так как пакет корректный, то его можно скопировать в последние полученные пакеты
                    _copyPacket(&_last_raw_data.last_small_info_packet, &_inPacket);

                    // отчитываемся в лог и переходим к следующему шагу
                    _debugMsg(F("Sequence [step %u]: correct small status packet received"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, _sequence_current_step);
                    _sequence_current_step++;
                }
                else
                {
                    // если пакет не подходящий, то отчитываемся в лог...
                    _debugMsg(F("Sequence [step %u]: irrelevant incoming packet"), ESPHOME_LOG_LEVEL_WARN, __LINE__, _sequence_current_step);
                    _debugMsg(F("Incoming packet:"), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    _debugPrintPacket(&_inPacket, ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    _debugMsg(F("Sequence packet needed: PACKET_TYPE = %02X, CMD = %02X"), ESPHOME_LOG_LEVEL_WARN, __LINE__, AC_PTYPE_INFO, AC_CMD_STATUS_SMALL);
                    // ...и прерываем последовательность, так как вернем false
                }
                return relevant;
            }

            // отправка запроса на большой статусный пакет
            bool sq_requestBigStatus()
            {
                // если исходящий пакет не пуст, то выходим и ждем освобождения
                if (_outPacket.bytesLoaded > 0)
                    return true;

                _fillStatusBig(&_outPacket);
                _fillStatusBig(&_sequence[_sequence_current_step].packet);
                _sequence[_sequence_current_step].packet_type = AC_SPT_SENT_PACKET;

                // Отчитываемся в лог
                _debugMsg(F("Sequence [step %u]: big status request generated:"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, _sequence_current_step);
                _debugPrintPacket(&_outPacket, ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);

                // увеличиваем текущий шаг
                _sequence_current_step++;
                return true;
            }

            // проверка ответа на запрос большого статусного пакета
            bool sq_controlBigStatus()
            {
                // если по каким-то причинам нет входящего пакета, значит проверять нам нечего - просто выходим
                if (_inPacket.bytesLoaded == 0)
                    return true;

                // Пинги игнорируем
                if (_inPacket.header->packet_type == AC_PTYPE_PING)
                    return true;

                // сохраняем полученный пакет в последовательность, чтобы на возможных следующих шагах с ним можно было работать
                _copyPacket(&_sequence[_sequence_current_step].packet, &_inPacket);
                _sequence[_sequence_current_step].packet_type = AC_SPT_RECEIVED_PACKET;

                // проверяем ответ
                bool relevant = true;
                relevant = (relevant && (_inPacket.header->packet_type == AC_PTYPE_INFO));
                relevant = (relevant && (_inPacket.header->body_length == 0x18 || _inPacket.header->body_length == 0x19)); // канальник Royal Clima отвечает пакетом длиной 0x19
                relevant = (relevant && (_inPacket.body[0] == 0x01));
                relevant = (relevant && (_inPacket.body[1] == AC_CMD_STATUS_BIG));

                // если пакет подходит...
                if (relevant)
                {
                    // ...значит можно переходить к следующему шагу
                    // так как пакет корректный, то его можно скопировать в последние полученные пакеты
                    _copyPacket(&_last_raw_data.last_big_info_packet, &_inPacket);

                    // отчитываемся в лог и переходим к следующему шагу
                    _debugMsg(F("Sequence [step %u]: correct big status packet received"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, _sequence_current_step);
                    _sequence_current_step++;
                }
                else
                {
                    // если пакет не подходящий, то отчитываемся в лог...
                    _debugMsg(F("Sequence [step %u]: irrelevant incoming packet"), ESPHOME_LOG_LEVEL_WARN, __LINE__, _sequence_current_step);
                    _debugMsg(F("Incoming packet:"), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    _debugPrintPacket(&_inPacket, ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    _debugMsg(F("Sequence packet needed: PACKET_TYPE = %02X, CMD = %02X"), ESPHOME_LOG_LEVEL_WARN, __LINE__, AC_PTYPE_INFO, AC_CMD_STATUS_BIG);
                    // ...и прерываем последовательность
                }
                return relevant;
            }

            // отправка запроса на выполнение команды
            bool sq_requestDoCommand()
            {
                // если исходящий пакет не пуст, то выходим и ждем освобождения
                if (_outPacket.bytesLoaded > 0)
                    return true;

                _fillSetCommand(true, &_outPacket, &_sequence[_sequence_current_step].cmd);
                _fillSetCommand(true, &_sequence[_sequence_current_step].packet, &_sequence[_sequence_current_step].cmd);
                _sequence[_sequence_current_step].packet_type = AC_SPT_SENT_PACKET;

                // Отчитываемся в лог
                _debugMsg(F("Sequence [step %u]: doCommand request generated:"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, _sequence_current_step);
                _debugPrintPacket(&_outPacket, ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);

                // увеличиваем текущий шаг
                _sequence_current_step++;
                return true;
            }

            // проверка ответа на выполнение команды
            bool sq_controlDoCommand()
            {
                // если по каким-то причинам нет входящего пакета, значит проверять нам нечего - просто выходим
                if (_inPacket.bytesLoaded == 0)
                    return true;

                // Пинги игнорируем
                if (_inPacket.header->packet_type == AC_PTYPE_PING)
                    return true;

                // сохраняем полученный пакет в последовательность, чтобы на возможных следующих шагах с ним можно было работать
                _copyPacket(&_sequence[_sequence_current_step].packet, &_inPacket);
                _sequence[_sequence_current_step].packet_type = AC_SPT_RECEIVED_PACKET;

                // проверяем ответ
                bool relevant = true;
                relevant = (relevant && (_inPacket.header->packet_type == AC_PTYPE_INFO));
                relevant = (relevant && (_inPacket.header->body_length == 0x04));
                relevant = (relevant && (_inPacket.body[0] == 0x01));
                relevant = (relevant && (_inPacket.body[1] == AC_CMD_SET_PARAMS));
                // байты 2 и 3 обычно равны CRC отправленного пакета с командой
                relevant = (relevant && (_inPacket.body[2] == _sequence[_sequence_current_step - 1].packet.crc->crc[0]));
                relevant = (relevant && (_inPacket.body[3] == _sequence[_sequence_current_step - 1].packet.crc->crc[1]));

                // если пакет подходит, значит можно переходить к следующему шагу
                if (relevant)
                {
                    _debugMsg(F("Sequence [step %u]: correct doCommand packet received"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, _sequence_current_step);
                    _sequence_current_step++;
                }
                else
                {
                    // если пакет не подходящий, то отчитываемся в лог...
                    _debugMsg(F("Sequence [step %u]: irrelevant incoming packet"), ESPHOME_LOG_LEVEL_WARN, __LINE__, _sequence_current_step);
                    _debugMsg(F("Incoming packet:"), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    _debugPrintPacket(&_inPacket, ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    _debugMsg(F("Sequence packet needed: PACKET_TYPE = %02X, CMD = %02X"), ESPHOME_LOG_LEVEL_WARN, __LINE__, AC_PTYPE_INFO, AC_CMD_STATUS_BIG);
                    // ...и прерываем последовательность
                }
                return relevant;
            }

            // отправка запроса с тестовым пакетом
            bool sq_requestTestPacket()
            {
                // если исходящий пакет не пуст, то выходим и ждем освобождения
                if (_outPacket.bytesLoaded > 0)
                    return true;

                _copyPacket(&_outPacket, &_outTestPacket);
                _copyPacket(&_sequence[_sequence_current_step].packet, &_outTestPacket);
                _sequence[_sequence_current_step].packet_type = AC_SPT_SENT_PACKET;

                // Отчитываемся в лог
                _debugMsg(F("Sequence [step %u]: Test Packet request generated:"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, _sequence_current_step);
                _debugPrintPacket(&_outPacket, ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);

                // увеличиваем текущий шаг
                _sequence_current_step++;
                return true;
            }

            // сенсоры, отображающие параметры сплита
            esphome::sensor::Sensor *sensor_indoor_temperature_ = nullptr;
            esphome::sensor::Sensor *sensor_outdoor_temperature_ = nullptr;
            esphome::sensor::Sensor *sensor_inbound_temperature_ = nullptr;
            esphome::sensor::Sensor *sensor_outbound_temperature_ = nullptr;
            esphome::sensor::Sensor *sensor_compressor_temperature_ = nullptr;
            esphome::sensor::Sensor *sensor_inverter_power_ = nullptr;
            esphome::sensor::Sensor *sensor_vlouver_state_ = nullptr;
            esphome::binary_sensor::BinarySensor *sensor_display_ = nullptr;
            esphome::binary_sensor::BinarySensor *sensor_defrost_ = nullptr;
            esphome::text_sensor::TextSensor *sensor_preset_reporter_ = nullptr;
            esphome::sensor::Sensor *sensor_inverter_power_limit_value_ = nullptr;
            esphome::binary_sensor::BinarySensor *sensor_inverter_power_limit_state_ = nullptr;

            // загружает на выполнение последовательность команд на включение/выключение табло с температурой
            bool _displaySequence(ac_display dsp = AC_DISPLAY_ON)
            {
                // нет смысла в последовательности, если нет коннекта с кондиционером
                if (!get_has_connection())
                {
                    _debugMsg(F("displaySequence: no pings from HVAC. It seems like no AC connected."), ESPHOME_LOG_LEVEL_ERROR, __LINE__);
                    return false;
                }
                if (dsp == AC_DISPLAY_UNTOUCHED)
                    return false; // выходим, чтобы не тратить время

                // формируем команду
                ac_command_t cmd;
                _clearCommand(&cmd); // не забываем очищать, а то будет мусор
                cmd.display = dsp;
                // добавляем команду в последовательность
                if (!commandSequence(&cmd))
                    return false;

                _debugMsg(F("displaySequence: loaded (display = %02X)"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, dsp);
                return true;
            }

#if defined(PRESETS_SAVING)
            // номер глобального пресета от режима работы
            uint8_t get_num_preset(ac_command_t *cmd)
            {
                if (cmd->power == AC_POWER_OFF)
                {
                    return POS_MODE_OFF;
                }
                else if (cmd->mode == AC_MODE_AUTO)
                {
                    return POS_MODE_AUTO;
                }
                else if (cmd->mode == AC_MODE_COOL)
                {
                    return POS_MODE_COOL;
                }
                else if (cmd->mode == AC_MODE_DRY)
                {
                    return POS_MODE_DRY;
                }
                else if (cmd->mode == AC_MODE_FAN)
                {
                    return POS_MODE_FAN;
                }
                else if (cmd->mode == AC_MODE_HEAT)
                {
                    return POS_MODE_HEAT;
                }
                cmd->power = AC_POWER_OFF;
                return POS_MODE_OFF;
            }

            // восстановление данных из пресета
            void load_preset(ac_command_t *cmd, uint8_t num_preset)
            {
                if (num_preset < sizeof(global_presets) / sizeof(global_presets[0]))
                { // проверка выхода за пределы массива
                    if (cmd->power == global_presets[num_preset].power && cmd->mode == global_presets[num_preset].mode)
                    {                                                                     // контроль инициализации
                        memcpy(cmd, &(global_presets[num_preset]), AC_COMMAND_BASE_SIZE); // просто копируем из массива
                        _debugMsg(F("Preset %02d read from RAM massive."), ESPHOME_LOG_LEVEL_WARN, __LINE__, num_preset);
                    }
                    else
                    {
                        _debugMsg(F("Preset %02d not initialized, use current settings."), ESPHOME_LOG_LEVEL_WARN, __LINE__, num_preset);
                    }
                }
            }

            // запись данных в массив персетов
            void save_preset(ac_command_t *cmd)
            {
                uint8_t num_preset = get_num_preset(cmd);
                if (memcmp(cmd, &(global_presets[num_preset]), AC_COMMAND_BASE_SIZE) != 0)
                {                                                                     // содержимое пресетов разное
                    memcpy(&(global_presets[num_preset]), cmd, AC_COMMAND_BASE_SIZE); // копируем пресет в массив

                    _debugMsg(F("Save preset %02d to NVRAM."), ESPHOME_LOG_LEVEL_WARN, __LINE__, num_preset);
                    if (storage.save(global_presets))
                    {
                        if (!global_preferences->sync()) // сохраняем все пресеты
                            _debugMsg(F("Sync NVRAM error ! (load result: %02d)"), ESPHOME_LOG_LEVEL_ERROR, __LINE__, load_presets_result);
                    }
                    else
                    {
                        _debugMsg(F("Save presets to flash ERROR ! (load result: %02d)"), ESPHOME_LOG_LEVEL_ERROR, __LINE__, load_presets_result);
                    }
                }
                else
                {
                    _debugMsg(F("Preset %02d has not been changed, Saving canceled."), ESPHOME_LOG_LEVEL_WARN, __LINE__, num_preset);
                }
            }
#endif

        public:
            // инициализация объекта
            void initAC(esphome::uart::UARTComponent *parent = nullptr)
            {
                _dataMillis = millis();
                _clearInPacket();
                _clearOutPacket();
                _clearPacket(&_outTestPacket);
                _clearPacket(&_last_raw_data.last_big_info_packet);
                _clearPacket(&_last_raw_data.last_small_info_packet);

                _setStateMachineState(ACSM_IDLE);
                _ac_serial = parent;
                _hw_initialized = (_ac_serial != nullptr);
                _has_connection = false;
                _packet_timeout = Constants::AC_PACKET_TIMEOUT_MIN;

                // заполняем структуру состояния начальными значениями
                _clearCommand((ac_command_t *)&_current_ac_state);

                // очищаем последовательность пакетов
                _clearSequence();

                // выполнена ли уже стартовая последовательность команд (сбор информации о статусе кондея)
                _startupSequenceComplete = false;

                // первоначальная инициализация
                this->preset = climate::CLIMATE_PRESET_NONE;
                this->custom_preset = (std::string) "";
                this->mode = climate::CLIMATE_MODE_OFF;
                this->action = climate::CLIMATE_ACTION_IDLE;
                this->fan_mode = climate::CLIMATE_FAN_LOW;
                this->custom_fan_mode = (std::string) "";
            };

            float get_setup_priority() const override { return esphome::setup_priority::DATA; }

            void set_indoor_temperature_sensor(sensor::Sensor *temperature_sensor) { sensor_indoor_temperature_ = temperature_sensor; }
            void set_outdoor_temperature_sensor(sensor::Sensor *temperature_sensor) { sensor_outdoor_temperature_ = temperature_sensor; }
            void set_inbound_temperature_sensor(sensor::Sensor *temperature_sensor) { sensor_inbound_temperature_ = temperature_sensor; }
            void set_outbound_temperature_sensor(sensor::Sensor *temperature_sensor) { sensor_outbound_temperature_ = temperature_sensor; }
            void set_compressor_temperature_sensor(sensor::Sensor *temperature_sensor) { sensor_compressor_temperature_ = temperature_sensor; }
            void set_vlouver_state_sensor(sensor::Sensor *vlouver_state_sensor) { sensor_vlouver_state_ = vlouver_state_sensor; }
            void set_defrost_state(binary_sensor::BinarySensor *defrost_state_sensor) { sensor_defrost_ = defrost_state_sensor; }
            void set_display_sensor(binary_sensor::BinarySensor *display_sensor) { sensor_display_ = display_sensor; }
            void set_inverter_power_sensor(sensor::Sensor *inverter_power_sensor) { sensor_inverter_power_ = inverter_power_sensor; }
            void set_preset_reporter_sensor(text_sensor::TextSensor *preset_reporter_sensor) { sensor_preset_reporter_ = preset_reporter_sensor; }
            void set_inverter_power_limit_value_sensor(sensor::Sensor *inverter_power_limit_value_sensor) { sensor_inverter_power_limit_value_ = inverter_power_limit_value_sensor; }
            void set_inverter_power_limit_state_sensor(binary_sensor::BinarySensor *inverter_power_limit_state_sensor) { sensor_inverter_power_limit_state_ = inverter_power_limit_state_sensor; }

            bool get_hw_initialized() { return _hw_initialized; };
            bool get_has_connection() { return _has_connection; };

            // возвращает, есть ли елементы в последовательности команд
            bool hasSequence()
            {
                return (_sequence[0].item_type != AC_SIT_NONE);
            }

            // вызывается для публикации нового состояния кондиционера
            void stateChanged()
            {
                _debugMsg(F("State changed, let's publish it."), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);

                // экшины кондиционера (информация для пользователя, что кондиционер сейчас делает)
                // сейчас экшины рассчётные и могут не отражать реального положения дел, но других вариантов не придумалось
                if (_is_inverter)
                {
                    // анализ режима для инвертора точнее потому, что использует показания мощности инвертора
                    static uint32_t timerInv = 0;
                    if (_current_ac_state.inverter_power == 0)
                    { // инвертор выключен
                        timerInv = millis();
                        if (_current_ac_state.realFanSpeed == AC_REAL_FAN_OFF &&
                            _current_ac_state.power == AC_POWER_OFF)
                        {                                               // внутренний кулер остановлен, кондей выключен
                            this->action = climate::CLIMATE_ACTION_OFF; // значит кондей не работает
                        }
                        else
                        {
                            int16_t delta_temp = _current_ac_state.temp_ambient - _current_ac_state.temp_inbound;
                            if (delta_temp > 0 && delta_temp < 2 &&
                                (_current_ac_state.realFanSpeed == AC_REAL_FAN_OFF ||
                                 _current_ac_state.realFanSpeed == AC_REAL_FAN_MUTE ||
                                 _current_ac_state.realFanSpeed == AC_REAL_FAN_MUTE))
                            {
                                this->action = climate::CLIMATE_ACTION_DRYING; // ОСУШЕНИЕ
                            }
                            else if (_current_ac_state.realFanSpeed == AC_REAL_FAN_MUTE ||
                                     _current_ac_state.realFanSpeed == AC_REAL_FAN_OFF)
                            {                                                // кулер чуть вертится
                                this->action = climate::CLIMATE_ACTION_IDLE; // кондей в простое
                            }
                            else
                            {
                                this->action = climate::CLIMATE_ACTION_FAN; // другие режимы - вентиляция
                            }
                        }
                    }
                    else if (millis() - timerInv > 2000)
                    { // инвертор включен, но нужно дождаться реакции на его включение
                        if (_current_ac_state.realFanSpeed == AC_REAL_FAN_OFF ||
                            _current_ac_state.realFanSpeed == AC_REAL_FAN_MUTE)
                        { // медленное вращение
                            if (_current_ac_state.temp_ambient - _current_ac_state.temp_inbound > 0)
                            {                                                  // холодный радиатор
                                this->action = climate::CLIMATE_ACTION_DRYING; // ОСУШЕНИЕ
                            }
                            else
                            { // теплый радиатор, видимо переходный режим
                                this->action = climate::CLIMATE_ACTION_IDLE;
                            }
                        }
                        else
                        {
                            int16_t delta_temp = _current_ac_state.temp_ambient - _current_ac_state.temp_inbound;
                            if (delta_temp < -2)
                            { // входящая температура выше комнатной, быстрый фен - ОБОГРЕВ
                                this->action = climate::CLIMATE_ACTION_HEATING;
                            }
                            else if (delta_temp > 2)
                            { // ниже, быстрый фен - ОХЛАЖДЕНИЕ
                                this->action = climate::CLIMATE_ACTION_COOLING;
                            }
                            else
                            { // просто вентиляция
                                this->action = climate::CLIMATE_ACTION_IDLE;
                            }
                        }
                    }
                    else
                    {
                        if (_current_ac_state.realFanSpeed == AC_REAL_FAN_OFF ||
                            _current_ac_state.realFanSpeed == AC_REAL_FAN_MUTE)
                        {
                            this->action = climate::CLIMATE_ACTION_IDLE;
                        }
                        else
                        {
                            this->action = climate::CLIMATE_ACTION_FAN; // другие режимы - вентиляция
                        }
                    }
                }
                else
                { // if(_is_inverter)
                    // для on-off сплита рассчет экшена упрощен
                    if (_current_ac_state.realFanSpeed == AC_REAL_FAN_OFF &&
                        _current_ac_state.power == AC_POWER_OFF)
                    {
                        this->action = climate::CLIMATE_ACTION_OFF; // значит кондей не работает
                    }
                    else
                    {
                        int16_t delta_temp = _current_ac_state.temp_ambient - _current_ac_state.temp_inbound; // разность температуры между комнатной и входящей
                        if (delta_temp > 0 && delta_temp < 2 &&
                            (_current_ac_state.realFanSpeed == AC_REAL_FAN_OFF ||
                             _current_ac_state.realFanSpeed == AC_REAL_FAN_MUTE))
                        {
                            this->action = climate::CLIMATE_ACTION_DRYING; // ОСУШЕНИЕ
                        }
                        else if (_current_ac_state.realFanSpeed != AC_REAL_FAN_OFF &&
                                 _current_ac_state.realFanSpeed != AC_REAL_FAN_MUTE)
                        {
                            if (delta_temp > 2)
                            {
                                this->action = climate::CLIMATE_ACTION_COOLING;
                            }
                            else if (delta_temp < -2)
                            {
                                this->action = climate::CLIMATE_ACTION_HEATING;
                            }
                            else
                            {
                                this->action = climate::CLIMATE_ACTION_FAN; // другие режимы - вентиляция
                            }
                        }
                        else
                        {
                            this->action = climate::CLIMATE_ACTION_IDLE;
                        }
                    }
                }

                _debugMsg(F("Action mode: %i"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, this->action);

                /*************************** POWER & MODE ***************************/
                if (_current_ac_state.power == AC_POWER_ON)
                {
                    switch (_current_ac_state.mode)
                    {
                    case AC_MODE_AUTO:
                        // по факту режим, названный в AUX как AUTO, является режимом HEAT_COOL
                        this->mode = climate::CLIMATE_MODE_HEAT_COOL;
                        break;

                    case AC_MODE_COOL:
                        this->mode = climate::CLIMATE_MODE_COOL;
                        break;

                    case AC_MODE_DRY:
                        this->mode = climate::CLIMATE_MODE_DRY;
                        break;

                    case AC_MODE_HEAT:
                        this->mode = climate::CLIMATE_MODE_HEAT;
                        break;

                    case AC_MODE_FAN:
                        this->mode = climate::CLIMATE_MODE_FAN_ONLY;
                        break;

                    default:
                        _debugMsg(F("Warning: unknown air conditioner mode."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                        break;
                    }
                }
                else
                {
                    this->mode = climate::CLIMATE_MODE_OFF;
                }

                _debugMsg(F("Climate mode: %i"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, this->mode);

                /*************************** FAN SPEED ***************************/
                this->fan_mode = climate::CLIMATE_FAN_OFF;
                switch (_current_ac_state.fanSpeed)
                {
                case AC_FANSPEED_HIGH:
                    this->fan_mode = climate::CLIMATE_FAN_HIGH;
                    break;

                case AC_FANSPEED_MEDIUM:
                    this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
                    break;

                case AC_FANSPEED_LOW:
                    this->fan_mode = climate::CLIMATE_FAN_LOW;
                    break;

                case AC_FANSPEED_AUTO:
                    this->fan_mode = climate::CLIMATE_FAN_AUTO;
                    break;

                default:
                    _debugMsg(F("Warning: unknown fan speed."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    break;
                }

                _debugMsg(F("Climate fan mode: %i"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, this->fan_mode);

                /*************************** TURBO FAN MODE ***************************/
                // TURBO работает в режимах FAN, COOL, HEAT, HEAT_COOL
                // в режиме DRY изменение скорости вентилятора никак не влияло на его скорость, может сплит просто не вышел еще на режим? Надо попробовать долгую работу в этом режиме.
                switch (_current_ac_state.fanTurbo)
                {
                case AC_FANTURBO_ON:
                    // if ((_current_ac_state.mode == AC_MODE_HEAT) || (_current_ac_state.mode == AC_MODE_COOL)) {
                    this->custom_fan_mode = Constants::TURBO;
                    //}
                    break;

                case AC_FANTURBO_OFF:
                default:
                    if (this->custom_fan_mode == Constants::TURBO)
                        this->custom_fan_mode = (std::string) "";
                    break;
                }

                _debugMsg(F("Climate fan TURBO mode: %i"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, _current_ac_state.fanTurbo);

                /*************************** MUTE FAN MODE ***************************/
                // MUTE работает в режиме FAN. В режимах HEAT, COOL, HEAT_COOL не работает. DRY не проверял.
                // проверку на несовместимые режимы выпилили, т.к. нет уверенности, что это поведение одинаково для всех
                switch (_current_ac_state.fanMute)
                {
                case AC_FANMUTE_ON:
                    // if (_current_ac_state.mode == AC_MODE_FAN) {
                    this->custom_fan_mode = Constants::MUTE;
                    //}
                    break;

                case AC_FANMUTE_OFF:
                default:
                    if (this->custom_fan_mode == Constants::MUTE)
                        this->custom_fan_mode = (std::string) "";
                    break;
                }

                _debugMsg(F("Climate fan MUTE mode: %i"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, _current_ac_state.fanMute);

                //========================  ОТОБРАЖЕНИЕ ПРЕСЕТОВ ================================
                /*************************** HEALTH CUSTOM PRESET ***************************/
                // режим работы ионизатора
                if (_current_ac_state.health == AC_HEALTH_ON &&
                    _current_ac_state.power == AC_POWER_ON)
                {
                    this->custom_preset = Constants::HEALTH;
                }
                else if (this->custom_preset == Constants::HEALTH)
                {
                    // AC_HEALTH_OFF
                    // только в том случае, если до этого пресет был установлен
                    this->custom_preset = (std::string) "";
                }

                _debugMsg(F("Climate HEALTH preset: %i"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, _current_ac_state.health);

                /*************************** SLEEP PRESET ***************************/
                // Комбинируется только с режимами COOL и HEAT. Автоматически выключается через 7 часов.
                // COOL: температура +1 градус через час, еще через час дополнительные +1 градус, дальше не меняется.
                // HEAT: температура -2 градуса через час, еще через час дополнительные -2 градуса, дальше не меняется.
                // Восстанавливается ли температура через 7 часов при отключении режима - не понятно.
                if (_current_ac_state.sleep == AC_SLEEP_ON &&
                    _current_ac_state.power == AC_POWER_ON)
                {
                    this->preset = climate::CLIMATE_PRESET_SLEEP;
                }
                else if (this->preset == climate::CLIMATE_PRESET_SLEEP)
                {
                    // AC_SLEEP_OFF
                    // только в том случае, если до этого пресет был установлен
                    this->preset = climate::CLIMATE_PRESET_NONE;
                }

                _debugMsg(F("Climate preset: %i"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, this->preset);

                /*************************** CLEAN CUSTOM PRESET ***************************/
                // режим очистки кондиционера, включается (или должен включаться) при AC_POWER_OFF
                if (_current_ac_state.clean == AC_CLEAN_ON &&
                    _current_ac_state.power == AC_POWER_OFF)
                {
                    this->custom_preset = Constants::CLEAN;
                }
                else if (this->custom_preset == Constants::CLEAN)
                {
                    // AC_CLEAN_OFF
                    // только в том случае, если до этого пресет был установлен
                    this->custom_preset = (std::string) "";
                }

                _debugMsg(F("Climate CLEAN preset: %i"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, _current_ac_state.clean);

                /*************************** ANTIFUNGUS CUSTOM PRESET ***************************/
                // пресет просушки кондиционера после выключения
                // По факту: после выключения сплита он оставляет минут на 5 открытые жалюзи и глушит вентилятор.
                // Уличный блок при этом гудит и тарахтит. Возможно, прогревается теплообменник для высыхания.
                // Через некоторое время внешний блок замолкает и сплит закрывает жалюзи.
                //
                // Brokly:
                // У меня есть на этот режим, конедй реагирует только в выключеном состоянии. Причем пульт шлет
                // 5 посылок и при включении и при выключении. Но каких то видимых отличий в работе или в сценарии
                // при выключении кондея, я не наблюдаю. На пульте горит пиктограмма этого режима, но просушки
                // я не вижу. После выключения , с активированым режимом Anti-FUNGUS, кондей сразу закрывает хлебало
                // и затыкается.
                //
                // GK: оставил возможность включения функции в работающем состоянии, т.к. установка флага должна быть в работающем состоянии,
                // а сама функция отработает при выключении сплита.
                // У Brokly возможно какие-то особенности кондея.
                switch (_current_ac_state.mildew)
                {
                case AC_MILDEW_ON:
                    this->custom_preset = Constants::ANTIFUNGUS;
                    break;

                case AC_MILDEW_OFF:
                default:
                    if (this->custom_preset == Constants::ANTIFUNGUS)
                        this->custom_preset = (std::string) "";
                    break;
                }

                _debugMsg(F("Climate ANTIFUNGUS preset: %i"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, _current_ac_state.mildew);

                /*************************** LOUVERs ***************************/
                this->swing_mode = climate::CLIMATE_SWING_OFF;
                if (_current_ac_state.power == AC_POWER_ON)
                {
                    if (_current_ac_state.louver.louver_h == AC_LOUVERH_SWING_LEFTRIGHT && _current_ac_state.louver.louver_v == AC_LOUVERV_OFF)
                    {
                        this->swing_mode = climate::CLIMATE_SWING_HORIZONTAL;
                    }
                    else if (_current_ac_state.louver.louver_h == AC_LOUVERH_OFF_AUX && _current_ac_state.louver.louver_v == AC_LOUVERV_SWING_UPDOWN)
                    {
                        // TODO: КОСТЫЛЬ!
                        this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
                    }
                    else if (_current_ac_state.louver.louver_h == AC_LOUVERH_OFF_ALTERNATIVE && _current_ac_state.louver.louver_v == AC_LOUVERV_SWING_UPDOWN)
                    {
                        // TODO: КОСТЫЛЬ!
                        //       временно сделал так. Сделать нормально - это надо подумать.
                        //       На AUX и многих других марках выключенный режим горизонтальных жалюзи равен 0x20, а на ROVEX и Royal Clima 0xE0
                        //       Из-за этого происходил сброс на OFF во фронтенде Home Assistant. Пришлось городить это.
                        //       Надо как-то изящнее решить эту историю
                        this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
                    }
                    else if (_current_ac_state.louver.louver_h == AC_LOUVERH_SWING_LEFTRIGHT && _current_ac_state.louver.louver_v == AC_LOUVERV_SWING_UPDOWN)
                    {
                        this->swing_mode = climate::CLIMATE_SWING_BOTH;
                    }
                }

                _debugMsg(F("Climate swing mode: %i"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, this->swing_mode);

                /*************************** TEMPERATURE ***************************/
                this->target_temperature = _current_ac_state.temp_target;
                _debugMsg(F("Target temperature: %f"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, this->target_temperature);

                this->current_temperature = _current_ac_state.temp_ambient;
                _debugMsg(F("Room temperature: %f"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, this->current_temperature);

                /*********************************************************************/
                /*************************** PUBLISH STATE ***************************/
                /*********************************************************************/
                this->publish_all_states();
            }

            // публикуем все состояния сенсоров и сплита
            void publish_all_states()
            {
                this->publish_state();
                // температура в комнате
                if (sensor_indoor_temperature_ != nullptr)
                    sensor_indoor_temperature_->publish_state(_current_ac_state.temp_ambient);
                // температура уличного блока
                if (sensor_outdoor_temperature_ != nullptr)
                    sensor_outdoor_temperature_->publish_state(_current_ac_state.temp_outdoor);
                // температура подводящей магистрали
                if (sensor_inbound_temperature_ != nullptr)
                    sensor_inbound_temperature_->publish_state(_current_ac_state.temp_inbound);
                // температура отводящей магистрали
                if (sensor_outbound_temperature_ != nullptr)
                    sensor_outbound_temperature_->publish_state(_current_ac_state.temp_outbound);
                // температура странного датчика
                if (sensor_compressor_temperature_ != nullptr)
                    sensor_compressor_temperature_->publish_state(_current_ac_state.temp_compressor);
                // мощность инвертора
                if (sensor_inverter_power_ != nullptr)
                    sensor_inverter_power_->publish_state(_current_ac_state.inverter_power);
                // флаг режима разморозки
                if (sensor_defrost_ != nullptr)
                    sensor_defrost_->publish_state(_current_ac_state.defrost);
                // положение вертикальных жалюзи
                if (sensor_vlouver_state_ != nullptr)
                    sensor_vlouver_state_->publish_state((float)this->getCurrentVlouverFrontendState());
                // флаг включенного ограничения мощности инвертора
                if (sensor_inverter_power_limit_state_ != nullptr)
                    sensor_inverter_power_limit_state_->publish_state(_current_ac_state.power_lim_state == AC_POWLIMSTAT_ON);
                // значение ограничения мощности инвертора
                if (sensor_inverter_power_limit_value_ != nullptr)
                    sensor_inverter_power_limit_value_->publish_state(_current_ac_state.power_lim_value);
                // сенсор состояния сплита
                if (sensor_preset_reporter_ != nullptr)
                {
                    std::string state_str = "";
                    if (this->preset == climate::CLIMATE_PRESET_SLEEP)
                    {
                        state_str += "SLEEP";
                    }
                    else if (this->custom_preset.has_value() && this->custom_preset.value().length() > 0)
                    {
                        state_str += this->custom_preset.value().c_str();
                    }
                    else
                    {
                        state_str += "NONE";
                    }
                    sensor_preset_reporter_->publish_state(state_str.c_str());
                }

                // состояние дисплея
                if (sensor_display_ != nullptr)
                {
                    sensor_display_->publish_state((_current_ac_state.display == AC_DISPLAY_ON) ^ this->get_display_inverted());
                }
            }

            // вывод в дебаг текущей конфигурации компонента
            void dump_config()
            {
                ESP_LOGCONFIG(TAG, "AUX HVAC:");
                ESP_LOGCONFIG(TAG, "  [x] Firmware version: %s", Constants::AC_FIRMWARE_VERSION.c_str());
                ESP_LOGCONFIG(TAG, "  [x] Period: %dms", this->get_period());
                ESP_LOGCONFIG(TAG, "  [x] Show action: %s", TRUEFALSE(this->get_show_action()));
                ESP_LOGCONFIG(TAG, "  [x] Display inverted: %s", TRUEFALSE(this->get_display_inverted()));
                ESP_LOGCONFIG(TAG, "  [x] Optimistic: %s", TRUEFALSE(this->get_optimistic()));
                ESP_LOGCONFIG(TAG, "  [x] Packet timeout: %dms", this->get_packet_timeout());

#if defined(PRESETS_SAVING)
                ESP_LOGCONFIG(TAG, "  [x] Save settings %s", TRUEFALSE(this->get_store_settings()));
#endif

                ESP_LOGCONFIG(TAG, "  [?] Is inverter %s", millis() > _update_period + 1000 ? YESNO(_is_inverter) : "pending...");
                LOG_SENSOR("  ", "Inverter Power", this->sensor_inverter_power_);
                LOG_SENSOR("  ", "Inverter Power Limit Value", this->sensor_inverter_power_limit_value_);
                LOG_BINARY_SENSOR("  ", "Inverter Power Limit State", this->sensor_inverter_power_limit_state_);
                LOG_SENSOR("  ", "Indoor Temperature", this->sensor_indoor_temperature_);
                LOG_SENSOR("  ", "Outdoor Temperature", this->sensor_outdoor_temperature_);
                LOG_SENSOR("  ", "Inbound Temperature", this->sensor_inbound_temperature_);
                LOG_SENSOR("  ", "Outbound Temperature", this->sensor_outbound_temperature_);
                LOG_SENSOR("  ", "Compressor Temperature", this->sensor_compressor_temperature_);
                LOG_BINARY_SENSOR("  ", "Defrost Status", this->sensor_defrost_);
                LOG_BINARY_SENSOR("  ", "Display", this->sensor_display_);
                LOG_TEXT_SENSOR("  ", "Preset Reporter", this->sensor_preset_reporter_);
                this->dump_traits_(TAG);
            }

            // вызывается пользователем из интерфейса ESPHome или Home Assistant
            void control(const esphome::climate::ClimateCall &call) override
            {
                bool hasCommand = false;
                ac_command_t cmd;

                _clearCommand(&cmd); // не забываем очищать, а то будет мусор

                // User requested mode change
                if (call.get_mode().has_value())
                {
                    ClimateMode mode = *call.get_mode();

                    switch (mode)
                    {
                    case climate::CLIMATE_MODE_OFF:
                        hasCommand = true;
                        cmd.power = AC_POWER_OFF;

#if defined(PRESETS_SAVING)
                        load_preset(&cmd, POS_MODE_OFF);
#endif

                        this->mode = mode;
                        break;

                    case climate::CLIMATE_MODE_COOL:
                        hasCommand = true;
                        cmd.power = AC_POWER_ON;
                        cmd.mode = AC_MODE_COOL;

#if defined(PRESETS_SAVING)
                        load_preset(&cmd, POS_MODE_COOL);
#endif

                        this->mode = mode;
                        break;

                    case climate::CLIMATE_MODE_HEAT:
                        hasCommand = true;
                        cmd.power = AC_POWER_ON;
                        cmd.mode = AC_MODE_HEAT;

#if defined(PRESETS_SAVING)
                        load_preset(&cmd, POS_MODE_HEAT);
#endif

                        this->mode = mode;
                        break;

                    case climate::CLIMATE_MODE_HEAT_COOL:
                        hasCommand = true;
                        cmd.power = AC_POWER_ON;
                        cmd.mode = AC_MODE_AUTO;

#if defined(PRESETS_SAVING)
                        load_preset(&cmd, POS_MODE_AUTO);
#endif

                        this->mode = mode;
                        break;

                    case climate::CLIMATE_MODE_FAN_ONLY:
                        hasCommand = true;
                        cmd.power = AC_POWER_ON;
                        cmd.mode = AC_MODE_FAN;

#if defined(PRESETS_SAVING)
                        load_preset(&cmd, POS_MODE_FAN);
#endif

                        cmd.sleep = AC_SLEEP_OFF;
                        this->mode = mode;
                        break;

                    case climate::CLIMATE_MODE_DRY:
                        hasCommand = true;
                        cmd.power = AC_POWER_ON;
                        cmd.mode = AC_MODE_DRY;

#if defined(PRESETS_SAVING)
                        load_preset(&cmd, POS_MODE_DRY);
#endif

                        cmd.fanTurbo = AC_FANTURBO_OFF; // зависимость от режима DRY
                        cmd.sleep = AC_SLEEP_OFF;       // зависимость от режима DRY
                        this->mode = mode;
                        break;

                    // другие возможные значения (чтоб не забыть)
                    // case climate::CLIMATE_MODE_AUTO:        // этот режим в будущем можно будет использовать для автоматического пресета (ПИД-регулятора, например)
                    default:
                        break;
                    }
                }

                // User requested fan_mode change
                if (call.get_fan_mode().has_value())
                {
                    ClimateFanMode fanmode = *call.get_fan_mode();

                    switch (fanmode)
                    {
                    case climate::CLIMATE_FAN_AUTO:
                        hasCommand = true;
                        cmd.fanSpeed = AC_FANSPEED_AUTO;
                        cmd.fanTurbo = AC_FANTURBO_OFF;
                        cmd.fanMute = AC_FANMUTE_OFF;
                        this->fan_mode = fanmode;
                        break;

                    case climate::CLIMATE_FAN_LOW:
                        hasCommand = true;
                        cmd.fanSpeed = AC_FANSPEED_LOW;
                        cmd.fanTurbo = AC_FANTURBO_OFF;
                        cmd.fanMute = AC_FANMUTE_OFF;
                        this->fan_mode = fanmode;
                        break;

                    case climate::CLIMATE_FAN_MEDIUM:
                        hasCommand = true;
                        cmd.fanSpeed = AC_FANSPEED_MEDIUM;
                        cmd.fanTurbo = AC_FANTURBO_OFF;
                        cmd.fanMute = AC_FANMUTE_OFF;
                        this->fan_mode = fanmode;
                        break;

                    case climate::CLIMATE_FAN_HIGH:
                        hasCommand = true;
                        cmd.fanSpeed = AC_FANSPEED_HIGH;
                        cmd.fanTurbo = AC_FANTURBO_OFF;
                        cmd.fanMute = AC_FANMUTE_OFF;
                        this->fan_mode = fanmode;
                        break;

                    // другие возможные значения (чтобы не забыть)
                    // case climate::CLIMATE_FAN_ON:
                    // case climate::CLIMATE_FAN_OFF:
                    // case climate::CLIMATE_FAN_MIDDLE:
                    // case climate::CLIMATE_FAN_FOCUS:
                    // case climate::CLIMATE_FAN_DIFFUSE:
                    default:
                        break;
                    }
                }
                else if (call.get_custom_fan_mode().has_value())
                {
                    std::string customfanmode = *call.get_custom_fan_mode();

                    if (customfanmode == Constants::TURBO)
                    {
                        // TURBO fan mode is suitable in COOL and HEAT modes.
                        // Other modes don't accept TURBO fan mode.
                        /*
                        if (       cmd.mode == AC_MODE_COOL
                                or cmd.mode == AC_MODE_HEAT
                                or _current_ac_state.mode == AC_MODE_COOL
                                or _current_ac_state.mode == AC_MODE_HEAT) {
                        */
                        hasCommand = true;
                        cmd.fanTurbo = AC_FANTURBO_ON;
                        cmd.fanMute = AC_FANMUTE_OFF; // зависимость от fanturbo
                        this->custom_fan_mode = customfanmode;
                        /*
                        } else {
                            _debugMsg(F("TURBO fan mode is suitable in COOL and HEAT modes only."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                        }
                        */
                    }
                    else if (customfanmode == Constants::MUTE)
                    {
                        // MUTE fan mode is suitable in FAN mode only for Rovex air conditioner.
                        // In COOL mode AC receives command without any changes.
                        // May be other AUX-based air conditioners do the same.
                        // if (                     cmd.mode == AC_MODE_FAN
                        //        or _current_ac_state.mode == AC_MODE_FAN) {

                        hasCommand = true;
                        cmd.fanMute = AC_FANMUTE_ON;
                        cmd.fanTurbo = AC_FANTURBO_OFF; // зависимость от fanmute
                        this->custom_fan_mode = customfanmode;
                        //} else {
                        //    _debugMsg(F("MUTE fan mode is suitable in FAN mode only."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                        //}
                    }
                }

                // Пользователь выбрал пресет
                if (call.get_preset().has_value())
                {
                    ClimatePreset preset = *call.get_preset();

                    switch (preset)
                    {
                    case climate::CLIMATE_PRESET_SLEEP:
                        // Ночной режим (SLEEP).
                        // По инструкциям комбинируется только с режимами COOL и HEAT. Автоматически выключается через 7 часов.
                        // Brokly: вроде как работает еще и с AUTO и DRY
                        // COOL: температура +1 градус через час, еще через час дополнительные +1 градус, дальше не меняется.
                        // HEAT: температура -2 градуса через час, еще через час дополнительные -2 градуса, дальше не меняется.
                        // Восстанавливается ли температура через 7 часов при отключении режима - не понятно.
                        if (cmd.mode == AC_MODE_COOL or _current_ac_state.mode == AC_MODE_COOL or
                            cmd.mode == AC_MODE_HEAT or _current_ac_state.mode == AC_MODE_HEAT or
                            cmd.mode == AC_MODE_DRY or _current_ac_state.mode == AC_MODE_DRY or
                            cmd.mode == AC_MODE_AUTO or _current_ac_state.mode == AC_MODE_AUTO)
                        {
                            hasCommand = true;
                            cmd.sleep = AC_SLEEP_ON;
                            cmd.health = AC_HEALTH_OFF; // для логики пресетов
                            cmd.health_status = AC_HEALTH_STATUS_OFF;
                            this->preset = preset;
                        }
                        else
                        {
                            _debugMsg(F("SLEEP preset is suitable in COOL and HEAT modes only."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                        }
                        break;

                    case climate::CLIMATE_PRESET_NONE:
                        // выбран пустой пресет, сбрасываем все настройки
                        hasCommand = true;
                        cmd.health = AC_HEALTH_OFF;
                        // cmd.health_status = AC_HEALTH_STATUS_OFF;   // GK: не нужно ставить, т.к. этот флаг устанавливается самим сплитом
                        cmd.sleep = AC_SLEEP_OFF;
                        cmd.mildew = AC_MILDEW_OFF;
                        cmd.clean = AC_CLEAN_OFF;
                        this->preset = preset;

                        _debugMsg(F("Clear all builtin presets."), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);
                        break;
                    default:
                        // никакие другие встроенные пресеты не поддерживаются
                        _debugMsg(F("Preset %02X is unsupported."), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, preset);
                        break;
                    }
                }
                else if (call.get_custom_preset().has_value())
                {
                    std::string custom_preset = *call.get_custom_preset();

                    if (custom_preset == Constants::CLEAN)
                    {
                        // режим очистки кондиционера, включается (или должен включаться) при AC_POWER_OFF
                        // TODO: надо отдебажить выключение этого режима
                        if (cmd.power == AC_POWER_OFF or _current_ac_state.power == AC_POWER_OFF)
                        {
                            hasCommand = true;
                            cmd.clean = AC_CLEAN_ON;
                            cmd.mildew = AC_MILDEW_OFF; // для логики пресетов
                            this->custom_preset = custom_preset;
                        }
                        else
                        {
                            _debugMsg(F("CLEAN preset is suitable in POWER_OFF mode only."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                        }
                    }
                    else if (custom_preset == Constants::HEALTH)
                    {
                        if (cmd.power == AC_POWER_ON ||
                            _current_ac_state.power == AC_POWER_ON)
                        {
                            hasCommand = true;
                            cmd.health = AC_HEALTH_ON;
                            // cmd.health_status = AC_HEALTH_STATUS_ON;  // GK: статус кондей сам поднимает
                            cmd.fanTurbo = AC_FANTURBO_OFF; // зависимость от health
                            cmd.fanMute = AC_FANMUTE_OFF;   // зависимость от health
                            cmd.sleep = AC_SLEEP_OFF;       // для логики пресетов

                            if (cmd.mode == AC_MODE_COOL ||
                                cmd.mode == AC_MODE_HEAT ||
                                cmd.mode == AC_MODE_AUTO ||
                                _current_ac_state.mode == AC_MODE_COOL ||
                                _current_ac_state.mode == AC_MODE_HEAT ||
                                _current_ac_state.mode == AC_MODE_AUTO)
                            {
                                cmd.fanSpeed = AC_FANSPEED_AUTO; // зависимость от health
                            }
                            else if (cmd.mode == AC_MODE_FAN ||
                                     _current_ac_state.mode == AC_MODE_FAN)
                            {
                                cmd.fanSpeed = AC_FANSPEED_MEDIUM; // зависимость от health
                            }
                            this->custom_preset = custom_preset;
                        }
                        else
                        {
                            _debugMsg(F("HEALTH preset is suitable in POWER_ON mode only."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                        }
                    }
                    else if (custom_preset == Constants::ANTIFUNGUS)
                    {
                        // включение-выключение функции "Антиплесень".
                        // По факту: после выключения сплита он оставляет минут на 5 открытые жалюзи и глушит вентилятор.
                        // Уличный блок при этом гудит и тарахтит. Возможно, прогревается теплообменник для высыхания.
                        // Через некоторое время внешний блок замолкает и сплит закрывает жалюзи.

                        // Brokly:
                        // включение-выключение функции "Антиплесень".
                        // у меня пульт отправляет 5 посылок и на включение и на выключение, но реагирует на эту кнопку
                        // только в режиме POWER_OFF

                        // TODO: надо уточнить, в каких режимах штатно включается этот режим у кондиционера
                        cmd.mildew = AC_MILDEW_ON;
                        cmd.clean = AC_CLEAN_OFF; // для логики пресетов

                        hasCommand = true;
                        this->custom_preset = custom_preset;
                    }
                }

                // User requested swing_mode change
                if (call.get_swing_mode().has_value())
                {
                    ClimateSwingMode swingmode = *call.get_swing_mode();

                    switch (swingmode)
                    {
                    // The protocol allows other combinations for SWING.
                    // For example "turn the louvers to the desired position or "spread to the sides" / "concentrate in the center".
                    // But the ROVEX IR-remote does not provide this features. Therefore this features haven't been tested.
                    // May be suitable for other models of AUX-based ACs.
                    case climate::CLIMATE_SWING_OFF:
                        cmd.louver.louver_h = AC_LOUVERH_OFF_ALTERNATIVE;
                        cmd.louver.louver_v = AC_LOUVERV_OFF;
                        hasCommand = true;
                        this->swing_mode = swingmode;
                        break;

                    case climate::CLIMATE_SWING_BOTH:
                        cmd.louver.louver_h = AC_LOUVERH_SWING_LEFTRIGHT;
                        cmd.louver.louver_v = AC_LOUVERV_SWING_UPDOWN;
                        hasCommand = true;
                        this->swing_mode = swingmode;
                        break;

                    case climate::CLIMATE_SWING_VERTICAL:
                        cmd.louver.louver_h = AC_LOUVERH_OFF_ALTERNATIVE;
                        cmd.louver.louver_v = AC_LOUVERV_SWING_UPDOWN;
                        hasCommand = true;
                        this->swing_mode = swingmode;
                        break;

                    case climate::CLIMATE_SWING_HORIZONTAL:
                        cmd.louver.louver_h = AC_LOUVERH_SWING_LEFTRIGHT;
                        cmd.louver.louver_v = AC_LOUVERV_OFF;
                        hasCommand = true;
                        this->swing_mode = swingmode;
                        break;
                    }
                }

                // User requested target temperature change
                if (call.get_target_temperature().has_value())
                {
                    // выставлять температуру в режиме FAN не нужно
                    if (cmd.mode != AC_MODE_FAN && _current_ac_state.mode != AC_MODE_FAN)
                    {
                        hasCommand = true;
                        cmd.temp_target = _temp_target_normalise(*call.get_target_temperature()); // Send target temp to climate
                        cmd.temp_target_matter = true;
                    }
                }

                if (hasCommand)
                {
                    commandSequence(&cmd);
                    if (this->get_optimistic())
                    {
                        this->publish_all_states(); // Publish updated state
                    }

#if defined(PRESETS_SAVING)
                    // флаг отправки новой команды, для процедуры сохранения пресетов, если есть настройка
                    _new_command_set = _store_settings;
#endif
                }
            }

            // как оказалось сюда обращаются каждый раз для получения любого параметра
            // по этому имеет смысл держать готовый объект
            esphome::climate::ClimateTraits traits() override
            {
                return _traits;
            }

            // запрос маленького пакета статуса кондиционера
            bool getStatusSmall()
            {
                // нет смысла в последовательности, если нет коннекта с кондиционером
                if (!get_has_connection())
                {
                    _debugMsg(F("getStatusSmall: no pings from HVAC. It seems like no AC connected."), ESPHOME_LOG_LEVEL_ERROR, __LINE__);
                    return false;
                }
                // есть ли место на запрос в последовательности команд?
                if (_getFreeSequenceSpace() < 2)
                {
                    _debugMsg(F("getStatusSmall: not enough space in command sequence. Sequence steps doesn't loaded."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    return false;
                }

                /*************************************** getSmallInfo request ***********************************************/
                if (!_addSequenceFuncStep(&AirCon::sq_requestSmallStatus))
                {
                    _debugMsg(F("getStatusSmall: getSmallInfo request sequence step fail."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    return false;
                }
                /*************************************** getSmallInfo control ***********************************************/
                if (!_addSequenceFuncStep(&AirCon::sq_controlSmallStatus))
                {
                    _debugMsg(F("getStatusSmall: getSmallInfo control sequence step fail."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    return false;
                }
                /**************************************************************************************/

                _debugMsg(F("getStatusSmall: loaded to sequence"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);
                return true;
            }

            // запрос большого пакета статуса кондиционера
            bool getStatusBig()
            {
                // нет смысла в последовательности, если нет коннекта с кондиционером
                if (!get_has_connection())
                {
                    _debugMsg(F("getStatusBig: no pings from HVAC. It seems like no AC connected."), ESPHOME_LOG_LEVEL_ERROR, __LINE__);
                    return false;
                }
                // есть ли место на запрос в последовательности команд?
                if (_getFreeSequenceSpace() < 2)
                {
                    _debugMsg(F("getStatusBig: not enough space in command sequence. Sequence steps doesn't loaded."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    return false;
                }

                /*************************************** getBigInfo request ***********************************************/
                if (!_addSequenceFuncStep(&AirCon::sq_requestBigStatus))
                {
                    _debugMsg(F("getStatusBig: getBigInfo request sequence step fail."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    return false;
                }
                /*************************************** getBigInfo control ***********************************************/
                if (!_addSequenceFuncStep(&AirCon::sq_controlBigStatus))
                {
                    _debugMsg(F("getStatusBig: getBigInfo control sequence step fail."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    return false;
                }
                /**************************************************************************************/

                _debugMsg(F("getStatusBig: loaded to sequence"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);
                return true;
            }

            // запрос большого и малого пакетов статуса последовательно
            bool getStatusBigAndSmall()
            {
                // нет смысла в последовательности, если нет коннекта с кондиционером
                if (!get_has_connection())
                {
                    _debugMsg(F("getStatusBigAndSmall: no pings from HVAC. It seems like no AC connected."), ESPHOME_LOG_LEVEL_ERROR, __LINE__);
                    return false;
                }

                if (!getStatusSmall())
                {
                    _debugMsg(F("getStatusBigAndSmall: error with small status sequence."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    return false;
                }

                if (!getStatusBig())
                {
                    _debugMsg(F("getStatusBigAndSmall: error with big status sequence."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    return false;
                }

                _debugMsg(F("getStatusBigAndSmall: loaded to sequence"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);
                return true;
            }

            /** стартовая последовательность пакетов
             *
             * нужна, чтобы не ждать долго обновления статуса кондиционера
             * запускаем сразу, как только удалось подключиться к кондиционеру и прошел первый пинг-пакет
             * возвращаемое значение будет присвоено флагу выполнения последовательности
             * то есть при возврате false последовательность считается не запущенной и будет вызоваться до тех пор, пока не вернет true
             **/
            bool startupSequence()
            {
                // нет смысла в последовательности, если нет коннекта с кондиционером
                if (!get_has_connection())
                {
                    _debugMsg(F("startupSequence: no pings from HVAC. It seems like no AC connected."), ESPHOME_LOG_LEVEL_ERROR, __LINE__);
                    return false;
                }

                // по сути на старте надо получить от кондиционера два статуса
                if (!getStatusBigAndSmall())
                {
                    _debugMsg(F("startupSequence: error with big&small status sequence."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    return false;
                };

                _debugMsg(F("startupSequence: loaded to sequence"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);
                return true;
            }

            /** загружает на выполнение команду
             *
             * стандартная последовательность - это запрос маленького статусного пакета, выполнение команды и повторный запрос
             * такого же статуса для проверки, что всё включилось, ну и для обновления интерфейсов всяких связанных компонентов
             **/
            bool commandSequence(ac_command_t *cmd)
            {
                // нет смысла в последовательности, если нет коннекта с кондиционером
                if (!get_has_connection())
                {
                    _debugMsg(F("commandSequence: no pings from HVAC. It seems like no AC connected."), ESPHOME_LOG_LEVEL_ERROR, __LINE__);
                    return false;
                }

                // добавление начального запроса маленького статусного пакета в последовательность команд
                if (!getStatusSmall())
                {
                    _debugMsg(F("commandSequence: error with first small status sequence."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    return false;
                }

                // есть ли место на запрос в последовательности команд?
                if (_getFreeSequenceSpace() < 2)
                {
                    _debugMsg(F("commandSequence: not enough space in command sequence. Sequence steps doesn't loaded."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    return false;
                }

                /*************************************** set params request ***********************************************/
                if (!_addSequenceFuncStep(&AirCon::sq_requestDoCommand, cmd))
                {
                    _debugMsg(F("commandSequence: request sequence step fail."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    return false;
                }
                /*************************************** set params control ***********************************************/
                if (!_addSequenceFuncStep(&AirCon::sq_controlDoCommand))
                {
                    _debugMsg(F("commandSequence: control sequence step fail."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    return false;
                }
                /**************************************************************************************/

                // добавление финального запроса маленького статусного пакета в последовательность команд
                if (!getStatusSmall())
                {
                    _debugMsg(F("commandSequence: error with last small status sequence."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    return false;
                }

                _debugMsg(F("commandSequence: loaded to sequence"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);
                return true;
            }

            // загружает на выполнение последовательность команд на включение/выключение
            bool powerSequence(ac_power pwr = AC_POWER_ON)
            {
                // нет смысла в последовательности, если нет коннекта с кондиционером
                if (!get_has_connection())
                {
                    _debugMsg(F("powerSequence: no pings from HVAC. It seems like no AC connected."), ESPHOME_LOG_LEVEL_ERROR, __LINE__);
                    return false;
                }
                if (pwr == AC_POWER_UNTOUCHED)
                    return false; // выходим, чтобы не тратить время

                // формируем команду
                ac_command_t cmd;
                _clearCommand(&cmd); // не забываем очищать, а то будет мусор
                cmd.power = pwr;
                // добавляем команду в последовательность
                if (!commandSequence(&cmd))
                    return false;

                _debugMsg(F("powerSequence: loaded (power = %02X)"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, pwr);
                return true;
            }

            // выключает экран
            bool displayOffSequence()
            {
                ac_display dsp = AC_DISPLAY_OFF;
                if (this->get_display_inverted())
                    dsp = AC_DISPLAY_ON;
                return _displaySequence(dsp);
            }

            // включает экран
            bool displayOnSequence()
            {
                ac_display dsp = AC_DISPLAY_ON;
                if (this->get_display_inverted())
                    dsp = AC_DISPLAY_OFF;
                return _displaySequence(dsp);
            }

            // отправляет сплиту заданный набор байт
            // Перед отправкой:
            //      устанавливает первый байт в 0xBB
            //      проверяет, чтобы длина тела пакета в заголовке не превышала длину буфера
            //      рассчитывает и записывает в конец пакета CRC
            bool sendTestPacket(const std::vector<uint8_t> &data)
            {
                if (data.size() == 0)
                {
                    _debugMsg(F("sendTestPacket: no data to send."), ESPHOME_LOG_LEVEL_ERROR, __LINE__);
                    return false;
                }
                // if (data.size() > AC_BUFFER_SIZE) return false;

                // нет смысла в отправке, если нет коннекта с кондиционером
                if (!get_has_connection())
                {
                    _debugMsg(F("sendTestPacket: no pings from HVAC. It seems like no AC connected."), ESPHOME_LOG_LEVEL_ERROR, __LINE__);
                    return false;
                }

                // очищаем пакет
                _clearPacket(&_outTestPacket);

                // копируем данные в пакет
                uint8_t i = 0;
                for (uint8_t n : data)
                {
                    // всё, что не влезет в буфер - игнорируем
                    if (i >= AC_BUFFER_SIZE)
                    {
                        _debugMsg(F("sendTestPacket: buffer size =  %02d, data length = %02d. Extra data was omitted."), ESPHOME_LOG_LEVEL_ERROR, __LINE__, AC_BUFFER_SIZE, data.size());
                        break;
                    }
                    // что влезает - копируем в буфер
                    _outTestPacket.data[i] = n;
                    i++;
                }

                // на всякий случай указываем правильные некоторые байты:
                //    - установим стартовый байт
                _outTestPacket.header->start_byte = AC_PACKET_START_BYTE;
                //    - установим длину тела, если она больше возможной для нашего буфера
                if (_outTestPacket.header->body_length > (AC_BUFFER_SIZE - AC_HEADER_SIZE - 2))
                    _outTestPacket.header->body_length = AC_BUFFER_SIZE - AC_HEADER_SIZE - 2;

                _outTestPacket.msec = millis();
                _outTestPacket.body = &(_outTestPacket.data[AC_HEADER_SIZE]);
                _outTestPacket.bytesLoaded = AC_HEADER_SIZE + _outTestPacket.header->body_length + 2;

                // рассчитываем и записываем в пакет CRC
                _outTestPacket.crc = (packet_crc_t *)&(_outTestPacket.data[AC_HEADER_SIZE + _outTestPacket.header->body_length]);
                _setCRC16(&_outTestPacket);

                _debugMsg(F("sendTestPacket: test packet loaded:"), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                _debugPrintPacket(&_outTestPacket, ESPHOME_LOG_LEVEL_WARN, __LINE__);

                // ниже блок добавления отправки пакета в последовательность команд
                //*****************************************************************
                // есть ли место на запрос в последовательности команд?
                if (_getFreeSequenceSpace() < 1)
                {
                    _debugMsg(F("sendTestPacket: not enough space in command sequence. Sequence steps doesn't loaded."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    return false;
                }

                /*************************************** sendTestPacket request ***********************************************/
                if (!_addSequenceFuncStep(&AirCon::sq_requestTestPacket))
                {
                    _debugMsg(F("sendTestPacket: sendTestPacket request sequence step fail."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    return false;
                }
                /**************************************************************************************/

                _debugMsg(F("sendTestPacket: loaded to sequence"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__);

                return true;
            }

            // устанавливает ограничение мощности сплита на нужный уровень
            bool powerLimitationSetSequence(uint8_t power_limit, bool set_on=false)
            {
                // нет смысла в последовательности, если нет коннекта с кондиционером
                if (!get_has_connection())
                {
                    _debugMsg(F("powerLimitationSetSequence: no pings from HVAC. It seems like no AC connected."), ESPHOME_LOG_LEVEL_ERROR, __LINE__);
                    return false;
                }

                if (!this->_is_inverter)
                { // если кондиционер не инверторный, то выходим
                    _debugMsg(F("powerLimitationSetSequence: unsupported for noninverter AC."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    return false;
                }

                if(power_limit != this->_power_limitation_value_normalise(power_limit))
                {
                    _debugMsg(F("powerLimitationSetSequence: incorrect power limit value."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    return false;
                }
                
                // формируем команду
                ac_command_t cmd;
                _clearCommand(&cmd); // не забываем очищать, а то будет мусор
                cmd.power_lim_value = power_limit;
                if (set_on)
                {
                    cmd.power_lim_state = AC_POWLIMSTAT_ON;
                }
                // добавляем команду в последовательность
                if (!commandSequence(&cmd))
                    return false;
                
                if (set_on)
                {
                   _debugMsg(F("powerLimitationSetSequence: loaded (state = %02X, power limit = %02X)"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, cmd.power_lim_state, power_limit);
                } else {
                   _debugMsg(F("powerLimitationSetSequence: loaded (power limit = %02X)"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, power_limit);
                }
                return true;
            }

            // включает/выключает ограничение мощности сплита
            bool powerLimitationOnOffSequence(bool enable_limit)
            {
                // нет смысла в последовательности, если нет коннекта с кондиционером
                if (!get_has_connection())
                {
                    _debugMsg(F("powerLimitationOnOffSequence: no pings from HVAC. It seems like no AC connected."), ESPHOME_LOG_LEVEL_ERROR, __LINE__);
                    return false;
                }

                if (!this->_is_inverter)
                {
                    _debugMsg(F("powerLimitationOnSequence: unsupported for noninverter AC."), ESPHOME_LOG_LEVEL_WARN, __LINE__);
                    return false; // если кондиционер не инверторный, то выходим
                }
                
                // формируем команду
                ac_command_t cmd;
                _clearCommand(&cmd); // не забываем очищать, а то будет мусор
                if(enable_limit){
                   cmd.power_lim_state = AC_POWLIMSTAT_ON; // включить ограничение мощности
                } else {
                   cmd.power_lim_state = AC_POWLIMSTAT_OFF; // отключить ограничение мощности
                }
                // добавляем команду в последовательность
                if (!commandSequence(&cmd))
                    return false;

                _debugMsg(F("powerLimitationOnOffSequence: loaded (state = %02X)"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, cmd.power_lim_state);
                return true;
            }

            // включает ограничение мощности сплита
            bool powerLimitationOnSequence()
            {
               return powerLimitationOnOffSequence(true);
            }

            // включает ограничение мощности сплита на нужный уровень
            bool powerLimitationOnSequence(uint8_t power_limit)
            {
               return powerLimitationSetSequence(power_limit, true);
            }
            
            // выключает ограничение мощности сплита
            bool powerLimitationOffSequence()
            {
               return powerLimitationOnOffSequence(false);
            }

            // конвертирует состояние жалюзи из кодов сплита в коды для фронтенда
            ac_vlouver_frontend AUXvlouverToVlouverFrontend(const ac_louver_V vLouver)
            {
                switch (vLouver)
                {
                case AC_LOUVERV_SWING_UPDOWN:
                    return AC_VLOUVER_FRONTEND_SWING;

                case AC_LOUVERV_OFF:
                    return AC_VLOUVER_FRONTEND_STOP;

                case AC_LOUVERV_SWING_TOP:
                    return AC_VLOUVER_FRONTEND_TOP;

                case AC_LOUVERV_SWING_MIDDLE_ABOVE:
                    return AC_VLOUVER_FRONTEND_MIDDLE_ABOVE;

                case AC_LOUVERV_SWING_MIDDLE:
                    return AC_VLOUVER_FRONTEND_MIDDLE;

                case AC_LOUVERV_SWING_MIDDLE_BELOW:
                    return AC_VLOUVER_FRONTEND_MIDDLE_BELOW;

                case AC_LOUVERV_SWING_BOTTOM:
                    return AC_VLOUVER_FRONTEND_BOTTOM;

                default:
                    _debugMsg(F("AUXvlouverToVlouverFrontend: unknown vertical louver state = %u"), ESPHOME_LOG_LEVEL_DEBUG, __LINE__, _current_ac_state.louver.louver_v);
                    return AC_VLOUVER_FRONTEND_STOP;
                }
            }

            // возвращает текущее положение шторок в кодах для фронтенда
            ac_vlouver_frontend getCurrentVlouverFrontendState()
            {
                return AUXvlouverToVlouverFrontend(_current_ac_state.louver.louver_v);
            }

            // конвертирует состояние жалюзи из кодов для фронтенда в коды сплита
            ac_louver_V vlouverFrontendToAUXvlouver(const ac_vlouver_frontend vLouver)
            {
                switch (vLouver)
                {
                case AC_VLOUVER_FRONTEND_SWING:
                    return AC_LOUVERV_SWING_UPDOWN;

                case AC_VLOUVER_FRONTEND_STOP:
                    return AC_LOUVERV_OFF;

                case AC_VLOUVER_FRONTEND_TOP:
                    return AC_LOUVERV_SWING_TOP;

                case AC_VLOUVER_FRONTEND_MIDDLE_ABOVE:
                    return AC_LOUVERV_SWING_MIDDLE_ABOVE;

                case AC_VLOUVER_FRONTEND_MIDDLE:
                    return AC_LOUVERV_SWING_MIDDLE;

                case AC_VLOUVER_FRONTEND_MIDDLE_BELOW:
                    return AC_LOUVERV_SWING_MIDDLE_BELOW;

                case AC_VLOUVER_FRONTEND_BOTTOM:
                    return AC_LOUVERV_SWING_BOTTOM;

                default:
                    _debugMsg(F("vlouverFrontendToAUXvlouver: unknown vertical louver state = %u"), ESPHOME_LOG_LEVEL_DEBUG, __LINE__, _current_ac_state.louver.louver_v);
                    return AC_LOUVERV_OFF;
                }
            }

            // устанавливает жалюзи в нужное положение по коду сплита
            bool setVLouverSequence(const ac_louver_V vLouver)
            {
                // нет смысла в последовательности, если нет коннекта с кондиционером
                if (!get_has_connection())
                {
                    _debugMsg(F("setVLouverSequence: no pings from HVAC. It seems like no AC connected."), ESPHOME_LOG_LEVEL_ERROR, __LINE__);
                    return false;
                }
                if (vLouver == AC_LOUVERV_UNTOUCHED)
                    return false; // выходим, чтобы не тратить время

                if ((vLouver > AC_LOUVERV_OFF) || (vLouver == 0x06))
                    return false; // нет таких команд

                // формируем команду
                ac_command_t cmd;
                _clearCommand(&cmd); // не забываем очищать, а то будет мусор
                cmd.louver.louver_v = vLouver;
                // добавляем команду в последовательность
                if (!commandSequence(&cmd))
                    return false;

                _debugMsg(F("setVLouverSequence: loaded (vLouver = %02X)"), ESPHOME_LOG_LEVEL_VERBOSE, __LINE__, vLouver);
                return true;
            }

            // устанавливает жалюзи в нужное положение по коду для фронтенда
            bool setVLouverFrontendSequence(const ac_vlouver_frontend vLouver)
            {
                return setVLouverSequence(vlouverFrontendToAUXvlouver(vLouver));
            }

            // установка жалюзи в определенные положения
            bool setVLouverSwingSequence() { return setVLouverSequence(AC_LOUVERV_SWING_UPDOWN); }
            bool setVLouverStopSequence() { return setVLouverSequence(AC_LOUVERV_OFF); }
            bool setVLouverTopSequence() { return setVLouverSequence(AC_LOUVERV_SWING_TOP); }
            bool setVLouverMiddleAboveSequence() { return setVLouverSequence(AC_LOUVERV_SWING_MIDDLE_ABOVE); }
            bool setVLouverMiddleSequence() { return setVLouverSequence(AC_LOUVERV_SWING_MIDDLE); }
            bool setVLouverMiddleBelowSequence() { return setVLouverSequence(AC_LOUVERV_SWING_MIDDLE_BELOW); }
            bool setVLouverBottomSequence() { return setVLouverSequence(AC_LOUVERV_SWING_BOTTOM); }

            void set_period(uint32_t ms) { this->_update_period = ms; }
            uint32_t get_period() { return this->_update_period; }

            void set_show_action(bool show_action) { this->_show_action = show_action; }
            bool get_show_action() { return this->_show_action; }

            void set_display_inverted(bool display_inverted) { this->_display_inverted = display_inverted; }
            bool get_display_inverted() { return this->_display_inverted; }

            void set_packet_timeout(uint32_t ms)
            {
                if (ms < Constants::AC_PACKET_TIMEOUT_MIN)
                    ms = Constants::AC_PACKET_TIMEOUT_MIN;
                if (ms > Constants::AC_PACKET_TIMEOUT_MAX)
                    ms = Constants::AC_PACKET_TIMEOUT_MIN;
                this->_packet_timeout = ms;
            }
            uint32_t get_packet_timeout() { return this->_packet_timeout; }

            void set_optimistic(bool optimistic) { this->_optimistic = optimistic; }
            bool get_optimistic() { return this->_optimistic; }

            // возможно функции get и не нужны, но вроде как должны быть
            void set_supported_modes(const std::set<ClimateMode> &modes) { this->_supported_modes = modes; }
            std::set<ClimateMode> get_supported_modes() { return this->_supported_modes; }

            void set_supported_swing_modes(const std::set<ClimateSwingMode> &modes) { this->_supported_swing_modes = modes; }
            std::set<ClimateSwingMode> get_supported_swing_modes() { return this->_supported_swing_modes; }

            void set_supported_presets(const std::set<ClimatePreset> &presets) { this->_supported_presets = presets; }
            const std::set<climate::ClimatePreset> &get_supported_presets() { return this->_supported_presets; }

            void set_custom_presets(const std::set<std::string> &presets) { this->_supported_custom_presets = presets; }
            const std::set<std::string> &get_supported_custom_presets() { return this->_supported_custom_presets; }

            void set_custom_fan_modes(const std::set<std::string> &modes) { this->_supported_custom_fan_modes = modes; }
            const std::set<std::string> &get_supported_custom_fan_modes() { return this->_supported_custom_fan_modes; }

#if defined(PRESETS_SAVING)
            void set_store_settings(bool store_settings) { this->_store_settings = store_settings; }
            bool get_store_settings() { return this->_store_settings; }
            uint8_t load_presets_result = 0xFF;
#endif

            void setup() override
            {
#if defined(PRESETS_SAVING)
                load_presets_result = storage.load(global_presets); // читаем все пресеты из флеша
                _debugMsg(F("Preset base read from NVRAM, result %02d."), ESPHOME_LOG_LEVEL_WARN, __LINE__, load_presets_result);
#endif

                // заполнение шаблона параметров отображения виджета
                // GK: всё же похоже правильнее это делать тут, а не в initAC()
                // initAC() в формируемом питоном коде вызывается до вызова aux_ac.set_supported_***() с установленными пользователем в конфиге параметрами
                _traits.set_supports_current_temperature(true);
                _traits.set_supports_two_point_target_temperature(false); // if the climate device's target temperature should be split in target_temperature_low and target_temperature_high instead of just the single target_temperature

                _traits.set_supported_modes(this->_supported_modes);
                _traits.set_supported_swing_modes(this->_supported_swing_modes);
                _traits.set_supported_presets(this->_supported_presets);
                _traits.set_supported_custom_presets(this->_supported_custom_presets);
                _traits.set_supported_custom_fan_modes(this->_supported_custom_fan_modes);

                // tells the frontend what range of temperatures the climate device should display (gauge min/max values)
                // TODO: GK: а вот здесь похоже неправильно. Похоже, так мы не сможем выставить в конфиге свой диапазон температур - всегда будет от AC_MIN_TEMPERATURE до AC_MAX_TEMPERATURE
                _traits.set_visual_min_temperature(Constants::AC_MIN_TEMPERATURE);
                _traits.set_visual_max_temperature(Constants::AC_MAX_TEMPERATURE);
                // the step with which to increase/decrease target temperature. This also affects with how many decimal places the temperature is shown.
                _traits.set_visual_temperature_step(Constants::AC_TEMPERATURE_STEP);

                /* + MINIMAL SET */
                _traits.add_supported_mode(ClimateMode::CLIMATE_MODE_OFF);
                _traits.add_supported_mode(ClimateMode::CLIMATE_MODE_FAN_ONLY);
                _traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_AUTO);
                _traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_LOW);
                _traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_MEDIUM);
                _traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_HIGH);
                _traits.add_supported_swing_mode(ClimateSwingMode::CLIMATE_SWING_OFF);
                //_traits.add_supported_swing_mode(ClimateSwingMode::CLIMATE_SWING_VERTICAL);
                //_traits.add_supported_swing_mode(ClimateSwingMode::CLIMATE_SWING_BOTH);
                _traits.add_supported_preset(ClimatePreset::CLIMATE_PRESET_NONE);
                //_traits.add_supported_preset(ClimatePreset::CLIMATE_PRESET_SLEEP);

                // if the climate device supports reporting the active current action of the device with the action property.
                _traits.set_supports_action(this->_show_action);
            };

            void loop() override
            {
                if (!get_hw_initialized())
                    return;

#if defined(PRESETS_SAVING)
                // контролируем сохранение пресета
                if (_new_command_set)
                { // нужно сохранить пресет
                    _new_command_set = false;
                    save_preset((ac_command_t *)&_current_ac_state); // переносим текущие данные в массив пресетов
                }
#endif

                /// отрабатываем состояния конечного автомата
                switch (_ac_state)
                {
                case ACSM_RECEIVING_PACKET:
                    // находимся в процессе получения пакета, никакие отправки в этом состоянии невозможны
                    _doReceivingPacketState();
                    break;

                case ACSM_PARSING_PACKET:
                    // разбираем полученный пакет
                    _doParsingPacket();
                    break;

                case ACSM_SENDING_PACKET:
                    // отправляем пакет сплиту
                    _doSendingPacketState();
                    break;

                case ACSM_IDLE: // ничего не делаем, ждем, на что бы среагировать
                default:        // если состояние какое-то посторонее, то считаем, что IDLE
                    _doIdleState();
                    break;
                }

                // раз в заданное количество миллисекунд запрашиваем обновление статуса кондиционера
                if ((millis() - _dataMillis) > _update_period)
                {
                    _dataMillis = millis();

                    // обычный wifi-модуль запрашивает маленький пакет статуса
                    // но нам никто не мешает запрашивать и большой и маленький, чтобы чаще обновлять комнатную температуру
                    // делаем этот запрос только в случае, если есть коннект с кондиционером
                    if (get_has_connection())
                        getStatusBigAndSmall();
                }
            };
        };

    } // namespace aux_ac
} // namespace esphome