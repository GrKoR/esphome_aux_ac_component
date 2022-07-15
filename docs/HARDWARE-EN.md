## Hardware ##
I tested it with an esp8266 chip (esp-12e). Minimal scheme:  
![scheme](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/scheme.png?raw=true)
 
At the first time in addition to scheme above IO0 (GPIO0) must be pulled down to GND at the boot and ESPHome can be uploaded through UART0. If your ESPHome configuration contains OTA you can pull up IO0 or leave it floating. All further updates can be uploaded over-the-air.  
I leave GPIO0 in air cause I don't see any reason to solder additional components for single use.

ESP-12E before DC-DC and air conditioner connected:  
![esp-12e minimal photo](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/esp-12e.jpg?raw=true)
 
Air conditioner internal block has a 5-wire connection to the wifi-module. Connector is [JST SM](https://www.jst-mfg.com/product/pdf/eng/eSM.pdf).
 
## Wires ##
1. Yellow: +14V DC. Measured +14.70V max and +13.70V min. Service manual declares up to +16V.
2. Black: ground.
3. White: +5V DC (max: +5.63V; min: +4.43V) I have no idea what this is for. It goes directly to the air conditioner microcontroller through resistor 1kOhm and it does not affect the operation of the module.
4. Blue: TX of air conditioner. High is +5V.
5. Red: RX of air conditioner. High is +5V.
 
For power supply it is possible to use any kind of suitable modules. I use this:  
![power module](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/DD4012SA.jpg?raw=true).

## Connections ##
Black wire of AC's connector goes to the middle pin of the power module and to the GND pin of esp-12e.  
Yellow wire is connected to the Vin pin of the power module.  
Blue wire is connected to the RXD pin of esp-12e.  
Red wire is connected to the TXD pin of esp-12e.  

**ATTENTION!** In case you are using board like NodeMCU instead of clean esp8266/esp32 module, you shouldn't connect RX & TX wires of air conditioneer to TX & RX pins of board. Use any other digital pins for UART connection. It doesn't matter if your board will use hardware or software UART. All UART types are working well.  
The usage of alternate pins for NodeMCU-like boards is necessary cause RX & TX lines of this boards are often have additional components like resistors or USB-TTL converters connected. This components are violate esp-to-ac UART connection.

Here is it:  
![connections](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/connections.png?raw=true)
 
All connections in custom 3d-printed case looks like this:  
![module assembled](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/assembled.JPG?raw=true)
 
Cause I haven't JST SM connector I made own:  
![JST SM connector replica](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/connector.JPG?raw=true).
 
It is made of standard 2.54mm pins and 3D-printed case.  
All models for 3D-printing are available too: [STL-files for connector](https://github.com/GrKoR/esphome_aux_ac_component/tree/master/enclosure/JST%20SM%20connector), [models of case parts](https://github.com/GrKoR/esphome_aux_ac_component/tree/master/enclosure/case). 
 
## The result ##
![photo 1](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/real-1.JPG?raw=true)  
![photo 2](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/real-2.JPG?raw=true)  
![photo 3](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/real-3.JPG?raw=true)