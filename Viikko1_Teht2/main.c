#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <inttypes.h>
#include <zephyr/sys/util.h>


#define BUTTON_0 DT_ALIAS(sw0)
#define BUTTON_1 DT_ALIAS(sw1)
#define BUTTON_2 DT_ALIAS(sw2)
#define BUTTON_3 DT_ALIAS(sw3)
#define BUTTON_4 DT_ALIAS(sw4)

static const struct gpio_dt_spec button_0 = GPIO_DT_SPEC_GET_OR(BUTTON_0, gpios, {0});
static const struct gpio_dt_spec button_1 = GPIO_DT_SPEC_GET_OR(BUTTON_1, gpios, {0});
static const struct gpio_dt_spec button_2 = GPIO_DT_SPEC_GET_OR(BUTTON_2, gpios, {0});
static const struct gpio_dt_spec button_3 = GPIO_DT_SPEC_GET_OR(BUTTON_3, gpios, {0});
static const struct gpio_dt_spec button_4 = GPIO_DT_SPEC_GET_OR(BUTTON_4, gpios, {0});

static struct gpio_callback button_0_data;
static struct gpio_callback button_1_data;
static struct gpio_callback button_2_data;
static struct gpio_callback button_3_data;
static struct gpio_callback button_4_data;

// Led pin configurations
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

// Red led thread initialization
#define STACKSIZE 500
#define PRIORITY 5

volatile int LED_STATE = 0;
volatile int OLD_LED_STATE = 0;

int init_button();
int init_led();
void red_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);
void button_0_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void pause_task(void *, void*, void*);
K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(pause_thread,STACKSIZE,pause_task,NULL,NULL,NULL,PRIORITY,0,0);

// Main program
int main(void)
{
	init_led();
	init_button();
	return 0;
}

// Initialize leds
int  init_led() {

	// Led pin initialization
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
	// set led off
	gpio_pin_set_dt(&red,0);
	gpio_pin_set_dt(&green,0);

	printk("Led initialized ok\n");
	
	return 0;
}

int init_button() {

	int ret;
	if (!gpio_is_ready_dt(&button_0)) {
		printk("Error: button 0 is not ready\n");
		return -1;
	}
	if (!gpio_is_ready_dt(&button_1)) {
		printk("Error: button 0 is not ready\n");
		return -1;
	}

	if (!gpio_is_ready_dt(&button_2)) {
		printk("Error: button 0 is not ready\n");
		return -1;
	}
	if (!gpio_is_ready_dt(&button_3)) {
		printk("Error: button 0 is not ready\n");
		return -1;
	}
	if (!gpio_is_ready_dt(&button_0)) {
		printk("Error: button 0 is not ready\n");
		return -1;
	}


	ret = gpio_pin_configure_dt(&button_0, GPIO_INPUT);
	if (ret != 0) {
		printk("Error: failed to configure pin\n");
		return -1;
	}

	ret = gpio_pin_interrupt_configure_dt(&button_0, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error: failed to configure interrupt on pin\n");
		return -1;
	}

	gpio_init_callback(&button_0_data, button_0_handler, BIT(button_0.pin));
	gpio_add_callback(button_0.port, &button_0_data);
	printk("Set up button 0 ok\n");
	
	return 0;
}

// Task to handle red led
void red_led_task(void *, void *, void*) {
	printk("Red led thread started\n");
	while (true) {
		if(LED_STATE == 0){
			// 1. set led on 
			gpio_pin_set_dt(&red,1);
			printk("Red on\n");
			// 2. sleep for 2 seconds
			k_sleep(K_SECONDS(1));
			// 3. set led off
			gpio_pin_set_dt(&red,0);
			printk("Red off\n");
			// 4. sleep for 2 seconds
			k_msleep(1000);
			if(LED_STATE == 0){
				LED_STATE = 1;
			}
			
		}
		else{
			k_msleep(10);
		}
	}
}

void yellow_led_task(void *, void *, void*) {
	
	printk("Yellow led thread started\n");
	while (true) {
		if(LED_STATE == 1){
			// 1. set led on 
			gpio_pin_set_dt(&green,1);
			gpio_pin_set_dt(&red,1);
			printk("Yellow on\n");
			// 2. sleep for 2 seconds
			k_sleep(K_SECONDS(1));
			// 3. set led off
			gpio_pin_set_dt(&green,0);
			gpio_pin_set_dt(&red,0);
			printk("Yellow off\n");
			// 4. sleep for 2 seconds
			k_msleep(1000);
			if(LED_STATE == 1){
				LED_STATE = 2;
			}
			
		}
		else{
			k_msleep(10);
		}
	}
}

void green_led_task(void *, void *, void*) {
	
	printk("Green led thread started\n");
	while (true) {
		if(LED_STATE == 2){
			// 1. set led on 
			gpio_pin_set_dt(&green,1);
			printk("Green on\n");
			// 2. sleep for 2 seconds
			k_sleep(K_SECONDS(1));
			// 3. set led off
			gpio_pin_set_dt(&green,0);
			printk("Green off\n");
			// 4. sleep for 2 seconds
			k_msleep(1000);
			if(LED_STATE == 2){
				LED_STATE = 0;
			}
		}
		else{
			k_msleep(10);
		}
	}
}

void button_0_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins){
    // Keskeytyksessä vaihdetaan suoraan normaalitilan ja pause-tilan välillä
    if (LED_STATE == 4) {
        // Palautetaan tallennettu tila
        LED_STATE = OLD_LED_STATE;
        printk("Pause ended, resuming state %d\n", LED_STATE);
    } else {
        // Tallennetaan nykyinen tila ja siirrytään pause-tilaan
        OLD_LED_STATE = LED_STATE;
        LED_STATE = 4;
        printk("Button pressed, entering pause\n");
    }
}

void pause_task(void *p1, void *p2, void *p3){
    while(true){
        if(LED_STATE == 4){
            printk("Pause task active. Waiting for button press...\n");
            
            // Ohjelma pysyy tässä sisemmässä silmukassa (pause_taskissa) 
            // tasan niin kauan kunnes nappia painetaan uudestaan
            while(LED_STATE == 4){
                k_msleep(50);
            }
        }
        else{
            k_msleep(50);
        }
    }
}