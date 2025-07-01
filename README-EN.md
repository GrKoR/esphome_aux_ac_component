# ESPHome AUX air conditioner custom component (aux_ac) #
For communication about this project [please join this telegram chat](https://t.me/aux_ac). 
 
For issues or feature requests, please go to [the issue section](https://github.com/GrKoR/esphome_aux_ac_component/issues). It will be perfect if you attach log to your issue. Log you can collect with [this python script](https://github.com/GrKoR/ac_python_logger). It helps you to save all data frames from the UART bus to a csv-file. This log combined with the detailed situation description will significantly speed up bug correction.
There is also a [detailed instruction describing how to properly request a feature](docs/HOW_TO_FEATURE_REQUEST-EN.md).
 
## DISCLAIMER ##
1. All data of this project (software, firmware, schemes, 3d-models etc.) are provided **'AS IS'**. Everything you do with your devices, you are doing at your own risk. If you don't strongly understand what you are doing, just buy Wi-Fi module from your air conditioner manufacturer.
2. I am not a programmer. So source code is certainly not optimal and badly decorated (but there are a lot of comments in it; sorry, a significant part of it is in Russian). Also, code may be written unsafe. I tried to test all parts of the code, but I'm sure I missed a lot of things. So treat it with suspicion, expect a trick from it, and if you discover something wrong write an issue here.
3. Russian and English readme files are substantially identical in meaning. But in case of differences, the [Russian](https://github.com/GrKoR/esphome_aux_ac_component#readme) version is more significant.
 
## Short description ##
This custom component allows you to control your air conditioner through Wi-Fi if it is made in the AUX factory.<br />
Component tested with ESPHome 1.18.0 and Rovex ALS1 air conditioner. It looks like many other air conditioners can be controlled by `aux_ac`, but this possibility isn't tested. See list of tested ACs below for more details.
 
 
## Supported air conditioners ##
AUX is one of the OEM air conditioner manufacturers. AUX produce ACs for many brands.
There is the following list of AUX-based air conditioner on the internet: AUX, Abion, AC ELECTRIC, Almacom, Ballu, Centek, Climer, DAX, Energolux, ERISSON, Green Energy, Hyundai, IGC, Kentatsu (some series only), Klimaire, KOMANCHI, LANZKRAFT, LEBERG, LGen, Monroe, Neoclima, NEOLINE, One Air, Pioneer (until 2016), Roda, Rovex, Royal Clima, SAKATA, Samurai, SATURN, Scarlett, SmartWay, Soling, Subtropic, SUBTROPIC, Supra, Timberk, Vertex, Zanussi. There are doubts about its completeness and reliability, but nothing better could be found.

### List of compatible ACs (tested) ###
[The list of tested ACs](docs/AC_TESTED.md) is placed in a separate file and includes tested by the author or by users ACs. This list is permanently updated, mainly based on feedback from users in [Telegram chat](https://t.me/aux_ac).<br />

### If your AC is not in the list ###
1. If your AC is listed above, you should take a closer look at `aux_ac`.<br />
2. If something about AUX is written on the nameplate of the air conditioner in the manufacturer line.<br />
3. If the User Manual of your HVAC describes connection to Wi-Fi with mobile app ACFreedom it seems you may go deeper with `aux_ac`. But try all soft and hardware for your own risk. You must clearly understand what you are doing.<br />
4. If the manufacturer of your AC offers a CTTM-40X24-WIFI-AKS Wi-Fi module (left) or the one in the photo on the right for control. Moreover, the right module can be either with a USB connector or with a 5-pin connector.<br />
<img src="https://user-images.githubusercontent.com/57137862/172053621-60fe39d8-066e-44fa-91c5-725fa1f5c3bc.png" height="300">  <img src="https://user-images.githubusercontent.com/57137862/172053744-8ce4a13d-28cb-4688-a998-11ca3a7129df.png" height="300"> 

If you are unsure, it is better to wait while other users will test your model of AC (maybe never). Or please [go to telegram-chat](https://t.me/aux_ac) with your questions. Maybe you will get help there.<br />
If you have tested your air conditioner and `aux_ac` works with it, please let me know about it. I'll add this info to the list of tested ACs above.<br />
The best way to report about your test results is writing a message in the [telegram](https://t.me/aux_ac) or [in the issue section](https://github.com/GrKoR/esphome_aux_ac_component/issues).<br />

## How to use it ##
For correct component operation, you need hardware and firmware. The hardware description is located [in a separate file](docs/HARDWARE-EN.md).

### Firmware: Integration aux_ac to your configuration ###
You need [ESPHome](https://esphome.io) v.2025.2.0 or above. You can try esphome before 2025.2.0 but I can't guarantee error-free compilation of the examples.

## Installing ##
1. Declare external component. Read [the manual](https://esphome.io/components/external_components.html?highlight=external) for details.
```yaml
external_components:
  - source:
      type: git
      url: https://github.com/GrKoR/esphome_aux_ac_component
```
In case you need a specific version of the component, you can use the component declaration from the example below. The example uses version 0.2.14 of the component. You can find a list of available versions [on the GitHub tags page](https://github.com/GrKoR/esphome_aux_ac_component/tags).
```yaml
external_components:
  - source:
      type: git
      url: https://github.com/GrKoR/esphome_aux_ac_component
      ref: v.0.2.14
```
2. Configure UART to communicate with air conditioner:
```yaml
uart:
  id: ac_uart_bus
  # ATTENTION! For TX and RX use GPIO4 (D2) and GPIO5 (D1) for NodeMCU-like boards!
  # See docs for details: https://github.com/GrKoR/esphome_aux_ac_component/blob/master/docs/HARDWARE-EN.md
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
If for some reason you need the logger output to the UART, you can switch it to another UART. ESP8266 has two hardware UARTs: UART0 and UART1. Only UART0 suits for `aux_ac` cause only it has both TX and RX. UART1 has TX only, and it can be used by logger for output:
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
    display_inverted: false
    timeout: 150
    optimistic: true
    indoor_temperature:
      name: AC Indoor Temperature
      id: ac_indoor_temp
      accuracy_decimals: 1
      internal: false
    outdoor_temperature:
      name: AC Outdoor Temperature
      id: ac_outdoor_temp
      internal: false
    outbound_temperature:
      name: AC Coolant Outbound Temperature
      id: ac_outbound_temp
      internal: false
    inbound_temperature:
      name: AC Coolant Inbound Temperature
      id: ac_inbound_temp
      internal: false
    compressor_temperature:
      name: AC Compressor Temperature
      id: ac_strange_temp
      internal: false
    display_state:
      name: AC Display State
      id: ac_display_state
      internal: false
    defrost_state:
      name: AC Defrost State
      id: ac_defrost_state
      internal: false
    inverter_power:
      name: AC Inverter Power
      id: ac_inverter_power
      internal: false
    inverter_power_limit_value:
      name: AC Inverter Power Limit Value
      id: ac_inverter_power_limit_value
      internal: false
    inverter_power_limit_state:
      name: AC Inverter Power Limit State
      id: ac_inverter_power_limit_state
      internal: false
    preset_reporter:
      name: AC Preset Reporter
      id: ac_preset_reporter
      internal: false
    vlouver_state:
      name: AC Vertical Louvers State
      id: ac_vlouver_state
      internal: false
    visual:
      min_temperature: 16
      max_temperature: 32
      temperature_step: 1
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

- **period** (*Optional*, [time](https://esphome.io/guides/configuration-types.html#config-time), default ``7s``): Period between status requests to the AC. `Aux_ac` will receive the new air conditioner status only after a regular request, even if you change the settings of AC using IR-remote.

- **show_action** (*Optional*, boolean, default ``true``): Whether to show current action of the device (experimental). For example, in the HEAT_COOL mode, AC hardware may be in one of the following actions:
  - HEATING: AC is heating the air in the room;
  - IDLE: AC is working in the FAN mode, cause the target temperature is reached;
  - COOLING: AC is cooling the air.
  The same thing will be in HEAT or COOL modes, with the only difference of the list of actions (IDLE + HEATING or IDLE + COOLING).

  - **display_inverted** (*Optional*, boolean, default ``false``): It configures display driver logic level. As it turned out in the issue [#31](https://github.com/GrKoR/esphome_aux_ac_component/issues/31), different models of conditioners manage display different way. Rovex ACs powers off display by bit `1` in command packet and power it on by bit `0`. Many other conditioners do this vice versa.

- **timeout** (*Optional*, unsigned integer, default ``150``): Packet timeout for `aux_ac` data receiver.  
  In the most common use of `aux_ac`, it isn't necessary to change this value. This keyword is optional, so you may omit it.  
  The only situation when you can play with timeout is heavily loaded ESP. When you are using your ESP for many hard tasks, it is possible that `aux_ac` does not have enough time to receive AC responses. In this case, you can slightly raise the timeout value. But the best solution would be to remove some of the tasks from the ESP.  
  The timeout is limited to a range from `150` to `600` milliseconds. Other values are possible only with source code modification. But I don't recommend that.

- **optimistic** (*Optional*, boolean, default ``true``): Whether entity states should be updated immediately after receiving a command from Home Assistant/ESPHome.

- **indoor_temperature** (*Optional*): Parameters of the room air temperature sensor.
  - **name** (**Required**, string): The name for the temperature sensor.
  - **id** (*Optional*, [ID](https://esphome.io/guides/configuration-types.html#config-id)): Set the ID of this sensor for use in lambdas.
  - **internal** (*Optional*, boolean): Mark this component as internal. Internal components will not be exposed to the frontend (like Home Assistant). As opposed to default [Sensor](https://esphome.io/components/sensor/index.html#base-sensor-configuration) behaviour, this variable is **always true** except in cases where the user has set it directly.
  - All other options from [Sensor](https://esphome.io/components/sensor/index.html#base-sensor-configuration).

- **outdoor_temperature** (*Optional*): Parameters of the outdoor temperature sensor. They are the same as the **indoor_temperature** (see description above).  
  > **Attention!** When the air conditioner is turned off, the outdoor temperature is updated rarely (every 6-7 hours). This isn't a bug of the component, but a feature of the air conditioner hardware. The only way to get changes more often is to create a template sensor, the temperature of which can be changed manually. When the air conditioner is working, the value of this sensor can be copied from the **outdoor_temperature**. When the air conditioner is turned off, the temperature value should be recalculated according to the dynamics of the **outbound_temperature** sensor (it changes frequently and shows values close to the air temperature when the air conditioner is turned off). You can't copy the value of **outbound_temperature** without changes to the template sensor in AC off mode, because these temperatures are not identical.

- **inbound_temperature** (*Optional*): Parameters of the coolant inbound temperature sensor. They are the same as the **indoor_temperature** (see description above).

- **outbound_temperature** (*Optional*):  Parameters of the coolant outbound temperature sensor. They are the same as the **indoor_temperature** (see description above).

- **compressor_temperature** (*Optional*):  Parameters of the compressor temperature sensor. They are the same as the **indoor_temperature** (see description above).

- **display_state** (*Optional*): The information for the HVAC display state sensor (is display ON or OFF)
  - **name** (**Required**, string): The name for the display state sensor.
  - **id** (*Optional*, [ID](https://esphome.io/guides/configuration-types.html#config-id)): Set the ID of this sensor for use in lambdas.
  - **internal** (*Optional*, boolean): Mark this component as internal. Internal components will not be exposed to the frontend (like Home Assistant). As opposed to default [Binary Sensor](https://esphome.io/components/binary_sensor/index.html#base-binary-sensor-configuration) behavior, this variable is **always true** except in cases where the user has set it directly.
  - All other options from [Binary Sensor](https://esphome.io/components/binary_sensor/index.html#base-binary-sensor-configuration).

- **defrost_state** (*Optional*): The information for the HVAC defrost function state sensor (is it ON or OFF). All settings are the same as for the **display_state** (see description above).

- **inverter_power** (*Optional*): The information for the inverter power sensor. All settings are the same as for the **indoor_temperature** (see description above).
  > **ATTENTION!** The parameter name was changed in v.0.2.9 due to incorrect spelling.

- **inverter_power_limit_state** (*Optional*): Configuration of the power limit state sensor. It displays the state of the power limitation function for the inverter HVAC (is it ON or OFF). All settings are the same as for the **display_state** (see description above).

- **inverter_power_limit_value** (*Optional*): Configuration of the power limit value sensor. All settings are the same as for the **indoor_temperature** (see description above).  
It reports the current value of the power limitation function for the inverter HVAC. This sensor represents the value only after the HVAC confirms the power limitation. The value is always in the range from 30% to 100%. This is the hardware limitation.

- **preset_reporter** (*Optional*): Parameters of text sensor with current preset. All settings are the same as for the **display_state** (see description above).  
  ESPHome Climate devices are not reporting their active presets (from **supported_presets** and **custom_presets** lists) to MQTT. This behavior has been noticed at least in version 1.20.0. In case you are using MQTT and want to receive information about active preset, you should declare this sensor in your yaml.

- **vlouver_state** (*Optional*): Parameters of vertical louvers state sensor. All settings are the same as for the **display_state** (see description above). The state of the vertical louvers is encoded by the integer value (see [aux_ac.vlouver_set action](#aux_ac_._vlouver_set) below).

- **supported_modes** (*Optional*, list): List of supported modes. Possible values are: ``HEAT_COOL``, ``COOL``, ``HEAT``, ``DRY``, ``FAN_ONLY``. Please note: some manufacturers call AUTO mode instead of HEAT_COOL. Defaults to ``FAN_ONLY``.

- **custom_fan_modes** (*Optional*, list): List of supported custom fan modes. Possible values are: ``MUTE``, ``TURBO``. No custom fan modes by default.

- **supported_presets** (*Optional*, list): List of supported presets. Possible values are: ``SLEEP``. No presets by default.

- **custom_presets** (*Optional*, list): List of supported custom presets. Possible values are: ``CLEAN``, ``HEALTH``, ``ANTIFUNGUS``. No custom presets by default.

- **supported_swing_modes** (*Optional*, list): List of supported swing modes. Possible values are: ``VERTICAL``, ``HORIZONTAL``, ``BOTH``. No swing modes by default.

- All other options from [Climate](https://esphome.io/components/climate/index.html#base-climate-configuration).


## Actions: ##
### ``aux_ac.display_on`` ###
This action turns a HVAC temperature display on when executed.

```yaml
on_...:
  then:
    - aux_ac.display_on: aux_id
```
- **aux_id** (**Requared**, string): ID of `aux_ac` component.

### ``aux_ac.display_off`` ###
This action turns a HVAC temperature display off when executed.

```yaml
on_...:
  then:
    - aux_ac.display_off: aux_id
```
- **aux_id** (**Requared**, string): ID of `aux_ac` component.

### ``aux_ac.vlouver_set`` ###
This action moves HVAC vertical louvers to the specified position.

The position is encoded by the following values:  
- `0`: the vertical louvers are in `SWING` mode (they are moving up and down);
- `1`: the louvers are stopped in a user position;
- `2`: the louvers are in the topmost position;
- `3`: the louvers are in one step above middle position;
- `4`: the louvers are in the middle position;
- `5`: the louvers are in one step below middle position;
- `6`: the louvers are in the lowest position.

```yaml
on_...:
  then:
    - aux_ac.vlouver_set:
        id: aux_id
        position: 3 # moves the louvers to the middle position
```
- **aux_id** (**Required**, string): ID of `aux_ac` component.
- **position** (**Required**, integer): position of the vertical louvers.

### ``aux_ac.vlouver_stop`` ###
This action stops vertical swing of louvers.

```yaml
on_...:
  then:
    - aux_ac.vlouver_stop: aux_id
```
- **aux_id** (**Required**, string): ID of `aux_ac` component.

### ``aux_ac.vlouver_swing`` ###
This action starts the vertical swing of louvers.

```yaml
on_...:
  then:
    - aux_ac.vlouver_swing: aux_id
```
- **aux_id** (**Required**, string): ID of `aux_ac` component.

### ``aux_ac.vlouver_top`` ###
This action moves HVAC louvers to the topmost position.

```yaml
on_...:
  then:
    - aux_ac.vlouver_top: aux_id
```
- **aux_id** (**Required**, string): ID of `aux_ac` component.

### ``aux_ac.vlouver_middle_above`` ###
This action moves HVAC louvers to the position one step under the topmost.

```yaml
on_...:
  then:
    - aux_ac.vlouver_middle_above: aux_id
```
- **aux_id** (**Required**, string): ID of `aux_ac` component.

### ``aux_ac.vlouver_middle`` ###
This action moves HVAC louvers to the middle position.

```yaml
on_...:
  then:
    - aux_ac.vlouver_middle: aux_id
```
- **aux_id** (**Required**, string): ID of `aux_ac` component.

### ``aux_ac.vlouver_middle_below`` ###
This action moves HVAC louvers to the position one step under the middle position.

```yaml
on_...:
  then:
    - aux_ac.vlouver_middle_below: aux_id
```
- **aux_id** (**Required**, string): ID of `aux_ac` component.

### ``aux_ac.vlouver_bottom`` ###
This action moves HVAC louvers to the lowest position.

```yaml
on_...:
  then:
    - aux_ac.vlouver_bottom: aux_id
```
- **aux_id** (**Required**, string): ID of `aux_ac` component.

### ``aux_ac.aux_ac.power_limit_off`` ###
This action disables inverter HVAC power limitation.

```yaml
on_...:
  then:
    - aux_ac.power_limit_off: aux_id
```
- **aux_id** (**Required**, string): ID of `aux_ac` component.

### ``aux_ac.power_limit_on`` ###
This action enables inverter HVAC power limitation and sets this limit value.

```yaml
on_...:
  then:
    - aux_ac.power_limit_on:
        id: aux_id
        limit: 46   # limits the maximum power of the inverter HVAC at 46%
```
- **aux_id** (**Required**, string): ID of `aux_ac` component.
- **limit** (**Optional**, integer): the maximum power of the inverter HVAC. If the power limitation is enabled, the inverter HVAC will limits its power.  
 > **Notice**, that power limitation will affect the efficiency of your HVAC. For example, a low power limit may block the possibility of the conditioner to reach user-specified room temperature, because HVAC will not have enough power for it. Keep this in mind when you are using this function.
  
 Due to hardware limitation this value should be in the range from `30%` to `100%`. The default value for `limit` is `30%` (it will be used if `limit` is omitted in configuration).



## Simple example ##
The source code of this example is located in the [aux_ac_simple.yaml](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/examples/simple/aux_ac_simple.yaml) file.

All settings in it is trivial. Just copy the file to your local folder, specify your Wi-Fi settings and compile YAML with ESPHome.


## Advanced example ##
All sources are located [in advanced example folder](https://github.com/GrKoR/esphome_aux_ac_component/tree/master/examples/advanced).

This time we'll configure two relative identical air conditioners with `aux_ac` custom component.<br />
Let's imagine we have ACs in a kitchen and in a living room. Air conditioners can be of the same brand or different brands - the main thing is that they are compatible with `aux_ac` and can be controlled using `aux_ac`.<br />  
Because we are lazy, we'll define all common configuration parts for two air conditioners in one `ac_common.yaml` file.<br />
All specific parts of configuration are located in the `ac_kitchen.yaml` and `ac_livingroom.yaml`. Here we set `devicename` and `upper_devicename` for correct sensors and component naming. And here we specify the correct IP address of the device from `secrets.yaml`.<br />
**Don't forget** to specify `wifi_ip_kitchen`, `wifi_ota_ip_kitchen`, `wifi_ip_livingroom` and `wifi_ota_ip_livingroom` in the `secrets.yaml` along with the other sensitive information, such as passwords, tokens etc.

If you try to compile `ac_common.yaml` it will raise errors. You need to compile `ac_kitchen.yaml` or `ac_livingroom.yaml` instead.
