
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "gyroscope.h"
#include "common.h"
#include "periph_conf.h"
#include "periph/i2c.h"
#include "shell.h"
#include "xtimer.h"
// Ускорения 
#define RANGE_2G        2
#define RANGE_4G        4
#define RANGE_8G        8
#define G               9.8

// Переменные регистров
#define CTRL_REG1       0x20
#define CTRL_REG2       0x21
#define CTRL_REG3       0x22
#define CTRL_REG4       0x23
#define CTRL_REG5       0x24
#define WHO_am_I        0xF
#define DEV             0

// Переменные адресов
#define ADR_FS_2        0x00
#define ADR_FS_4        0x10 
#define ADR_FS_8        0x30

// Переменные вывода

#define OUT_X           0x28
#define OUT_Y           0x2A
#define OUT_Z           0x2C
#define I2C_NOSTOP      0x04 
#define I2C_REG16       0x02
#define _addr           0b0011000

static char printer_stack[THREAD_STACKSIZE_MAIN];
uint8_t readByte(uint8_t reg);
static void *printer(void *arg);

// Переменные адресов
  //  uint8_t _addr;
    uint8_t _ctrlReg1;
    uint8_t _ctrlReg2;
    uint8_t _ctrlReg3;
    uint8_t _ctrlReg4;
    uint8_t _ctrlReg5;
    float _mult;
 extern data_t data_zxy;

int16_t readAxis(uint8_t reg) {
    return ((int16_t)readByte(reg+1) << 8) | readByte(reg);
}

int16_t readX(void) {
    return readAxis(OUT_X);
}

int16_t readY(void) {
    return readAxis(OUT_Y);
}

int16_t readZ(void) {
    return readAxis(OUT_Z);
}

uint8_t readByte(uint8_t reg) {
    uint8_t value;
    //printf("\naddr: %d",reg);
    i2c_write_byte(DEV,_addr, reg, I2C_REG16);
    i2c_read_byte(DEV,_addr, &value, I2C_REG16);
      // printf("\n%d",value);
    return value;
}

static void writeCtrlReg1(void){
    //  uint8_t value;
    //printf("%d",_ctrlReg1);
    i2c_write_reg (DEV,_addr, CTRL_REG1, _ctrlReg1,I2C_REG16);
   // i2c_read_reg(DEV,_addr,CTRL_REG1,&value,I2C_REG16);
   // printf("this is a value,%d",value);
    //printf("zapis\n");
}



static void writeCtrlReg4(void){
   i2c_write_reg (DEV,_addr, CTRL_REG4, _ctrlReg4,I2C_REG16);
}

static void setRange(uint8_t range) {
    switch (range) {
        case RANGE_2G: {
            _ctrlReg4 = ADR_FS_2;
            _mult = RANGE_2G / 32767.0;
            break;
        }
        case RANGE_4G: {
            _ctrlReg4 = ADR_FS_4;
            _mult = RANGE_4G / 32767.0;
            break;
        }
        case RANGE_8G: {
            _ctrlReg4 = ADR_FS_8;
            _mult = RANGE_8G / 32767.0;
            break;
        }
        default: {
            _mult = RANGE_2G / 32767.0;    
        }
        break;
    }
    writeCtrlReg4();
}

float readGX(void) {
    return readX()*_mult;
}

float readGY(void) {
    return readY()*_mult;
}

float readGZ(void) {
    return readZ()*_mult;
}

float readAX(void) {
    return readGX() * G;
}

float readAY(void) {
    return readGY() * G;
}

float readAZ(void) {
    return readGZ() * G;
}
static void begin(void) {
    // подключаемся к шине I²C
    i2c_init(DEV);
    i2c_acquire(DEV);
    // включаем координаты x, y, z
    _ctrlReg1 |= (1 << 0);
    _ctrlReg1 |= (1 << 1);
    _ctrlReg1 |= (1 << 2);
    // включаем аксселерометр
    _ctrlReg1 |= (1 << 5);
    // устанавливаем максимальное измеряемое ускорение в G
    setRange(RANGE_2G);
    writeCtrlReg1();
}

int start_gyroscope(void)
{
    begin();
    thread_create(printer_stack, sizeof(printer_stack),
                                 THREAD_PRIORITY_MAIN - 1, 0, printer, NULL, "printer");
    return(0);
}

static void *printer(void *arg)
{
    (void)arg;
    xtimer_ticks32_t last_wakeup  = xtimer_now();
    while(1){
    //float val = readGZ();
    // printf("////////////////////////\n");
    // printf("Z = %f\n",val);
    // printf("X = %f\n",readGX());
    // printf("Y = %f\n",readGY());
    data_zxy.z = readGZ();
    data_zxy.y = readGY();
    data_zxy.x  = readGX();
    xtimer_periodic_wakeup(&last_wakeup,100000); 
   // printf("////////////////////////\n");
    }
    return NULL;
}

// static void writeCtrlReg2(void){
//     i2c_write_reg (DEV,_addr, CTRL_REG2, _ctrlReg2,I2C_REG16);
// }

// static void writeCtrlReg3(void){
//     i2c_write_reg (DEV,_addr, CTRL_REG3, _ctrlReg3,I2C_REG16);
// }
// static void writeCtrlReg5(void){
//     i2c_write_reg (DEV,_addr, CTRL_REG5, _ctrlReg5, I2C_REG16);
// }
// void readXYZ(int16_t *x, int16_t *y, int16_t *z) {
//     i2c_write_reg (DEV,_addr,OUT_X | (1 << 7),I2C_REG16);
//     uint8_t burstSize = 6;
//     uint8_t values[burstSize];
//     i2c_read_bytes(DEV,_addr,&values,burstsize,I2C_REG16);
//     *x = *((int16_t*)&values[0]);
//     *y = *((int16_t*)&values[2]);
//     *z = *((int16_t*)&values[4]);
// }
// void readGXYZ(float *gx, float *gy, float *gz) {
//     int16_t x, y, z;
//     readXYZ(&x, &y, &z);
//     *gx = x * _mult;
//     *gy = y * _mult;
//     *gz = z * _mult;
// }

// void readAXYZ(float *ax, float *ay, float *az) {
//     readGXYZ(ax, ay, az);
//     (*ax) *= G;
//     (*ay) *= G;
//     (*az) *= G;
// }
