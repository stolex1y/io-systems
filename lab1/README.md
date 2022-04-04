# Лабораторная работа 1

**Название:** "Разработка драйверов символьных устройств"

**Цель работы:** Получить знания и навыки разработки драйверов символьных
устройств для операционной системы Linux.

## Описание функциональности драйвера

При записи в файл символьного устройства текста, содержащего цифры, должен запоминаться результат суммы всех чисел, разделенных другими символами (буквы, пробелы и т.п.). Последовательность полученных результатов с момента загрузки модуля ядра должна выводиться при чтении созданного файла /proc/varN в консоль пользователя.

При чтении из файла символьного устройства в кольцевой буфер ядра должен осуществляться вывод тех же данных, которые выводятся при чтении файла /proc/varN.

## Инструкция по сборке

`make`

## Инструкция пользователя

* `make load` -- загрузить драйвер устройства
* `dmesg` -- вывести сообщения из кольцевого буфера ядра
* `/dev/chdev` -- файл символьного устройства
* `make unload` -- выгрузить драйвер устройства
* `make clean` -- удалить файлы сборки

## Примеры использования

```
stolexiy@HP:~/io-systems/lab1$ make load
stolexiy@HP:~/io-systems/lab1$ echo "1   1   2 asdf 3" > /dev/chdev
stolexiy@HP:~/io-systems/lab1$ cat /proc/var3
[7]
stolexiy@HP:~/io-systems/lab1$ cat /dev/chdev
stolexiy@HP:~/io-systems/lab1$ dmesg
[37463.956326] chdev driver: init
[37650.166474] chdev driver: [7]

stolexiy@HP:~/io-systems/lab1$ echo "" > /dev/chdev
stolexiy@HP:~/io-systems/lab1$ cat /proc/var3
[7 0]
stolexiy@HP:~/io-systems/lab1$ cat /dev/chdev
stolexiy@HP:~/io-systems/lab1$ dmesg
[37463.956326] chdev driver: init
[37650.166474] chdev driver: [7]
[37766.595613] chdev driver: [7 0]

stolexiy@HP:~/io-systems/lab1$ make unload
sudo rmmod char_driver
stolexiy@HP:~/io-systems/lab1$ dmesg
[37463.956326] chdev driver: init
[37650.166474] chdev driver: [7]
[37766.595613] chdev driver: [7 0]
[37861.670926] chdev driver: exit
```
