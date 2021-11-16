## Электроника, необходимая ля управления кондиционером по wifi ##
Я тестировал проект на esp8266 (esp-12e). Минимальная обвязка традиционная и выглядит так:<br /> 
![scheme](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/scheme.png?raw=true)

Для прошивки esp8266 в первый раз нужно в дополнение к обвязке, показанной на схеме выше, притянуть к Земле пин IO0 (GPIO0). После этого ESPHome может быть загружена в esp8266 по UART0. Если при этом вы указали OTA в конфигурации ESPHome, то в дальнейшем пин IO0 можно подтянуть к питанию или оставить висеть в воздухе. Он никак не будет влиять на загрузку новых прошивок, потому что все апдейты можно будет делать "по воздуху" (то есть по wifi). Я никуда IO0 не подтягивал и ничего к нему не паял, потому что не вижу смысла это делать ради одного раза. Первую прошивку делал в самодельном переходнике на макетке.

Плата esp-12e перед подключением кондиционера и модуля питания:<br /> 
![esp-12e minimal photo](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/esp-12e.jpg?raw=true)

Внутренний блок сплит-системы имеет 5-проводное подключение к модулю wifi. Коннектор [JST SM](https://www.jst-mfg.com/product/pdf/eng/eSM.pdf).
 
Перечень проводников:
1. Желтый: +14В постоянного тока. Осциллограф показал от +13.70В до +14.70В. В сервисном мануале встречалось, что питание возможно до +16В.
2. Черный: земля.
3. Белый: +5В постоянного тока (измерено от +4.43В до +5.63В). Для чего нужна эта линия - не понятно. У меня нет версий. Эксперименты с родным wifi-модулем сплит-системы показали, что эта линия в работе wifi не участвует. Линия идет напрямую на ножку контроллера в сплите через резистор 1 кОм.
4. Синий: TX кондиционера. Высокий уровень +5В.
5. Red: RX кондиционера. Высокий уровень +5В.

Для питания ESP8266 можно использовать любой подходящий DC-DC преобразователь. Я использовал такой:<br /> 
![power module](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/DD4012SA.jpg?raw=true).

Подключение:<br />
Черный провод (земля) подключается к земле DC-DC преобразователя и к пину GND модуля ESP8266.<br />
Желтый провод подключается ко входу DC-DC преобразователя (в моём случае контакт Vin).<br />
Синий провод подключается к пину RXD модуля esp-12e.<br />
Красный провод подключается к пину TXD модуля esp-12e.<br />

Вот схема всех соединений:<br />
![connections](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/connections.png?raw=true)
 
Вот так это выглядит внутри самодельного корпуса:<br /> 
![module assembled](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/assembled.JPG?raw=true)
 
Поскольку у меня не было под рукой коннекторов JST SM, а ехать искать их не хотелось, я сделал свой собственный из стандартных пинов с шагом 2,54 мм и нескольких напечатанных на 3D-принтере деталей:<br /> 
![JST SM connector replica](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/connector.JPG?raw=true).
 
Все относящиеся к проекту модели для 3D-принтера также доступны: [STL-файлы коннектора](https://github.com/GrKoR/esphome_aux_ac_component/tree/master/enclosure/JST%20SM%20connector), [модельки частей корпуса](https://github.com/GrKoR/esphome_aux_ac_component/tree/master/enclosure/case). 
 
Конечный результат:<br /> 
![photo 1](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/real-1.JPG?raw=true)<br />
![photo 2](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/real-2.JPG?raw=true)<br />
![photo 3](https://github.com/GrKoR/esphome_aux_ac_component/blob/master/images/real-3.JPG?raw=true)