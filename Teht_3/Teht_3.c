#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>


#define STACKSIZE 500
#define PRIORITY 5

// Condition Variables
K_MUTEX_DEFINE(red_mutex);
K_CONDVAR_DEFINE(red_signal);
K_MUTEX_DEFINE(release_mutex);
K_CONDVAR_DEFINE(release_signal);
K_MUTEX_DEFINE(yellow_mutex);
K_CONDVAR_DEFINE(yellow_signal);
K_MUTEX_DEFINE(green_mutex);
K_CONDVAR_DEFINE(green_signal);

static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

void red_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);
void dispatcher_task(void *, void *, void*);
void uart_task(void *, void *, void*);


K_THREAD_DEFINE(dis_thread,STACKSIZE,dispatcher_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(uart_thread,STACKSIZE,uart_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task,NULL,NULL,NULL,PRIORITY,0,0);

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

// Create dispatcher FIFO buffer
K_FIFO_DEFINE(dispatcher_fifo);

// FIFO dispatcher data type
struct data_t {
	void *fifo_reserved;
	char msg[20];
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
	// UART initialization
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
	// Received character from UART
	char rc=0;
	char uart_msg[20];
	memset(uart_msg,0,20);
	int uart_msg_cnt = 0;

	while (true) {
		// Ask UART if data available
		if (uart_poll_in(uart_dev,&rc) == 0) {
			// printk("Received: %c\n",rc);
			// If character is not newline, add to UART message buffer
			if (rc != '\r') {
				uart_msg[uart_msg_cnt] = rc;
				uart_msg_cnt++;
			// Character is newline, copy dispatcher data and put to FIFO buffer
			} else {
				printk("UART msg: %s\n", uart_msg);
                
				struct data_t *buf = k_malloc(sizeof(struct data_t));
				if (buf == NULL) {
					return;
				}
                                snprintf(buf->msg,20,"%s", uart_msg);

                                k_fifo_put(&dispatcher_fifo, buf);
				// Clear UART message buffer
				uart_msg_cnt = 0;
				memset(uart_msg,0,20);
			}
		}
		k_msleep(10);
	}
}


void dispatcher_task(void *, void *, void *)
{
    while (true) {
        struct data_t *rec_item = k_fifo_get(&dispatcher_fifo, K_FOREVER);
        char sequence[20];
        memcpy(sequence, rec_item->msg, sizeof(sequence));
        k_free(rec_item);

        printk("Dispatcher: %s\n", sequence);

        for (int cnt = 0; sequence[cnt] != 0; cnt++) {
            switch (sequence[cnt]) {
            case 'R':
                k_mutex_lock(&red_mutex, K_FOREVER);
                k_condvar_broadcast(&red_signal);
                k_mutex_unlock(&red_mutex);
                break;
            case 'Y':
                k_mutex_lock(&yellow_mutex, K_FOREVER);
                k_condvar_broadcast(&yellow_signal);
                k_mutex_unlock(&yellow_mutex);
                break;
            case 'G':
                k_mutex_lock(&green_mutex, K_FOREVER);
                k_condvar_broadcast(&green_signal);
                k_mutex_unlock(&green_mutex);
                break;
            default:
                continue;
            }

            k_mutex_lock(&release_mutex, K_FOREVER);
            k_condvar_wait(&release_signal, &release_mutex, K_FOREVER);
            k_mutex_unlock(&release_mutex);
        }
    }
}

//Ledi taskit
void red_led_task(void *, void *, void *) {
    printk("Red led thread started\n");
    while (true) {
        k_mutex_lock(&red_mutex, K_FOREVER);
        k_condvar_wait(&red_signal, &red_mutex, K_FOREVER);
        k_mutex_unlock(&red_mutex);

        gpio_pin_set_dt(&red, 1);
        k_sleep(K_SECONDS(1));
        gpio_pin_set_dt(&red, 0);

        k_mutex_lock(&release_mutex, K_FOREVER);
        k_condvar_broadcast(&release_signal);
        k_mutex_unlock(&release_mutex);
    }
}

void yellow_led_task(void *, void *, void *) {
    printk("Yellow led thread started\n");
    while (true) {
        k_mutex_lock(&yellow_mutex, K_FOREVER);
        k_condvar_wait(&yellow_signal, &yellow_mutex, K_FOREVER);
        k_mutex_unlock(&yellow_mutex);

        gpio_pin_set_dt(&green, 1);
        gpio_pin_set_dt(&red, 1);
        k_sleep(K_SECONDS(1));
        gpio_pin_set_dt(&green, 0);
        gpio_pin_set_dt(&red, 0);

        k_mutex_lock(&release_mutex, K_FOREVER);
        k_condvar_broadcast(&release_signal);
        k_mutex_unlock(&release_mutex);
    }
}

void green_led_task(void *, void *, void *) {
    printk("Green led thread started\n");
    while (true) {
        k_mutex_lock(&green_mutex, K_FOREVER);
        k_condvar_wait(&green_signal, &green_mutex, K_FOREVER);
        k_mutex_unlock(&green_mutex);

        gpio_pin_set_dt(&green, 1);
        k_sleep(K_SECONDS(1));
        gpio_pin_set_dt(&green, 0);

        k_mutex_lock(&release_mutex, K_FOREVER);
        k_condvar_broadcast(&release_signal);
        k_mutex_unlock(&release_mutex);
    }
}

