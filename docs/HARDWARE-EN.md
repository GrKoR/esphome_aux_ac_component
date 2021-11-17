## Hardware ##
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