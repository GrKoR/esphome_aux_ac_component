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
### List of compatible ACs (tested) ###
These ACs were tested by the author or by users.
+ AUX (models: ASW-H09A4/LK-700R1, ASW-H09B4/LK-700R1, AMWM-xxx multysplit)
+ Centek (models: CT-65Q09, CT-65Z10)
+ Hyundai (models: H-AR21-09H)
+ IGC (models: RAK-07NH multysplit)
+ NEOLINE (models: NAC-07HN1)
+ Roda (models: RS-AL09F)
+ Rovex (models: RS-07ALS1, RS-09ALS1, RS-12ALS1)
+ Samurai (models: SMA-07HRN1 ION, SMA-09HRN1 ION)
+ Subtropic (models: SUB-07HN1_18Y)


### List of potential compatible ACs ###
**NOT TESTED! TRY AT YOUR OWN RISK!**<br />
AUX is one of the OEM air conditioner manufacturers. AUX produce ACs for many brands.

Internet says that following air conditioners may work with `aux_ac` component:
+ Abion
+ AC ELECTRIC
+ Almacom
+ Ballu
+ Climer
+ DAX
+ Energolux
+ ERISSON
+ Green Energy
+ Kentatsu (some series; Kentatsu KSGMA26HFAN1 was tested and **isn't supported**)
+ Klimaire
+ KOMANCHI
+ LANZKRAFT
+ LEBERG
+ LGen
+ Monroe
+ Neoclima
+ One Air
+ Pioneer (до 2016 года)
+ Royal Clima
+ SAKATA
+ SATURN
+ Scarlett
+ SmartWay
+ Soling
+ SUBTROPIC
+ Supra
+ Timberk
+ Vertex
+ Zanussi

If your AC is listed above you should take a closer look at `aux_ac`.<br />
If the User Manual of your HVAC describes connection to wifi with mobile app ACFreedom it seems you may go deeper with `aux_ac`. But try all soft and hardware for your own risk. You must clearly understand what you are doing.<br />
If you are unsure it is better to wait while other users will test your model of AC (but it may never). Or please [go to telegram-chat](https://t.me/aux_ac) with your questions. Maybe you will get help there.

If you have tested your air conditioner and `aux_ac` works with it please let me know about it. I'll add this info to the list of tested ACs above.
The best way to report about your test results is write a message [in the issue section](https://github.com/GrKoR/esphome_aux_ac_component/issues). Direct message in the [telegram](https://t.me/aux_ac) is possible too but probably I can miss your message among many others.


## How to use it ##
### Hardware ###
I tested it with an esp8266 chip (esp-12e). Minimal scheme:<br />
![scheme](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/scheme.png?raw=true)
 
At the first time in addition to scheme above IO0 (GPIO0) must be pulled down to GND at the boot and ESPHome can be uploaded through UART0. If your ESPHome configuration contains OTA you can pull up IO0 or leave it floating. All further updates can be uploaded over-the-air.<br />
I leave GPIO0 in air cause I don't see any reason to solder additional components for single use.

ESP-12E before DC-DC and air conditioner connected:<br />
![esp-12e minimal photo](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/esp-12e.jpg?raw=true)
 
Air conditioner internal block has a 5-wire connection to the wifi-module. Connector is [JST SM](https://www.jst-mfg.com/product/pdf/eng/eSM.pdf).
 
Wires:
1. Yellow: +14V DC. Measured +14.70V max and +13.70V min. Service manual declares up to +16V.
2. Black: ground.
3. White: +5V DC (max: +5.63V; min: +4.43V) I have no idea what this is for. It goes directly to the air conditioner microcontroller through resistor 1kOhm and it does not affect the operation of the module.
4. Blue: TX of air conditioner. High is +5V.
5. Red: RX of air conditioner. High is +5V.
 
For power supply it is possible to use any kind of suitable modules. I use this:<br />
![power module](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/DD4012SA.jpg?raw=true).
 
Black wire of AC's connector goes to the middle pin of the power module and to the GND pin of esp-12e.<br />
Yellow wire is connected to the Vin pin of the power module.<br />
Blue wire is connected to the RXD pin of esp-12e.<br />
Red wire is connected to the TXD pin of esp-12e.<br />

Here is it:<br />
![connections](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/connections.png?raw=true)
 
All connections in custom 3d-printed case looks like this:<br />
![module assembled](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/assembled.JPG?raw=true)
 
Cause I haven't JST SM connector I made own:<br />
![JST SM connector replica](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/connector.JPG?raw=true).
 
It is made of standard 2.54mm pins and 3D-printed case.<br />
All models for 3D-printing are available too: [STL-files for connector](https://github.com/GrKoR/esphome_aux_ac_component/tree/master/enclosure/JST%20SM%20connector), [models of case parts](https://github.com/GrKoR/esphome_aux_ac_component/tree/master/enclosure/case). 
 
Here is the result:<br />
![photo 1](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/real-1.JPG?raw=true)<br />
![photo 2](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/real-2.JPG?raw=true)<br />
![photo 3](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/real-3.JPG?raw=true)



### Firmware: Integration aux_ac to your configuration ###
1. Copy aux_ac_custom_component.h to folder with your ESPHome YAML file.
2. At the header of your YAML add include instruction like this:
```yaml
esphome:
  name: $devicename
  platform: ESP8266
  board: esp12e
  includes:
    - aux_ac_custom_component.h
```
3. Configure UART to communicate with air conditioner:
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
4. ESP8266 has two hardware UARTs: UART0 and UART1. Only UART0 suits for `aux_ac` cause only it has both TX and RX. In **uart:** section above we configure UART0 for `aux_ac`. But it used by **logger:**. So it is necessary to redefine UART for logger or switch it off:
```yaml
logger:
    level: DEBUG
    baud_rate: 0
    # set hardware_uart to UART1 and comment out baud_rate above in case of boot crashes
    # it is suitable if you need hardware loggin
    # hardware_uart: UART1
```
5. Finally define climate component:
```yaml
climate:
- platform: custom
  lambda: |-
    extern AirCon acAirCon;
    if (!acAirCon.get_initialized()) acAirCon.initAC(id(ac_uart_bus));
    App.register_component(&acAirCon);
    return {&acAirCon};
  climates:
    - name: "My awesome air conditioner"
```

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
 
## Additional functionality ##
`Aux_ac` component provides three additional sensors: two temperatures and firmware version.
 
### Ambient temperature ###
This is the current room air temperature from AC's sensor. If you need it in your configuration place this code to YAML file:
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
 
### Outdoor temperature ###
Currently it shows weather on Mars =) Maybe it will change if we get more statistics and some smart guys for decoding.<br />
If in spite of everything, you still want it in your configuration, just use this code:
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
 
### Both temperatures in one declaration ###
It is possible to add room and outdoor temperatures to your configuration with one yaml block:
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
 
### Firmware version ###
`Aux_ac` component also gives information about source code version. You can add it to your config with this code:
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
