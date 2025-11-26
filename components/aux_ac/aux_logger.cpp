#include "aux_logger.h"

namespace esphome
{
    namespace aux_ac
    {
        static const char *const TAG = "AirCon"; // TODO: verify if this tag is appropriate

        /** вывод отладочной информации в лог
         *
         * dbgLevel - уровень сообщения, определен в ESPHome. За счет его использования можно из ESPHome управлять полнотой сведений в логе.
         * msg - сообщение, выводимое в лог
         * line - строка, на которой произошел вызов (удобно при отладке)
         */
        void debugMsg(const /*String &*/ char *msg, uint8_t dbgLevel, unsigned int line, ...)
        {
            if (dbgLevel < ESPHOME_LOG_LEVEL_NONE)
                dbgLevel = ESPHOME_LOG_LEVEL_NONE;
            if (dbgLevel > ESPHOME_LOG_LEVEL_VERY_VERBOSE)
                dbgLevel = ESPHOME_LOG_LEVEL_VERY_VERBOSE;

            if (line == 0)
                line = __LINE__; // если строка не передана, берем текущую строку

            va_list vl;
            va_start(vl, line);
            esp_log_vprintf_(dbgLevel, TAG, line, msg, vl);
            va_end(vl);
        };

        /** выводим данные пакета в лог для отладки
         *
         * dbgLevel - уровень сообщения, определен в ESPHome. За счет его использования можно из ESPHome управлять полнотой сведений в логе.
         * packet - указатель на пакет для вывода;
         *          если указатель на crc равен nullptr или первый байт в буфере не AC_PACKET_START_BYTE, то считаем, что передан битый пакет
         *          или не пакет вовсе. Для такого выводим только массив байт.
         *          Для нормального пакета данные выводятся с форматированием.
         * line - строка, на которой произошел вызов (удобно при отладке)
         **/
        void debugPrintPacket(packet_t *packet, uint8_t dbgLevel, unsigned int line)
        {
            // определяем, полноценный ли пакет нам передан
            bool notAPacket = false;
            /*
            // указатель заголовка всегда установден на начало буфера
            notAPacket = notAPacket || (packet->crc == nullptr);
            notAPacket = notAPacket || (packet->data[0] != AC_PACKET_START_BYTE);

            String st = "";
            char textBuf[11];

            // заполняем время получения пакета
            memset(textBuf, 0, 11);
            sprintf(textBuf, "%010" PRIu32, packet->msec);
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
            */
        }

    } // namespace aux_ac
} // namespace esphome