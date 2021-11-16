# Кастомный компонент для ESPHome для управления кондиционером по wifi <!--[![GitHub release](https://img.shields.io/github/v/release/GrKoR/esphome_aux_ac_component)](https://github.com/GrKoR/esphome_aux_ac_component/releases/) [![Телеграм](https://img.shields.io/badge/Telegram-2CA5E0?style=flat&logo=telegram&logoColor=white)](https://t.me/aux_ac) -->

Readme in english [is here](README-EN.md#esphome-aux-air-conditioner-custom-component-aux_ac). 

Управляет кондиционерами на базе AUX по wifi.<br />
По тексту ниже для компонента используется сокращение `aux_ac`.

Обсудить проект можно [в чате Телеграм](https://t.me/aux_ac).<br /> 
Отзывы о багах и ошибках, а так же запросы на дополнительный функционал оставляйте [в соответствующем разделе](https://github.com/GrKoR/esphome_aux_ac_component/issues).
Будет просто отлично, если к своему сообщению вы добавите лог и подробное описание. Для сбора логов есть [специальный скрипт на Python](https://github.com/GrKoR/ac_python_logger). С его помощью вы сможете сохранить в csv-файл все пакеты, которыми обменивается wifi-модуль и сплит-система. Если такой лог дополнить описанием, в какое время и что именно вы пытались включить, то это сильно ускорит исправление багов.


## ДИСКЛЭЙМЕР (ОТМАЗКИ) ##
1. Все материалы этого проекта (программы, прошивки, схемы, 3D модели и т.п.) предоставляются "КАК ЕСТЬ". Всё, что вы делаете с вашим оборудованием, вы делаете на свой страх и риск. Автор не несет ответственности за результат и ничего не гарантирует. Если вы с абсолютной четкостью не понимаете, что именно вы делаете и для чего, лучше просто купите wifi-модуль у производителя вашего кондиционера. 
2. Я ~~не настоящий сварщик~~ не программер. Поэтому код наверняка не оптимален и плохо оформлен (зато комментариев по коду я разместил от души), местами может быть написан небезопасно. И хоть я и старался протестировать всё, но уверен, что какие-то моменты упустил. Так что отнеситесь к коду с подозрением, ожидайте от него подвоха и если что-то увидели - [пишите в багрепорт](https://github.com/GrKoR/esphome_aux_ac_component/issues).


## Поддерживаемые кондиционеры ## 
AUX - это один из нескольких OEM-производителей кондиционеров. AUX производят кондиционеры как под собственным брендом, так и для внешних заказчиков. Поэтому есть шанс, что произведенный на их фабрике кондиционер неизвестного бренда с `aux_ac` так же заработает.

В интернете есть такой перечень производившихся на фабриках AUX брендов: AUX, Abion, AC ELECTRIC, Almacom, Ballu , Centek, Climer, DAX, Energolux, ERISSON, Green Energy, Hyundai, IGC, Kentatsu (некоторые серии), Klimaire, KOMANCHI, LANZKRAFT, LEBERG, LGen, Monroe, Neoclima, NEOLINE, One Air, Pioneer (до 2016 года), Roda, Rovex, Royal Clima, SAKATA, Samurai, SATURN, Scarlett, SmartWay, Soling, Subtropic, SUBTROPIC, Supra, Timberk, Vertex, Zanussi. В его полноте и достоверности есть сомнения, но ничего лучше найти не удалось.


### Список совместимых (протестированных) кондиционеров ###
[Список протестированных кондиционеров](docs/AC_TESTED.md) включает те модели, на которых `aux_ac` был запущен автором компонента или пользователями. Этот список постоянно пополняется, преимущественно по обратной связи от пользователей [в чате Телеграм](https://t.me/aux_ac).<br />
 
### Если кондиционер в списке отсутствует ###
Если производитель вашего кондиционера есть в списке выше, то стоит изучить вопрос. Возможно, вам тоже подойдет `aux_ac` для управления по wifi.<br />
Если в инструкции пользователя вашего кондиционера что-то написано про возможность управления по wifi (особенно с помощью мобильного приложения ACFreedom), то есть весьма существенные шансы, что `aux_ac` сможет управлять и вашим кондиционером. Но будьте осмотрительны: ваш кондиционер никем не тестировался и важно четко понимать, что вы делаете. Иначе можете поломать кондиционер.<br />
Если вы не уверены в своих силах, лучше дождитесь, пока другие более опытные пользователи протестируют вашу модель кондиционера (правда, это может не случиться никогда). Или приходите с вопросами [в телеграм-чат](https://t.me/aux_ac). Возможно, там вам помогут.

Если вы протестировали ваш кондиционер и он работает, напишите мне, пожалуйста. Я внесу вашу модель в список протестированных. Возможно, это упростит кому-то жизнь =)<br />
Лучший способ сообщить о протестированном кондиционере - написать [в телеграм](https://t.me/aux_ac) или [в разделе багрепортов и заказа фич](https://github.com/GrKoR/esphome_aux_ac_component/issues).

## Как использовать компонент ##
Для работы с кондиционером понадобится "железо" и прошивка. Описание электроники вынесено [в отдельный файл](docs/HARDWARE.md).

### Прошивка: интеграция aux_ac в вашу конфигурацию ESPHome ###
Для использования требуется [ESPHome](https://esphome.io) версией не ниже 1.18.0. Именно в этой версии появились `external_components`. Но лучше использовать версию 1.20.4 или старше, так как до этой версии массированно исправлялись ошибки в механизме подключения внешних компонентов.<br />

## Установка
1. Подключите компонент.
За подробностями можно заглянуть в [официальную документацию ESPHome](https://esphome.io/components/external_components.html?highlight=external).
```yaml
external_components:
  - source:
      type: git
      url: https://github.com/GrKoR/esphome_aux_ac_component
```
2. Настройте UART для коммуникации с вашим кондиционером:
```yaml
uart:
  id: ac_uart_bus
  tx_pin: GPIO1
  rx_pin: GPIO3
  baud_rate: 4800
  data_bits: 8
  parity: EVEN
  stop_bits: 1
```
3. **ВАЖНО!** Нужно отключить логгер ESPHome, чтобы он не отправлял в кондиционер свои данные.
Отключение логгера от UART никак не затронет вывод в лог консоли или web-сервера.
```yaml
logger:
    baud_rate: 0
```
Если по каким-то причинам вам нужен вывод логгера в UART, можно переключить его на другой UART чипа. Например, у ESP8266 два аппаратных UART: UART0 и UART1. `Aux_ac` подходит только UART0, поскольку только он у esp8266 имеет и TX и RX. Логгеру достаточно только TX. Такой функционал в чипе esp8266 у UART1:
```yaml
logger:
    level: DEBUG
    hardware_uart: UART1
```

## Настройка компонента ##

```yaml
climate:
  - platform: aux_ac
    name: "AC Name"
    id: aux_id
    uart_id: ac_uart_bus
    period: 7s            # период опроса состояния сплита, по дефолту 7 сек
    show_action: true     # надо ли показывать текущий режим работы: при HEAT_COOL mode сплит может греть (HEAT), охлаждать (COOL) или бездействовать (IDLE)
    indoor_temperature:   # сенсор, показывающий температуру воздуха на внутреннем блоке кондиционера; имеет все те же параметры, как и любой сенсор ESPHome
      name: AC Indoor Temperature
      id: ac_indoor_temp
      internal: true    # сенсор установлен как внутренний по дефолту (не попадёт в Home Assistant)
    visual:
      min_temperature: 16
      max_temperature: 32
      temperature_step: 0.5
    supported_modes:
      - HEAT_COOL   # не AUTO, так как только нагревает и остужает. В доках на ESPHome говорится, что AUTO - это если у устройства есть календарь и какие-то установки по расписанию.
      - COOL
      - HEAT
      - DRY
      - FAN_ONLY
    custom_fan_modes:
      - MUTE
      - TURBO
    supported_presets:
      - SLEEP
    custom_presets:
      - CLEAN
      - FEEL
      - HEALTH
      - ANTIFUNGUS
    supported_swing_modes:
      - VERTICAL
      - HORIZONTAL
      - BOTH
```

## Простейший пример ##
Исходный код простейшего примера можно найти в файле [aux_ac_simple.yaml](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/examples/simple/aux_ac_simple.yaml).

Все настройки в нем тривиальны и подробно описаны [в официальной документации на ESPHome](https://esphome.io/index.html) и дополнены [в разделе об интеграции компонента](https://github.com/GrKoR/esphome_aux_ac_component#%D0%BF%D1%80%D0%BE%D1%88%D0%B8%D0%B2%D0%BA%D0%B0-%D0%B8%D0%BD%D1%82%D0%B5%D0%B3%D1%80%D0%B0%D1%86%D0%B8%D1%8F-aux_ac-%D0%B2-%D0%B2%D0%B0%D1%88%D1%83-%D0%BA%D0%BE%D0%BD%D1%84%D0%B8%D0%B3%D1%83%D1%80%D0%B0%D1%86%D0%B8%D1%8E-esphome) в ваш девайс.<br />
Просто скопируйте yaml-файл примера и `aux_ac_custom_component.h` в локальную папку у себя на компьютере, пропишите настройки вашей сети WiFi и откомпилируйте YAML с использованием ESPHome. 


## Продвинутый пример ##
Все исходники продвинутого примера лежат [в соответствующей папке](https://github.com/GrKoR/esphome_aux_ac_component/tree/master/examples/advanced).

В это примере мы конфигурируем два относительно одинаковых кондиционера на работу с `aux_ac`.<br />
Вводные: представим, что у нас есть два кондея, расположенных в кухне и в гостиной. Эти кондиционеры могут и не быть одного бренда. Главное, чтобы они были совместимы с `aux_ac`.<br />  
Поскольку мы ленивы, мы пропишем все общие настройки обоих кондиционеров в общем конфигурационном файле `ac_common.yaml`.<br />
А все параметры, специфичные для каждого конкретного устройства, вынесем в отдельные файлы. Это файлы `ac_kitchen.yaml` и `ac_livingroom.yaml`. В них мы установим значения для подстановок `devicename` и `upper_devicename`, чтобы у устройств в сети были корректные имена самого компонента и его сенсоров. И здесь же мы указываем уникальные для каждого устройства IP-адреса, спрятанные в `secrets.yaml`.<br />
Кстати да! **Не забудьте** присвоить корректные значения `wifi_ip_kitchen`, `wifi_ota_ip_kitchen`, `wifi_ip_livingroom` и `wifi_ota_ip_livingroom` в файле `secrets.yaml` наряду с остальной "секретной" информацией (например пароли, токены и т.п.). Файл `secrets.yaml` по понятным причинам на гитхаб не выложен.

Если попытаться компилировать файл `ac_common.yaml`, то ESPHome выдаст ошибку. Для корректной прошивки необходимо компилировать `ac_kitchen.yaml` или `ac_livingroom.yaml`.
 
## Дополнительная функциональность ##
Компонент `aux_ac` предоставляет три дополнительных сенсора: два значения температуры и один номер версии прошивки.

### Комнатная температура ###
Этот сенсор отдает значения комнатной температуры воздуха с внутреннего блока кондиционера. Если значение этого датчика вам нужно, пропишите подобную конфигурацию сенсора в вашем YAML-файле:
```yaml
sensor:
  - platform: custom
    lambda: |-
      extern AirCon acAirCon;
      if (!acAirCon.get_initialized()) acAirCon.initAC(id(ac_uart_bus));
      App.register_component(&acAirCon);
      return {acAirCon.sensor_ambient_temperature};
    sensors:
    - name: AC ambient temperature
      unit_of_measurement: "°C"
      accuracy_decimals: 1
```
 
### Уличная температура ###
К сожалению, пока этот сенсор показывает погоду на Марсе =) Значение, обрабатываемое `aux_ac` для нужд этого сенсора точно как-то связано с уличной температурой, но полностью расшифровка значения не известна. Есть предположение, что это температура испарителя во внешнем блоке, потому что при переключении кондиционера с обогрева на охлаждение или обратно эта температура стремительно меняется. А при выключенном кондиционере в течение суток меняется похожим на уличную температуру образом. Однако всё это при теплой погоде на улице. При отрицательной температуре показывает одно и то же значение. По крайней мере при температурах в диапазоне -25..-19 градусов Цельсия.<br />
В общем, для расшифровки надо собрать больше статистики и коллективно подумать в чатике.

Если несмотря на сказанное вам нужно это значение в ESPHome, пропишите следующий сенсор в конфигурации:
```yaml
sensor:
  - platform: custom
    lambda: |-
      extern AirCon acAirCon;
      if (!acAirCon.get_initialized()) acAirCon.initAC(id(ac_uart_bus));
      App.register_component(&acAirCon);
      return {acAirCon.sensor_outdoor_temperature};
    sensors:
    - name: AC outdoor temperature
      unit_of_measurement: "°C"
      accuracy_decimals: 1
```
 
### Обе температуры одновременно ###
Возможно прописать конфигурацию обоих сенсоров в одном определении:
```yaml
sensor:
  - platform: custom
    lambda: |-
      extern AirCon acAirCon;
      if (!acAirCon.get_initialized()) acAirCon.initAC(id(ac_uart_bus));
      App.register_component(&acAirCon);
      return {acAirCon.sensor_outdoor_temperature, acAirCon.sensor_ambient_temperature};
    sensors:
    - name: AC outdoor temperature
      unit_of_measurement: "°C"
      accuracy_decimals: 1
    - name: AC ambient temperature
      unit_of_measurement: "°C"
      accuracy_decimals: 1
```
 
### Версия прошивки ###
Компонент `aux_ac` предоставляет информацию о своей версии в виде текстового сенсора. Соответствующая конфигурация показана ниже:
```yaml
text_sensor:
- platform: custom
  lambda: |-
    auto aircon_firmware_version = new AirConFirmwareVersion();
    App.register_component(aircon_firmware_version);
    return {aircon_firmware_version};
  text_sensors:
    name: AC firmware version
    icon: "mdi:chip"
```
