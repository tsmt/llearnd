#ifndef _SHT21_H
#define _SHT21_H

#define SHT21_ADDR 0x40
#define SHT21_SOFTRESET 0xFE
#define SHT21_STATUS_BITMASK 0xFFFC
#define SHT21_TEM 0xF3
#define SHT21_HUM 0xF5
#define SHT21_TEM_WAIT 86
#define SHT21_HUM_WAIT 30

int sht21Setup();
float sht21GetTemp();
float sht21GetHum();

#endif
