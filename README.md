# WIP: The scale for the https://fellowproducts.com/products/opus-coffee-grinder

The .pio\libdeps\seeed_xiao_esp32s3\HX711_ADC\src\config.h should be modified for the faster feedback:
    - #define SAMPLES 0
    - #define IGN_HIGH_SAMPLE 0
    - #define IGN_LOW_SAMPLE 0

Wiring:
- Voltage reading: https://forum.seeedstudio.com/t/battery-voltage-monitor-and-ad-conversion-for-xiao-esp32c/267535
