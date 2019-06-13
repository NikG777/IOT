
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gyroscope.h"
#include "board.h"
#include "shell.h"
#include "thread.h"
#include "msg.h"
#include "ringbuffer.h"
#include "data_structure.h"
#include "periph/uart.h"
#include "xtimer.h"
#include "minmea.h"
#include "common.h"
#define doube timer
#define UART_BUFSIZE (500)
#define PRINTER_PRIO        (THREAD_PRIORITY_MAIN - 1)
#define POWEROFF_DELAY      (250U * US_PER_MS)

int DEV_num = 0;

data_t data_zxy;
extern void init_gsm(void);
typedef struct {
    char rx_mem[UART_BUFSIZE];
    ringbuffer_t rx_buf;
} uart_ctx_t;
 uart_ctx_t ctx[2];
void rx_cb(void *arg, uint8_t data);
static kernel_pid_t printer_pid;
static void sleep_test(int num, uart_t uart);

static void *printer(void *arg);

static char printer_stack[THREAD_STACKSIZE_MAIN];
void btn_pressed(void *arg);
int prevtime = 0;
static msg_t msg;

int main(void)
{

    gpio_init(GPIO_PIN(PORT_B,5),GPIO_IN);
    gpio_init_int(GPIO_PIN(PORT_B,5),GPIO_IN,GPIO_FALLING,btn_pressed,NULL); 
	data_zxy.button = 0;
    
	ringbuffer_init(&(ctx[0].rx_buf), ctx[0].rx_mem, UART_BUFSIZE);
   ringbuffer_init(&(ctx[1].rx_buf), ctx[1].rx_mem, UART_BUFSIZE);
    /* initialize UART */
    int res = uart_init(UART_DEV(2), 9600, rx_cb, (void *)&DEV_num);
    if (res == UART_NOBAUD) {
        printf("Error: Given baudrate (%u) not possible\n", (unsigned int)115200);
        return 1;
    }
    else if (res != UART_OK) {
        puts("Error: Unable to initialize UART device\n");
        return 1;
    }
    printf("Success: Successfully initialized UART_DEV(%i)\n", 1);

    /* also test if poweron() and poweroff() work (or at least don't break
     * anything) */
    sleep_test(1, UART_DEV(2));
    printer_pid = thread_create(printer_stack, sizeof(printer_stack),
                                 PRINTER_PRIO, 0, printer, NULL, "printer");
    init_gsm();
    start_gyroscope();
    init_data_thread();
    
}

void rx_cb(void *arg, uint8_t data)
{
    int num = *((int*)arg);
    if(num == 0){printf("%c\n",(char)data);}
        ringbuffer_add_one(&(ctx[num].rx_buf), data);
        if (data == '\n') {
            msg.content.value = num;
    	    msg_send(&msg,printer_pid);
        }
}

static void sleep_test(int num, uart_t uart)
{
    printf("UARD_DEV(%i): test uart_poweron() and uart_poweroff()  ->  ", num);
    uart_poweroff(uart);
    xtimer_usleep(POWEROFF_DELAY);
    uart_poweron(uart);
    puts("[OK]");
}

void btn_pressed(void *arg)
{ 		 	
		if(gpio_read(GPIO_PIN(PORT_B,5)) < 1 && xtimer_now_usec() - prevtime > 1000000)
		{
		  printf("pressed\n");
		data_zxy.button = 1;
		  prevtime = xtimer_now_usec(); 	
		}
		
(void)arg;
		
}


static void *printer(void *arg)
{
    (void)arg;
    msg_t msg;
    msg_t msg_queue[8];
    msg_init_queue(msg_queue, 8);
    char str[UART_BUFSIZE]; 

    while (1) {
        msg_receive(&msg);
        //char str[256];
      //  printf("Success: UART_DEV(%i) RX: ", 0);
      //  printf("z,height: %f %f",data_zxy.z,data_zxy.height);
    	int num = msg.content.value;
    	int size = 0;
        char c = 0;
        do
        {
            c = (int)ringbuffer_get_one(&(ctx[num].rx_buf));
            str[size] = c;
            size++;
        }
        while(c != '\n');
        str[size] = '\0';
       // printf("recieved line: %s", str);
        switch (minmea_sentence_id(str, false)) {
            case MINMEA_SENTENCE_RMC: {
                struct minmea_sentence_rmc frame;
                if (minmea_parse_rmc(&frame, str)) {
                    // printf("$RMC: raw coordinates and speed: (%ld/%ld,%ld/%ld) \n",
                    //         frame.latitude.value, frame.latitude.scale,
                    //         frame.longitude.value, frame.longitude.scale);
                            data_zxy.latitude =  frame.latitude.value;
                            data_zxy.longitude =  frame.longitude.value;
                    // printf("$RMC fixed-point coordinates and speed scaled to three decimal places: (%ld,%ld) \n",
                    //         minmea_rescale(&frame.latitude, 1000),
                    //         minmea_rescale(&frame.longitude, 1000));
                    // printf("$RMC floating point degree coordinates and speed: (%f,%f) %f\n",
                    //         minmea_tocoord(&frame.latitude),
                    //         minmea_tocoord(&frame.longitude),
                    //         minmea_tofloat(&frame.speed));
                } 
            } break;

            case MINMEA_SENTENCE_GGA: {
                struct minmea_sentence_gga frame;
                if (minmea_parse_gga(&frame, str)) {
                    // printf("$GGA: fix quality: %d\n", frame.fix_quality);
                    // printf("Height: %ld\n",frame.altitude.value);
                    float h = frame.height.value * 0.045;
                    data_zxy.height = frame.height.value * 0.045;
                   // printf("Height: %f\n",h);
                    if((h < 150) && (h > 100))
                    {
                        printf("20 floor!");
                    }
                }
            } break;

            case MINMEA_SENTENCE_GSV: {
                struct minmea_sentence_gsv frame;
                if (minmea_parse_gsv(&frame, str)) {
                    // printf("$GSV: message %d of %d\n", frame.msg_nr, frame.total_msgs);
                    // printf("$GSV: sattelites in view: %d\n", frame.total_sats);
                }
            } break;
            default:
            break;
        }
     }

    /* this should never be reached */
    return NULL;
}

    //printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    //printf("This board features a(n) %s MCU.\n", RIOT_MCU);
