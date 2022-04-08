# ESPHome AUX air conditioner custom component (aux_ac) #
For communication about this project [join telegram chat](https://t.me/aux_ac). 
 
For issues or feature requests, please go to [the issue section](https://github.com/GrKoR/esphome_aux_ac_component/issues). It will be perfect if you attach log to your issue. Log you can collect with [this python script](https://github.com/GrKoR/ac_python_logger). It helps you to save all data frames from the UART bus to a csv-file. This log combined with the detailed situation description will significantly speed up bug correction.
 
 
## DISCLAIMER ##
1. All data of this project (software, firmware, schemes, 3d-models etc.) are provided **'AS IS'**. Everything you do with your devices you are doing at your own risk. If you don't strongly understand what you are doing, just buy wifi-module from your air conditioner manufacturer.
2. I am not a programmer. So source code is certainly not optimal and badly decorated (but there are alot of comments in it; sorry, a significant part of it is in Russian). Also code may be written unsafe. I tried to test all parts of the code but I'm sure I missed a lot of things. So treat it with suspicion, expect a trick from it, and if you discover something wrong write an issue here.
3. Russian and English readmes are substantially identical in meaning. But in case of differences the [Russian](https://github.com/GrKoR/esphome_aux_ac_component#readme) version is more significant.
 
## Short description ##
This custom component allows you to control your air conditioner through wifi if it is made in the AUX factory.<br />
Component tested with ESPHome 1.15.3 and Rovex ALS1 air conditioner. It looks like many other air conditioners can be controlled by `aux_ac` but this possibility isn't tested. See list of testetd ACs below for more details.
 
 
## Supported air conditioners ##
AUX is one of the OEM air conditioner manufacturers. AUX produce ACs for many brands.
There is following list of AUX-based air conditioner in the internet: AUX, Abion, AC ELECTRIC, Almacom, Ballu , Centek, Climer, DAX, Energolux, ERISSON, Green Energy, Hyundai, IGC, Kentatsu (некоторые серии), Klimaire, KOMANCHI, LANZKRAFT, LEBERG, LGen, Monroe, Neoclima, NEOLINE, One Air, Pioneer (до 2016 года), Roda, Rovex, Royal Clima, SAKATA, Samurai, SATURN, Scarlett, SmartWay, Soling, Subtropic, SUBTROPIC, Supra, Timberk, Vertex, Zanussi. There are doubts about its completeness and reliability, but nothing better could be found.

### List of compatible ACs (tested) ###
[The list of tested ACs](docs/AC_TESTED.md) is placed in a separate file and includes tested by the author or by users ACs. This list is permanently updated mainly based on feedback from users in [Telegram chat](https://t.me/aux_ac).<br />


### If your AC is not in the list ###
If your AC is listed above you should take a closer look at `aux_ac`.<br />
If the User Manual of your HVAC describes connection to wifi with mobile app ACFreedom it seems you may go deeper with `aux_ac`. But try all soft and hardware for your own risk. You must clearly understand what you are doing.<br />
If you are unsure it is better to wait while other users will test your model of AC (but it may never). Or please [go to telegram-chat](https://t.me/aux_ac) with your questions. Maybe you will get help there.

If you have tested your air conditioner and `aux_ac` works with it please let me know about it. I'll add this info to the list of tested ACs above.
The best way to report about your test results is write a message in the [telegram](https://t.me/aux_ac) or [in the issue section](https://github.com/GrKoR/esphome_aux_ac_component/issues).


## How to use it ##
For correct component operation you need hardware and firmware. The hardware description is located [in separate file](docs/HARDWARE-EN.md).

### Firmware: Integration aux_ac to your configuration ###
You need [ESPHome](https://esphome.io) v.1.18.0 or above. `External_components` have appeared in this version. But it is better to use ESPHome v.1.20.4 or above cause there was alot of `external_components` errors corrected before this version.

## Installing ##
1. Declare external component. Read [the manual](https://esphome.io/components/external_components.html?highlight=external) for details.
```yaml
external_components:
  - source:
      type: git
      url: https://github.com/GrKoR/esphome_aux_ac_component
```
2. Configure UART to communicate with air conditioner:
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
3. **ATTENTION!** You need to disable the ESPHome logger so that it does not send its data to the air conditioner. Disabling the logger from the UART bus will not affect the logger output to the console or web server in any way.
```yaml
logger:
    baud_rate: 0
```
If for some reason you need the logger output to the UART, you can switch it to another UART. ESP8266 has two hardware UARTs: UART0 and UART1. Only UART0 suits for `aux_ac` cause only it has both TX and RX. UART1 has TX only and it can be used by logger for output:
```yaml
logger:
    level: DEBUG
    hardware_uart: UART1
```

## AUX_AC Configuration ##
Minimal configuration:
```yaml
climate:
  - platform: aux_ac
    name: "AC Name"
```

Full configuration:
```yaml
climate:
  - platform: aux_ac
    name: "AC Name"
    id: aux_id
    uart_id: ac_uart_bus
    period: 7s
    show_action: true
    indoor_temperature:
      name: AC Indoor Temperature
      id: ac_indoor_temp
      internal: true
    visual:
      min_temperature: 16
      max_temperature: 32
      temperature_step: 0.5
    supported_modes:
      - HEAT_COOL
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

## Configuration variables: ##
- **name** (**Required**, string): The name of the climate device. At least one of `id` or `name` is required!
- **id** (*Optional*, [ID](https://esphome.io/guides/configuration-types.html#config-id)): Manually specify the ID used for code generation. At least one of `id` or `name` is required!
- **uart_id** (*Optional*, [ID](https://esphome.io/guides/configuration-types.html#config-id)): Manually specify the ID of the [UART Bus](https://esphome.io/components/uart.html) if you want to use multiple UART buses.
- **period** (*Optional*, [time](https://esphome.io/guides/configuration-types.html#config-time)): Period between status requests to the AC. Defaults to ``7s``. `Aux_ac` will receive the new air conditioner status only after a regular request, even if you change the settings of AC using IR-remote.
- **show_action** (*Optional*, boolean): Whether to show current action of the device (experimental). For example in the HEAT-COOL mode AC hardware may be in one of the following actions:
  - HEATING: AC is heating the air in the room;
  - IDLE: AC is working in the FAN mode cause the target temperature is reached;
  - COOLING: AC is cooling the air.
  The same thing will be in HEAT or COOL modes, with the only difference of the list of actions (IDLE + HEATING or IDLE + COOLING).
- **indoor_temperature** (*Optional*): The information for the air temperature sensor
  - **name** (**Required**, string): The name for the temperature sensor.
  - **id** (*Optional*, [ID](https://esphome.io/guides/configuration-types.html#config-id)): Set the ID of this sensor for use in lambdas.
  - **internal** (*Optional*, boolean): Mark this component as internal. Internal components will not be exposed to the frontend (like Home Assistant). As opposed to default [Sensor](https://esphome.io/components/sensor/index.html#base-sensor-configuration) behaviour this variable is **always true** except in cases where the user has set it directly.
  - All other options from [Sensor](https://esphome.io/components/sensor/index.html#base-sensor-configuration).
- **supported_modes** (*Optional*, list): List of supported modes. Possible values are: ``HEAT_COOL``, ``COOL``, ``HEAT``, ``DRY``, ``FAN_ONLY``. Please note: some manufacturers call AUTO mode instead of HEAT_COOL. Defaults to ``FAN_ONLY``.
- **custom_fan_modes** (*Optional*, list): List of supported custom fan modes. Possible values are: ``MUTE``, ``TURBO``. No custom fan modes by default.
- **supported_presets** (*Optional*, list): List of supported presets. Possible values are: ``SLEEP``. No presets by default.
- **custom_presets** (*Optional*, list): List of supported custom presets. Possible values are: ``CLEAN``, ``FEEL``, ``HEALTH``, ``ANTIFUNGUS``. Please note: presets ``FEEL``, ``HEALTH`` and ``ANTIFUNGUS`` have not been implemented yet. No custom presets by default.
- **supported_swing_modes** (*Optional*, list): List of supported swing modes. Possible values are: ``VERTICAL``, ``HORIZONTAL``, ``BOTH``. No swing modes by default.
- All other options from [Climate](https://esphome.io/components/climate/index.html#base-climate-configuration).

## Actions: ##
### ``aux_ac.display_on`` ###
This action turns a HVAC temperature display on when executed.

```yaml
on_...:
  then:
    - aux_ac.display_on: aux_ac_id
```
- **aux_ac_id** (**Requared**, string): ID of `aux_ac` component.

### ``aux_ac.display_off`` ###
This action turns a HVAC temperature display off when executed.

```yaml
on_...:
  then:
    - aux_ac.display_off: aux_ac_id
```
- **aux_ac_id** (**Requared**, string): ID of `aux_ac` component.


## Simple example ##
The source code of this example is located in the [aux_ac_simple.yaml](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/examples/simple/aux_ac_simple.yaml) file.

All settings in it is trivial. Just copy file to your local folder, specify your wifi settings and compile YAML with ESPHome.


## Advanced example ##
All sources are located [in advanced example folder](https://github.com/GrKoR/esphome_aux_ac_component/tree/master/examples/advanced).

This time we'll configure two relative identical air conditioners with `aux_ac` custom component.<br />
Let's imagine we have ACs in a kitchen and in a living room. All ACs are the same brand and can be controlled by `aux_ac`.<br />  
Cause we are lazy we'll define all common configuration parts for two air conditioners in one `ac_common.yaml` file.<br />
All specific parts of configuration are located in the `ac_kitchen.yaml` and `ac_livingroom.yaml`. Here we set `devicename` and `upper_devicename` for correct sensors and component naming. And here we specify the correct IP-address of the device from `secrets.yaml`.<br />
**Don't forget** to specify `wifi_ip_kitchen`, `wifi_ota_ip_kitchen`, `wifi_ip_livingroom` and `wifi_ota_ip_livingroom` in the `secrets.yaml` along with the other sensitive information, such as passwords, tokens etc.

If you try to compile `ac_common.yaml` it will raise errors. You need to compile `ac_kitchen.yaml` or `ac_livingroom.yaml` instead.