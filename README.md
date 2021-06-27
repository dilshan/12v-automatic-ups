# 12V Uninterruptible Power Supply

![Prototype version of 12V UPS system](https://raw.githubusercontent.com/dilshan/12v-automatic-ups/main/resources/12v-psu-pcb.JPG)

This 12V uninterruptible power supply initially designs to drive my fiber optic modem/router. The key reason to build this power supply is to get continuous internet and phone connection during power failures. Core components of this power supply are a constant voltage charger, 12V DC power supply, AC line monitoring unit, and 12V high capacity sealed lead-acid battery. The entire system designs using locally available components.

The charging circuit of this system builds around using the popular *[LM350](https://www.ti.com/lit/ds/snvs772b/snvs772b.pdf)* voltage regulator. This regulator calibrates to provide both 14.4V fast charging and 13.6V trickle charging. Based on the condition of the battery, the MCU will determine the appropriate charging mode.

At the online state, the *[LM2576-12](https://www.ti.com/lit/ds/symlink/lm2576.pdf)* switching regulator provides 12V output to the driving system. (in my arrangement, a fiber optic router).

![Block diagram of the UPS system](https://raw.githubusercontent.com/dilshan/12v-automatic-ups/main/resources/12v-ups-block-en.png)

The *[PIC16F688](http://ww1.microchip.com/downloads/en/devicedoc/41203d.pdf)* MCU monitors the AC line and battery to controls the output voltage of the *LM350* regulator and the output relay. The firmware of this MCU written using the *[Microchip XC8](https://www.microchip.com/en-us/development-tools-tools-and-software/mplab-xc-compilers)* compiler, and all the source code and configuration files are available to download in this project repository.

The sealed 12V lead-acid battery is the most crucial component of this unit. 12V, 9Ah battery is the most recommended for this system. At the time of this writing, this battery cost around Rs. 3500 (US$ 17.60), and I bought it from a local online store.

In online mode, this system provides 12V output. In offline mode, if it has a fully charged battery, it delivers an output of 12.5V.

This uninterruptible power supply is suitable for appliances that need 12V - 13V input with a maximum of 2A current. With the configuration described above, this system continued to power my *[Huawei HG8245H5](https://carrier.huawei.com/en/products/fixed-network/access/cpe/h-series-products/hg8245h)* fiber optic router for more than 7 hours.

This power supply unit is an open hardware project. All the schematic, PCB design files, firmware source codes, and compiled binaries are available to download in this repository.

