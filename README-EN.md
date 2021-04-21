# ESPHome AUX air conditioner custom component (aux_ac) #
For communication about this project [join telegram chat](https://t.me/aux_ac). 
 
For issues or feature requests, please go to [the issue section](https://github.com/GrKoR/esphome_aux_ac_component/issues).
 
 
## DISCLAIMER ##
This software and hardware are provided **'AS IS'**. Everything you do with your devices you are doing at your own risk. If you don't strongly understand what you are doing, just buy wifi-module from your air conditioner manufacturer.
 
 
## Short description ##
This custom component allows you to control your air conditioner through wifi if it is made in the AUX factory.
Component tested with ESPHome 1.15.3 and Rovex ALS1 air conditioner. It looks like many other air conditioners can be controlled by aux_ac but this possibility isn't tested. See list of testetd ACs below for more details.
 
For issues, please go to [the issue tracker](https://github.com/esphome/issues/issues).
 
For feature requests, please see [feature requests](https://github.com/esphome/feature-requests/issues).
 
 
## Supported air conditioners ##
 
### List of compatible ACs (tested) ###
These ACs were tested by the author or by users.
+ Rovex (models: ALS1)
 
 
### List of potential compatible ACs ###
**NOT TESTED! TRY AT YOUR OWN RISK!**
AUX is one of the OEM air conditioner manufacturers. They produce ACs for many brands.
Internet says that following air conditioners may test with aux_ac component:
+ AUX
+ Abion
+ AC ELECTRIC
+ Almacom
+ Ballu
+ CENTEK
+ Energolux
+ ERISSON
+ Green Energy
+ Hyundai
+ Kentatsu (some series)
+ Klimaire
+ KOMANCHI
+ LANZKRAFT
+ LEBERG
+ Monroe
+ Neoclima
+ NEOLINE
+ Pioneer (before 2016)
+ Roda
+ Royal Clima
+ SAKATA
+ SATURN
+ Scarlett
+ SmartWay
+ SUBTROPIC
+ Supra
+ Vertex
+ Zanussi
 
If the User Manual of your HVAC describes connection to wifi with mobile app ACFreedom it seems you may go deeper with aux_ac. But try all soft and hardware for your own risk. You must clearly understand what you are doing. It is better not to use the aux_ac component if you are unsure.
 
If you are tested your air conditioner and aux_ac works with it please let me know about it. I'll add this info to the list of tested ACs above.
 
## How to use it ##
### Hardware ###
I tested it with an esp8266 chip (esp-12e). Minimal scheme:
 
![scheme](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/scheme.png?raw=true)
 
In real life looks minimalistic too:
 
![esp-12e minimal photo](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/esp-12e.jpg?raw=true)
 
At the first time IO0 (GPIO0) must be pulled down to GND at the boot and ESPHome can be uploaded through UART0. If your ESPHome configuration contains OTA you can pull up IO0 or leave it floating. All further updates can be uploaded over-the-air.
 
Air conditioner internal block has a 5-wire connection to the wifi-module. Connector is [JST SM](https://www.jst-mfg.com/product/pdf/eng/eSM.pdf).
 
Wires:
1. Yellow: +14V DC. Measured +14.70V max and +13.70V min. Service manual declares up to +16V.
2. Black: ground.
3. White: +5V DC (max: +5.63V; min: +4.43V) I have no idea what this is for. It goes directly to the air conditioner microcontroller through resistor 1kOhm and it does not affect the operation of the module.
4. Blue: TX of air conditioner. High is +5V.
5. Red: RX of air conditioner. High is +5V.
 
For power supply it is possible to use any kind of suitable modules. I use this:
 
![power module](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/DD4012SA.jpg?raw=true).
 
Black wire of AC's connector goes to the middle pin of the power module and to the GND pin of esp-12e.
Yellow wire is connected to the Vin pin of the power module.
Blue wire is connected to the RXD pin of esp-12e.
Red wire is connected to the TXD pin of esp-12e.
 
All connections in custom 3d-printed case looks like this:
 
![module assembled](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/assembled.JPG?raw=true)
 
Cause I haven't JST SM connector I made own:
 
![JST SM connector replica](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/connector.JPG?raw=true).
 
It is made of standard 2.54mm pins and 3D-printed case.
 
All models for 3D-printing are available too: [STL-files for connector](https://github.com/GrKoR/esphome_aux_ac_component/tree/master/enclosure/JST%20SM%20connector), [models of case parts](https://github.com/GrKoR/esphome_aux_ac_component/tree/master/enclosure/case). 
 
The result:
 
![photo 1](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/real-1.JPG?raw=true)
 
![photo 2](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/real-2.JPG?raw=true)
 
![photo 3](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/real-3.JPG?raw=true)
 
 
 
### Firmware: Integration aux_ac to your configuration ###
Copy aux_ac_custom_component.h to folder with your ESPHome YAML file.
 
At the header of your YAML add include instruction like this:
```yaml
esphome:
  name: $devicename
  platform: ESP8266
  board: esp12e
  includes:
    - aux_ac_custom_component.h
```
 
Configure UART to communicate with air conditioner:
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
 
ESP8266 has two hardware UARTs: UART0 and UART1. Only UART0 suits for aux_ac cause only it has both TX and RX. In **uart:** section above we configure UART0 for aux_ac. But it used by **logger:**. So it is necessary to redefine UART for logger:
```yaml
logger:
    level: DEBUG
    # important: for avoiding collisions logger works with UART1 (for esp8266 tx = GPIO2, rx = None)
    hardware_uart: UART1
```
 
Finally define climate component:
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
 
## Example ##
Files `ac_common.yaml`, `ac_kitchen.yaml` and `ac_livingroom.yaml` show standard way to use aux_ac custom component.
`ac_common.yaml` contains a common configuration part for two air conditioners. One of the ACs is located in a kitchen, the second one is in a living room.
`ac_kitchen.yaml` and `ac_livingroom.yaml` contain specific parts of configuration: IP-addresses, device names etc.
If you try to compile `ac_common.yaml` it will raise errors. You need to compile `ac_kitchen.yaml` or `ac_livingroom.yaml` instead.
 
## Additional functionality ##
Aux_ac component provides three additional sensors: two temperatures and firmware version.
 
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
Currently it shows weather on Mars =) Maybe it will change if we get more statistics and some smart guys for decoding.
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
Aux_ac component also gives information about source code version. You can add it to your config with this code:
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
