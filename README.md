# WebLamp

## Описание устройства

Парный светильник, который можно подарить своим друзьям. Цвет светильника синхронизируется с цветом светильников Ваших друзей.

Лампа будет медленно "дышать", если хотя бы один спаренный светильник сейчас находится в сети.
Лампа будет интенсивно "дышать", если хотя у одного спаренного светильника сработал ИК-датчик.

Используя кнопку на устройстве можно:
- Одно нажатие - выключить подстветку;
- Двойное нажатие - сменить цвет;
- Тройное нажатие - подмигнуть, спаренные светильники мигнут 3 раза;
- Зажать кнопку - изменение яркости, поочередно повышение яркости и уменьшение (*не синхронизиуется*).

## Внешний вид устройства

*типа картинка с подписями*

## Описание режимов работы
Режим загрузки включается сразу после подачи питания на устройство.

Во время загрузки можно принудительно перейти в режим собственной точки доступа и оффлайн режим.
- Для перехода в режим собственной точки доступа, необходимо во время загрузки один раз нажать на кнопку.
- Для перехода в оффлайн режим, необходимо нажать на кнопку находясь в режиме собственной точки доступа.

### Загрузка
Лампа мерцает следующими цветами:
- Желтый - ожидание (2 сек);
- Зеленый - подключение к Wi-Fi;
- Синий - создана собственная Wi-Fi точка доступа для настройки.

### Онлайн
Лампа синхронизиуется цветами со спаренными лампами.
Есть доступ к веб-интерфейсу для настройки.

### Оффлайн
Лампа отключена от сети. 
Ночной режим не доступен.
Синхронизация цвета невозможна.

## Начало работы с лампой

### Первоначальная настройка

При первом запуске устройство, не обнаружив данных Wi-Fi, включит мерцающую анимацию синего цвета и запустит собственную точку доступа с именем "WebLamp ...", где вместо точек будет последний успешно полученный IP-адрес лампы.

Необходимо подключиться к данной Wi-Fi сети. После подключения автоматически откроется окно настроек устройства, в случае мобильных устройств появится пуш-уведомленние об авторизации, на которое необходимо нажать. Если ничего из выше перечисленного не произошло, тогда необходимо в браузере перейти по адресу [192.168.4.1](http://192.168.4.1/) либо по адресу [weblamp.local](http://weblamp.local/).

На странице настройки устройства в разделе WIFI необходимо ввести данные Wi-Fi сети с доступом в интернет. Остальные настройки можно выполнить позднее. После нажатия на кнопку сохранить, лампа перезагрузится и попытается подключиться к Wi-Fi сети, указанной ранее.

### Настройка

После успешного подключения к Wi-Fi сети в интерфейс настройки устройства можно попасть с ПК введя в браузере [weblamp.local](http://weblamp.local/), либо по IP-адресу назначенному лампе. С мобильных устройств подключение возможно только по IP адресу.

Если подключиться к лампе по адресу [weblamp.local](http://weblamp.local/) не удается, узнать IP-адрес лампы можно перезагрузив её и во время загрузки нажать на кнопку один раз. Лампа включит собственную точку доступа, в названии которой будет присутствовать последний полученный IP-адрес.

Доступные для настройки параметры:

Раздел Settings:
Можно включать и выключать подстветку светильников.
Можно настраивать цвет светильников.
Можно настраивать яркость свечения.

Раздел MQTT:
Здесь указывается уникальное имя Вашего устройства в сети, а также до двух имен парных ламп.

Раздел NTP:
Здесь можно настроить работу "ночного" режима.
Указывается начало и конец "ночного" периода, а также часовой пояс, в котором находится лампа.

Раздел Miscellaneous:
Здесь настраиваются параметры работы "спящего" режима.
Можно разрешить или запретить работу "спящего" режима, а также указать время отсутствия активности до перехода в спящий режим.

## Функции работы

Между светильниками синхронизируется цвет и состояние питания, яркость свечения не синхронизируется.

Ночной режим - устройство уменьшает максимальную яркость свечения в течение указанного в настройках периода.

Спящий режим - лампа автоматически выключает подстветку, если ИК-датчик не обнаруживал движения или кнопка не была нажата в течение указанного периода. 
Выход из данного режима осуществляется автоматически при помощи ИК-датчика или при нажатии на кнопку.

## Обновление прошивки

Для обновления прошивки необходимо загрузить файл прошивки на ПК и открыть страницу по адресу http://weblamp.local:8080/update/. На странице нажать кнопку с подписью Firmware и выбрать загруженный файл прошивки. В случае успешной загрузки прошивки устройство перезагрузится уже с обновленной прошивкой. Текущую версию прошивки можно узнать в конце страницы настройки.
