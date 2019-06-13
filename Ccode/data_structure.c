
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "gyroscope.h"
#include "common.h"
#include "data_structure.h"
#include "periph_conf.h"
#include "periph/i2c.h"
#include "shell.h"
#include "xtimer.h"
extern data_t data_zxy;
char resultstr[256];
static char printer_stack[THREAD_STACKSIZE_MAIN];
static void *printer(void *arg);

void init_data_thread(void)
{
    thread_create(printer_stack, sizeof(printer_stack),
                                 THREAD_PRIORITY_MAIN - 1, 0, printer, NULL, "printer");

}

static void *printer(void *arg)
{
    (void)arg;
    while(1){
    printf("////////////////////////\n");
    printf("Z = %f\n",data_zxy.z);
    printf("X = %f\n",data_zxy.x);
    printf("Y = %f\n",data_zxy.y);
    printf("Height= %f\n",data_zxy.height);
    printf("////////////////////////\n");
    xtimer_sleep(2);
    sprintf(resultstr, "{\"coordX\":%f,\"coordY\":%f,\"coordZ\":%f,\"height\":%f,\"longitude\":%f,\"latitude\":%f,\"button\":%d}", data_zxy.x, data_zxy.y,data_zxy.z,data_zxy.height,data_zxy.longitude,data_zxy.latitude,data_zxy.button);
    puts(resultstr);
    }
    return NULL;
    
}
