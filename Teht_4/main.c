#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define STACKSIZE   1024
#define PRIORITY    5
#define MSG_LEN     64
#define MAX_STEPS   32
#define DEFAULT_MS  1000

static const struct gpio_dt_spec red   = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/* UART -> dispatcher FIFO */
K_FIFO_DEFINE(dispatcher_fifo);
struct data_t {
	void *fifo_reserved;
	char msg[MSG_LEN];
};

/* Release-signaali: valotaski -> dispatcher */
K_MUTEX_DEFINE(release_mutex);
K_CONDVAR_DEFINE(release_signal);
static bool released;

/* Yksi valotaski kerrallaan, joten yksi pino riittää */
K_THREAD_STACK_DEFINE(light_stack, STACKSIZE);
static struct k_thread light_thread;

struct step {
	char color;
	int ms;
};

void uart_task(void *, void *, void *);
void dispatcher_task(void *, void *, void *);

K_THREAD_DEFINE(dis_thread, STACKSIZE, dispatcher_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(uart_thread, STACKSIZE, uart_task, NULL, NULL, NULL, PRIORITY, 0, 0);

static void set_color(char color, int on)
{
	if (color == 'R' || color == 'Y') {
		gpio_pin_set_dt(&red, on);
	}
	if (color == 'G' || color == 'Y') {
		gpio_pin_set_dt(&green, on);
	}
}

/* Single-shot valotaski: ei looppia, suorittaa ja päättyy */
static void light_task(void *color_p, void *ms_p, void *unused)
{
	char color = (char)(uintptr_t)color_p;
	int ms = (int)(uintptr_t)ms_p;

	set_color(color, 1);
	k_msleep(ms);
	set_color(color, 0);

	k_mutex_lock(&release_mutex, K_FOREVER);
	released = true;
	k_condvar_broadcast(&release_signal);
	k_mutex_unlock(&release_mutex);
}

static void run_step(const struct step *s)
{
	released = false;
	k_thread_create(&light_thread, light_stack,
			K_THREAD_STACK_SIZEOF(light_stack),
			light_task,
			(void *)(uintptr_t)s->color,
			(void *)(uintptr_t)s->ms, NULL,
			PRIORITY, 0, K_NO_WAIT);

	k_mutex_lock(&release_mutex, K_FOREVER);
	while (!released) {
		k_condvar_wait(&release_signal, &release_mutex, K_FOREVER);
	}
	k_mutex_unlock(&release_mutex);

	k_thread_join(&light_thread, K_FOREVER); /* pino vapaaksi seuraavalle */
}

void dispatcher_task(void *, void *, void *)
{
	struct step hist[MAX_STEPS];
	int n = 0;

	while (true) {
		struct data_t *item = k_fifo_get(&dispatcher_fifo, K_FOREVER);
		char line[MSG_LEN];

		memcpy(line, item->msg, MSG_LEN);
		line[MSG_LEN - 1] = '\0';
		k_free(item);
		printk("Dispatcher: %s\n", line);

		for (int i = 0; line[i] != '\0'; i++) {
			char c = line[i];

			if (c == 'R' || c == 'Y' || c == 'G') {
				int ms = DEFAULT_MS;

				if (line[i + 1] == ',') {
					char *end;

					i += 2;
					ms = strtol(&line[i], &end, 10);
					i = (end - line) - 1;
					if (ms <= 0) {
						ms = DEFAULT_MS;
					}
				}
				struct step s = { .color = c, .ms = ms };

				if (n < MAX_STEPS) {
					hist[n++] = s;
				}
				run_step(&s);

			} else if (c == 'T' && n > 0) {
				/* Toista alusta, kunnes uutta dataa tulee */
				while (k_fifo_is_empty(&dispatcher_fifo)) {
					for (int j = 0; j < n &&
					     k_fifo_is_empty(&dispatcher_fifo); j++) {
						run_step(&hist[j]);
					}
				}
				n = 0; /* uusi sekvenssi alkaa puhtaalta pöydältä */
				break;
			}
		}
	}
}

void uart_task(void *, void *, void *)
{
	char rc = 0;
	char uart_msg[MSG_LEN] = {0};
	int cnt = 0;

	while (true) {
		if (uart_poll_in(uart_dev, &rc) == 0) {
			if (rc != '\r' && rc != '\n') {
				if (cnt < MSG_LEN - 1) {
					uart_msg[cnt++] = rc;
				}
			} else if (cnt > 0) {
				printk("UART msg: %s\n", uart_msg);

				struct data_t *buf = k_malloc(sizeof(struct data_t));

				if (buf != NULL) {
					snprintf(buf->msg, MSG_LEN, "%s", uart_msg);
					k_fifo_put(&dispatcher_fifo, buf);
				} else {
					printk("Out of memory\n");
				}
				cnt = 0;
				memset(uart_msg, 0, MSG_LEN);
			}
		}
		k_msleep(10);
	}
}

int main(void)
{
	if (!gpio_is_ready_dt(&red) || !gpio_is_ready_dt(&green) ||
	    !device_is_ready(uart_dev)) {
		printk("Init failed\n");
		return 1;
	}
	gpio_pin_configure_dt(&red, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&green, GPIO_OUTPUT_INACTIVE);
	return 0;
}