KuroBox Bootloader
==================

Code
----

This code is heavily influenced by mabl's stm32f107 bootloader:
https://github.com/mabl/ARMCM3-STM32F107-BOOTLOADER
And by "heavily influenced" I mean that I forked his code, and then chucked
most of it out except for basic structure and the hex8 reading code.

This code is customised for the STM32F4, mainly due to it's memory layout
which is 4x16kbyte, 1x64kbyte, 7x128kbyte sectors.

The code checks for presense of SD, reads the .HEX, and flashes it. If there's
any issues, it will blink the RED led a set number of times, and jump to the
app start anyway. This bootloader also checks that the destination address
does not overwrite the bootloader itself, so you should always be able to
reflash no matter what.

There are more modifications to be made, and, as always, you should refer
to the code to find out what it's actually doing.

License
-------

The code for my portion of the code is licensed under GPL3+. Other parts 
of the code are licensed according to what is stated in those files.

The iHex8 reader code comes from:
https://github.com/vsergeev/libGIS
