## Tested and compatible air conditioners ##
`Aux_ac` has been tested and works successfully with the air conditioners from the list below.<br/>
Кондиционеры из списка ниже протестированы и точно совместимы с `aux_ac`.

+ AUX (models: ASW-H09A4/LK-700R1, ASW-H09B4/LK-700R1, AMWM-xxx multisplit, AL-H48/5DR2(U)/ALMD-H48/5DR2)
+ Centek (models: CT-65Q09, CT-65Z10)
+ Energolux (models: SAS09Z4-AI, SASxxBN1-AI see Note below)
+ Hyundai (models: H-AR21-07H, H-AR21-09H)
+ Idea (models: ISR-12HR-SA7-DN1 ION)
+ IGC (models: RAK-07NH multysplit)
+ Roda (models: RS-AL09F)
+ Rovex (models: RS-07ALS1, RS-09ALS1, RS-12ALS1)
+ Royal Clima (models: CO-D 18HNI/CO-E 18HNI)
+ Samurai (models: SMA-07HRN1 ION, SMA-09HRN1 ION)
+ Subtropic (models: SUB-07HN1, SUB-12HN1)

## Tested and INCOMPATIBLE air conditioners ##
ACs from the list below are **INCOMPATIBLE** with `aux_ac`.<br/>
Кондиционеры из списка ниже протестированы и оказались **НЕСОВМЕСТИМЫ** с компонентом `aux_ac`.

+ Kentatsu KSGMA26HFAN1 was tested and **isn't supported**



## Tested and PARTIAL COMPATIBLE air conditioners ##
ACs from the list below are **PARTIAL COMPATIBLE** with `aux_ac`. Details are located in the items notes.<br/>
Кондиционеры из списка ниже были протестированы и оказались частично совместимы с компонентом `aux_ac`. В чём именно заключается частичная совместимость, указано в примечании к модели.

### Energolux Bern (models: SASxxBN1-Al).###
**Notes [EN]:** This model was tested by two users and that has different results. For one of them `aux_ac` is working correctly but the second one have to modify source of component. These modifications are described below. 
The `aux_ac` source code need some modifications otherwise it will not work correct. Since the difference between the protocols is significant, it was decided not to make edits to the `aux_ac` component. We will wait for the owners of Energolux Bern air conditioners to make a public fork of the `aux_ac` component with the necessary edits. As soon as this fork will be published, a link to it will be placed here.<br/>
Here is the edits:
1. Checksum for small data packet should be reduced by one (new_CRC16 = correct_CRC16-1 ).
2. Checksum for command packet should be reduced by one too (new_CRC16 = correct_CRC16-1 ).
3. Big data packet has a checksum of 3 sometimes 4 less than the correct one. Maybe you should ignore CRC for big data packet.
4. Checksum for ping packet is the same than correct one (strange!).
5. Display status is inverted (it works when status OFF and vice versa).
6. There may be other incorrect functions, but they don't affect the main functionality.

**[RU] Примечание:** Эта модель кондиционера была протестирована двумя пользователями и они получили разные результаты. Для одного из них компонент `aux_ac` сработал штатно. А второму пользователю пришлось модифицировать исходный код компонента. С чем связано такое поведение кондиционеров - не понятно. Изменения, которые второй пользователь вносил в исходный код компонента, описаны ниже.
Для корректной работы этого кондиционера с компонентом `aux_ac` в исходники компонента необходимо внести некоторые правки. Поскольку отдичия протокола для Energolux Bern довольно значительны, решено было не вносить изменения в основную ветку компонента `aux_ac`. Поддержка таких изменений была бы сильно затруднена, поскольку у автора компонента `aux_ac` отсутствуют кондиционеры Energolux и тестировать изменения будет просто не на чем. Поэтому решено было ожидать от владельцев кондиционеров Energolux создания публичного форка компонента `aux_ac` с необходимыми изменениями. Как только такой форк появится, сюда будет размещена ссылка на этот форк.<br/>
Пока же для работы компонента необходимо внести следующие правки:
1. CRC16 для малого пакета данных должен быть уменьшен на 1 (new_CRC16 = correct_CRC16-1 ).
2. CRC16 для командного пакета также должен быть уменьшен на 1 (new_CRC16 = correct_CRC16-1 ).
3. Чексумма для большого информационного пакета может быть на 3 или на 4 меньше корректной. Возможно стоит просто игнорировать проверку CRC для этого типа пакетов.
4. Чексумма для пинг-пакета на удивление соответствует правильной.
5. Дисплей включен, когда его статус установлен в OFF. И наоборот. Это легко исправить.
6. Могут быть и другие отличия в протоколе, но они не будут влиять на основную функциональность кондиционера.
