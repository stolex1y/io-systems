# Лабораторная работа 2

**Название:** "Разработка драйверов блочных устройств"

**Цель работы:** Получить знания и навыки разработки драйверов блочных устройств для операционной системы Linux.

## Описание функциональности драйвера

Один первичный раздел размером 30Мбайт и один расширенный раздел, содержащий два логических раздела размером 10Мбайт каждый.

## Инструкция по сборке

`make`

## Инструкция пользователя

* `make load` -- загрузить драйвер устройства
* `make unload` -- выгрузить драйвер устройства
* `make clean` -- удалить файлы сборки

1. Собрать драйвер
2. С помощью `fdisk -l /dev/ramdev` можно посмотреть, что созданные разделы соответствуют варианту
3. Отформатировать созданные разделы
```
  mkfs.vfat /dev/ramdev1
  mkfs.vfat /dev/ramdev5
  mkfs.vfat /dev/ramdev6
```
4. Измерить скорость передачи данных между разделами виртаульного диска `dd if=/dev/ramdev1 of=/dev/ramdev5 bs=512 count=20000` и 
между разделами виратуального и реального дисков `dd if=/dev/ramdev1 of=~/io-systems/lab2/test bs=512 count=20000`
5. Выгрузить драйвер 

## Примеры использования
```
stolexiy@HP:~/io-systems/lab2$ make load
stolexiy@HP:~/io-systems/lab2$ lsmod
Module                  Size  Used by
blk_dev                16384  0

stolexiy@HP:~/io-systems/lab2$ sudo fdisk -l /dev/ramdev
Disk /dev/ramdev: 50 MiB, 52428800 bytes, 102400 sectors
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disklabel type: dos
Disk identifier: 0x36e5756d

Device       Boot Start    End Sectors Size Id Type
/dev/ramdev1          1  61439   61439  30M 83 Linux
/dev/ramdev2      61440 102399   40960  20M  5 Extended
/dev/ramdev5      61441  81919   20479  10M 83 Linux
/dev/ramdev6      81921 102399   20479  10M 83 Linux

stolexiy@HP:/dev$ sudo mkfs.vfat /dev/ramdev1
mkfs.fat 4.1 (2017-01-24)
stolexiy@HP:/dev$ sudo mkfs.vfat /dev/ramdev5
mkfs.fat 4.1 (2017-01-24)
stolexiy@HP:/dev$ sudo mkfs.vfat /dev/ramdev6
mkfs.fat 4.1 (2017-01-24)

stolexiy@HP:~/io-systems/lab2$ sudo mkdir -p /mnt/ramdev1
stolexiy@HP:~/io-systems/lab2$ sudo mount /dev/ramdev1 /mnt/ramdev1
stolexiy@HP:~/io-systems/lab2$ sudo mkdir -p /mnt/ramdev5
stolexiy@HP:~/io-systems/lab2$ sudo mount /dev/ramdev5 /mnt/ramdev5
stolexiy@HP:~/io-systems/lab2$ mount
...
/dev/ramdev1 on /mnt/ramdev1 type vfat (rw,relatime,fmask=0022,dmask=0022,codepage=437,iocharset=iso8859-1,shortname=mixed,errors=remount-ro)
/dev/ramdev5 on /mnt/ramdev5 type vfat (rw,relatime,fmask=0022,dmask=0022,codepage=437,iocharset=iso8859-1,shortname=mixed,errors=remount-ro)

stolexiy@HP:~/io-systems/lab2$ sudo dd if=/dev/ramdev1 of=/dev/ramdev5 bs=512 count=20000
20000+0 records in
20000+0 records out
10240000 bytes (10 MB, 9.8 MiB) copied, 0.0433904 s, 236 MB/s
stolexiy@HP:~/io-systems/lab2$ sudo dd if=/dev/ramdev1 of=~/io-systems/lab2/test bs=512 count=20000
20000+0 records in
20000+0 records out
10240000 bytes (10 MB, 9.8 MiB) copied, 0.107432 s, 95.3 MB/s

stolexiy@HP:~/io-systems/lab2$ sudo umount /mnt/ramdev1
stolexiy@HP:~/io-systems/lab2$ sudo umount /mnt/ramdev5

stolexiy@HP:~/io-systems/lab2$ make unload
sudo rmmod blk_dev
```
