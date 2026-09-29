//Yritän saada 4 pistettä. Kaikki tehtävä 3 pyydettyt asiat ovat tehty.

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define STACKSIZE 1024
#define PRIORITY 5
#define MSG_LEN 64
#define DEFAULT_MS 1000

K_MUTEX_DEFINE(release_mutex);
K_CONDVAR_DEFINE(release_signal);

// LISÄTTY: lippu, jotta signaali ei voi kadota (condvar ei muista signaalia)
static bool released;

static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

void red_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);
void dispatcher_task(void *, void *, void*);
void uart_task(void *, void *, void*);

K_THREAD_DEFINE(dis_thread,STACKSIZE,dispatcher_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(uart_thread,STACKSIZE,uart_task,NULL,NULL,NULL,PRIORITY,0,0);

K_THREAD_STACK_DEFINE(red_stack, STACKSIZE);
K_THREAD_STACK_DEFINE(yellow_stack, STACKSIZE);
K_THREAD_STACK_DEFINE(green_stack, STACKSIZE);
static struct k_thread red_thread, yellow_thread, green_thread;

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

// Create dispatcher FIFO buffer
K_FIFO_DEFINE(dispatcher_fifo);

struct data_t {
	void *fifo_reserved;
	char msg[MSG_LEN];
};

struct led_data_t {
	void *fifo_reserved;
	int ms;
};
K_FIFO_DEFINE(red_fifo);
K_FIFO_DEFINE(yellow_fifo);
K_FIFO_DEFINE(green_fifo);


struct led_ctrl {
	struct k_thread *thread;
	k_thread_stack_t *stack;
	size_t stack_size;
	k_thread_entry_t entry;
	struct k_fifo *fifo;
};

static const struct led_ctrl leds[] = {
	{ &red_thread,    red_stack,    K_THREAD_STACK_SIZEOF(red_stack),    red_led_task,    &red_fifo    },
	{ &yellow_thread, yellow_stack, K_THREAD_STACK_SIZEOF(yellow_stack), yellow_led_task, &yellow_fifo },
	{ &green_thread,  green_stack,  K_THREAD_STACK_SIZEOF(green_stack),  green_led_task,  &green_fifo  },
};

int init_led() {
    int ret;
    ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error: Led configure failed\n");        
        return ret;
    }

    ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error: Led configure failed\n");        
        return ret;
    }
    gpio_pin_set_dt(&red,0);
    gpio_pin_set_dt(&green,0);

    printk("Led initialized ok\n");
    return 0;
}

int init_uart(void) {
	if (!device_is_ready(uart_dev)) {
		return 1;
	} 
	return 0;
}

int main(void)
{
        init_led();
	int ret = init_uart();
	if (ret != 0) {
		printk("UART initialization failed!\n");
		return ret;
	}

	return 0;
}

void uart_task(void *, void *, void *)
{
	char rc=0;
	char uart_msg[MSG_LEN];
	memset(uart_msg,0,sizeof(uart_msg));
	int uart_msg_cnt = 0;

	while (true) {
		if (uart_poll_in(uart_dev,&rc) == 0) {
			if (rc != '\r' && rc != '\n') {
				if (uart_msg_cnt < sizeof(uart_msg) - 1) {
					uart_msg[uart_msg_cnt] = rc;
					uart_msg_cnt++;
				}
			} else if (uart_msg_cnt > 0) {
				printk("UART msg: %s\n", uart_msg);
                
				struct data_t *buf = k_malloc(sizeof(struct data_t));
				if (buf != NULL) {
					snprintf(buf->msg, sizeof(buf->msg), "%s", uart_msg);
					k_fifo_put(&dispatcher_fifo, buf);
				} else {
					printk("Out of memory\n");
				}
				uart_msg_cnt = 0;
				memset(uart_msg,0,sizeof(uart_msg));
			}
		}
		k_msleep(10);
	}
}

static void send_release(void)
{
	k_mutex_lock(&release_mutex, K_FOREVER);
	released = true;
	k_condvar_broadcast(&release_signal);
	k_mutex_unlock(&release_mutex);
}


static void turn_led_on(int idx, int ms)
{
	const struct led_ctrl *l = &leds[idx];

	struct led_data_t d = { .ms = ms };

	k_fifo_put(l->fifo, &d);

	released = false;

	k_thread_create(l->thread, l->stack, l->stack_size, l->entry,
			NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);

	k_mutex_lock(&release_mutex, K_FOREVER);
	while (!released) {
		k_condvar_wait(&release_signal, &release_mutex, K_FOREVER);
	}
	k_mutex_unlock(&release_mutex);

	k_thread_join(l->thread, K_FOREVER);
}

void dispatcher_task(void *, void *, void *)
{
    while (true) {
        struct data_t *rec_item = k_fifo_get(&dispatcher_fifo, K_FOREVER);
        char sequence[MSG_LEN];
        memcpy(sequence, rec_item->msg, sizeof(sequence));

        sequence[MSG_LEN - 1] = '\0';
        k_free(rec_item);

        printk("Dispatcher: %s\n", sequence);

        for (int cnt = 0; sequence[cnt] != 0; cnt++) {
            int idx;
            switch (sequence[cnt]) {
            case 'R': idx = 0; break;
            case 'Y': idx = 1; break;
            case 'G': idx = 2; break;
            default:
                continue;
            }

            int ms = DEFAULT_MS;
            if (sequence[cnt + 1] == ',') {
                char *end;
                cnt += 2;                            
                ms = strtol(&sequence[cnt], &end, 10);
                cnt = (end - sequence) - 1;         
                if (ms <= 0) {
                    ms = DEFAULT_MS;
                }
            }

            turn_led_on(idx, ms);
        }
    }
}


void red_led_task(void *, void *, void *) {

    struct led_data_t *d = k_fifo_get(&red_fifo, K_FOREVER);
    int ms = d->ms;                

    gpio_pin_set_dt(&red, 1);
    k_msleep(ms);                   
    gpio_pin_set_dt(&red, 0);

    send_release();
}

void yellow_led_task(void *, void *, void *) {
    struct led_data_t *d = k_fifo_get(&yellow_fifo, K_FOREVER);
    int ms = d->ms;

    gpio_pin_set_dt(&green, 1);
    gpio_pin_set_dt(&red, 1);
    k_msleep(ms);
    gpio_pin_set_dt(&green, 0);
    gpio_pin_set_dt(&red, 0);

    send_release();
}

void green_led_task(void *, void *, void *) {
    struct led_data_t *d = k_fifo_get(&green_fifo, K_FOREVER);
    int ms = d->ms;

    gpio_pin_set_dt(&green, 1);
    k_msleep(ms);
    gpio_pin_set_dt(&green, 0);

    send_release();
}