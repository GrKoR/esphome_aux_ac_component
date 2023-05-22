# How to request a feature for a component #

With any functions, the alignment is such that they simply cannot be written down by name.
We need a log from someone who has air conditioner with such functions. If you are such a person, then you can help yourself and the community.

To capture the log you need to do the following steps:
1. Run a specially written [tool](https://github.com/GrKoR/ac_python_logger) to collect logs.
2. Turn on the AC.
3. Wait 10+ seconds. (During this time, the ESP will receive all packets from AC).
4. Turn on the desired function using AC's IR remote.
5. Wait 10+ seconds ones more. While you are waiting, you can write down what you have done.
6. Turn off the desired function.
7. Wait 10+ seconds again and write down what you've done.
8. Repeat steps 4..7 for all other functions you interested in.
9. Stop the log recording with a script.
10. Send collected log and your notes (explanations to the log) to [issues](https://github.com/GrKoR/esphome_aux_ac_component/issues) or to [telegram chat](https://t.me/aux_ac).

Instead of a Python script from the step #1, you can simply save the logs from the esphome web-interface with copy-paste or from the command line, but there is a lot of extra stuff there. And it's easy to miss something. But in principle, it is also quite a working option.