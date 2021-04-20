# ESPHome AUX air conditioner custom component (aux_ac) #
For communication about this project [join telegram chat](https://t.me/aux_ac). 

## DISCLAIMER ##
This software and hardware are provided 'AS IS'. Everything you do with your devices you are doing at your own risk. If you aren't strongly understand what you are doing, just buy wifi-module from your air conditioner manufacturer.


## Short description ##
This custom component allow you to control your air conditioner through wifi if it is made in the AUX factory.
Component tested with ESPHome 1.15.3 and Rovex ALS1 air conditioner. It looks like many other air conditioners can be controlled by aux_ac but this possibility isn't tested. See list of testetd ACs below for more details.

For issues, please go to [the issue tracker](https://github.com/esphome/issues/issues).

For feature requests, please see [feature requests](https://github.com/esphome/feature-requests/issues).


## Supported air conditioners ##

### List of compatible ACs (tested) ###
This ACs were tested by author or by users.
+ Rovex (models: ALS1)


### List of potential compatible ACs ###
**NOT TESTED! TRY AT YOUR OWN RISK!**
AUX is one of OEM air conditioner manufacturer. They produce ACs for many brands.
Internet says that folowing air conditioners may test with aux_ac component:
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

If User Manual of your HVAC describes connection to wifi with mobile app ACFreedom it seems you may go deeper with aux_ac. But try all soft and hardware for your own risk. You must clearly understand what you are doing. It is better not to use the aux_ac component if you are unsure.

If you are tested your air conditioner and aux_ac works with it please let me know about it. I'll add this info to the list of tested ACs above.

## How to use it ##
### Hardware ###
I tested it with esp8266 chip (esp-12e). Minimal scheme:
![scheme](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/scheme.png?raw=true)
![esp-12e minimal photo](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/esp-12e.jpg?raw=true)

At the first time IO0 (GPIO0) must be pulled down to GND at the boot and ESPHome can be uploaded through UART0. If your ESPHome configuration contains OTA you can pullup IO0 or leave it floating. All further updates can be uploaded over-the-air.

Air conditioner internal block has 5-wire connection to the wifi-module. Connector is [JST SM](https://www.jst-mfg.com/product/pdf/eng/eSM.pdf).

Wires:
1. Yellow: +14V DC. Measured +14.70V max and +13.70V min. Service manual declares up to +16V).
2. Black: ground.
3. White: +5V DC (max: +5.63V; min: +4.43V) I have no idea what this is for. It goes directly to air conditioner microcontroller through resistor 1kOhm.
4. Blue: TX of air conditioner. High is +5V.
5. Red: RX of air conditioner. High is +5V.

For power supply it is possible to use any kind of modules. I use this:
![power module](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/DD4012SA.jpg?raw=true).

Black wire of AC's connector goes to middle pin of power module and to GND pin of esp-12e.
Yellow wire is connected to Vin pin of power module.
Blue wire is connected to RXD pin of esp-12e.
Red wire is connected to TXD pin of esp-12e.

All connections in custom 3d-printed case looks like this:
![module assembled](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/assembled.JPG?raw=true)

Cause I haven't JST SM connector I made own:
![JST SM connector replica](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/connector.JPG?raw=true)
It mades of standart 2.54mm pins and 3D-printed case.

All models for 3D-printing are available too: [STL-files for connector](https://github.com/GrKoR/esphome_aux_ac_component/tree/master/enclosure/JST%20SM%20connector), [models of case parts](https://github.com/GrKoR/esphome_aux_ac_component/tree/master/enclosure/case). 

Final:
![photo 1](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/real-1.JPG?raw=true)
![photo 2](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/real-2.JPG?raw=true)
![photo 3](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/real-3.JPG?raw=true)

### Firmware: Integration aux_ac to your configuration ###
Copy aux_ac_custom_component.h to folder with your ESPHome YAML file.

At the header of your YAML add include instruction like this:
```
esphome:
  name: $devicename
  platform: ESP8266
  board: esp12e
  includes:
    - aux_ac_custom_component.h
```

Configure UART to communicate with air conditioner:
```
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
```
logger:
    level: DEBUG
    # important: for avoiding collisions logger works with UART1 (for esp8266 tx = GPIO2, rx = None)
    hardware_uart: UART1
```

Finally define climate component:
```
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

## Additional functionality ##
Aux_ac component provides three additional sensors: two temperatures and firmware version.

### Ambient temperature ###
This is current room air temperature from AC's sensor. If you need it in your configuration place this code to YAML file:
```
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
Currently it shows weather on Mars =) May be it will change if we get more statistics and some smart guys for decoding.
If in spite of everthing, you still want it in your configuration, just use this code:
```
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
```
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
```
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