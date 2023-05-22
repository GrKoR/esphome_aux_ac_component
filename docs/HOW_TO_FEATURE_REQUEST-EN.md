# How to request a feature for a component #

With any functions, the alignment is such that they simply cannot be written down by name.
We need a log from someone who has such functions.

The sequence of actions is as follows:
1. Run a specially written [tool](https://github.com/GrKoR/ac_python_logger) to collect logs and turn on the AC.
2. Wait 10+ seconds for all possible packet types to pass through the UART to the ESP.
3. Turn on the desired function.
4. Again I wait 10+ seconds. While you are waiting, you write down in a separate text what you did.
5. Turn off the desired function.
6. Again I wait 10+ seconds. And write down what you did.
7. Repeat steps 3..6 for all other functions you test.
8. Stop the log recording with a script.

Instead of a Python script, you can simply save the logs from the web-interface with copy-paste or from the command line, but there is a lot of extra stuff there. And it's easy to miss something. But in principle, it is also quite a working option.

As a result of all the efforts, you send the collected log and your notes (explanations to the log) to [issues](https://github.com/GrKoR/esphome_aux_ac_component/issues) or to [chat](https://t.me/aux_ac) . Using them, you can try to decipher the functionality and then you can file a new feature.
This is how we deciphered the fixed positions of the blinds at the air conditioner. Here you can see in [chat](https://t.me/aux_ac/6308).
