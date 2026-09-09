### Documentation des composants

##### Micro
component : INMP441
pin connection : 
| pin micro | pin esp32 |
| --------- | --------- |
| VDD       | 3.3V      |
| GND       | GND       |
| L/R       | GND       |
| SCK       | GPIO 41   |
| WS        | GPIO 42   |
| SD        | GPIO 40   |

##### Haut parleur et amplifier I2S
component :
pin connection :
| pin     | pin esp32 |
| ------- | --------- |
| VIN/VCC | 5V/VIN    |
| GND     | GND       |
| LRC     | GPIO 16   |
| BCLK    | GPIO 15   |
| DIN     | GPIO 17   |
| GAIN    | none      |
| SD      | none      |

##### TFT 3.5" tactile Screen
component : 
pin connection :
| pin ecran | pin esp32     |
| --------- | ------------- |
| VCC       | 3.3V ou 5V    |
| GND       | GND           |
| CS        | GPIO 10       |
| RESET     | GPIO 14       |
| DC/RS     | GPIO 9        |
| SDI/MOSI  | GPIO 11       |
| SCK       | GPIO 12       |
| LED       | 3.3V          |
| SDO/MISO  | GPIO 13       |
| T_CLK     | GPIO 12 (SCK) |
| T_CS      | GPIO 21       |
| T_DIN     | GPIO 11 (SDI) |
| T_DO      | GPIO 13 (SDO) |
| T_IRQ     | none          |
