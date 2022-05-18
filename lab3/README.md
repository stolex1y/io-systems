# Лабораторная работа 3

**Название:** "Разработка драйверов сетевых устройств"

**Цель работы:** Получить знания и навыки разработки драйверов сетевых интерфейсов для операционной системы Linux.

## Описание функциональности драйвера

Пакеты протокола UDP, адресуемые на конкретный порт. Вывести порт отправителя и получателя. Номер порта определяется исполнителями.

Состояние разбора пакетов необходимо выводить в файл в директории /proc.

## Инструкция по сборке

`make netload dest_port=<отслеживаемый порт> vni_addr=<сетевой адрес виртуального интерфейса>`

## Инструкция пользователя

* `make netload ...` -- загрузить драйвер устройства
* `make unload` -- выгрузить драйвер устройства
* `make clean` -- удалить файлы сборки

1. Загрузить драйвер
2. С помощью `ip addr show vni0` можно посмотреть, информацию о созданном сетевом интерфейсе
3. С помощью `echo 1234 | nc -u -p <порт отправителя> 127.0.0.1 <отслеживаемый порт>` можно отправить данные на отслеживаемый интерфейс и порт посредством
протокола UDP
4. Статистика для интерфейса `ip -s link show vni0`
5. Адреса отправителя и получателя `cat /proc/vni0`
6. Выгрузить драйвер 

## Примеры использования
```
stolexiy@HP:~/io-systems/lab3$ make netload
    sudo insmod /home/stolexiy/io-systems/lab3/build/vni.ko dest_port_filter=12345 debug=0 link=lo
    lsmod
    Module                  Size  Used by
    vni                    16384  0
    dmesg
    [ 1006.552312] vni: Module vni loaded
    [ 1006.552317] vni: vni create link vni0
    [ 1006.552318] vni: vni registered rx handler for lo
    [ 1006.552319] vni: destination port filter 12345
    sudo ifconfig vni0 127.0.0.15

stolexiy@HP:~/io-systems/lab3$ ip addr show vni0
8: vni0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UNKNOWN group default qlen 1000
    link/ether 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.15/8 brd 127.255.255.255 scope host vni0
       valid_lft forever preferred_lft forever
    inet6 fe80::200:ff:fe00:0/64 scope link
       valid_lft forever preferred_lft forever
       
stolexiy@HP:~/io-systems/lab3$ echo 1234 | nc -u -s 127.0.0.10 -p 2000 127.0.0.1 12345
stolexiy@HP:~/io-systems/lab3$ ip -s link show vni0
8: vni0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UNKNOWN mode DEFAULT group default qlen 1000
    link/ether 00:00:00:00:00:00 brd 00:00:00:00:00:00
    RX: bytes  packets  errors  dropped overrun mcast
    33         1        0       0       0       0
    TX: bytes  packets  errors  dropped carrier collsns
    0          0        0       0       0       0

stolexiy@HP:~/io-systems/lab3$ echo 1234 | nc -u -p 2000 127.0.0.1 12345
stolexiy@HP:~/io-systems/lab3$ cat /proc/vni0
from 127.0.0.1 2000 to 127.0.0.1 12345

stolexiy@HP:~/io-systems/lab3$ make unload
```
