//
//Tavoittelen 3 pistettä tein kaikki tehtävänannossa pyydetyt asiat.
//
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

static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

#define STACKSIZE 500
#define PRIORITY 5

volatile int YELLOW_SEQ = 0;
volatile int LED_RED = 0;
volatile int LED_YELLOW = 0;
volatile int LED_GREEN = 0;
volatile int LED_STATE = 0;
volatile int OLD_LED_STATE = 0;

int init_button();
int init_led();
void red_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);
void button_0_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void button_1_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void button_2_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void button_3_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void button_4_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
void pause_task(void *, void*, void*);

K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(pause_thread,STACKSIZE,pause_task,NULL,NULL,NULL,PRIORITY,0,0);

int main(void)
{
    init_led();
    init_button();
    return 0;
}


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

int init_button() {
    
    if (!gpio_is_ready_dt(&button_0) || !gpio_is_ready_dt(&button_1) || 
        !gpio_is_ready_dt(&button_2) || !gpio_is_ready_dt(&button_3) || 
        !gpio_is_ready_dt(&button_4)) {
        printk("Error: One or more buttons are not ready\n");
        return -1;
    }

    gpio_pin_configure_dt(&button_0, GPIO_INPUT);
    gpio_pin_configure_dt(&button_1, GPIO_INPUT);
    gpio_pin_configure_dt(&button_2, GPIO_INPUT);
    gpio_pin_configure_dt(&button_3, GPIO_INPUT);
    gpio_pin_configure_dt(&button_4, GPIO_INPUT);

    gpio_pin_interrupt_configure_dt(&button_0, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_pin_interrupt_configure_dt(&button_1, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_pin_interrupt_configure_dt(&button_2, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_pin_interrupt_configure_dt(&button_3, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_pin_interrupt_configure_dt(&button_4, GPIO_INT_EDGE_TO_ACTIVE);

    gpio_init_callback(&button_0_data, button_0_handler, BIT(button_0.pin));
    gpio_add_callback(button_0.port, &button_0_data);

    gpio_init_callback(&button_1_data, button_1_handler, BIT(button_1.pin));
    gpio_add_callback(button_1.port, &button_1_data);

    gpio_init_callback(&button_2_data, button_2_handler, BIT(button_2.pin));
    gpio_add_callback(button_2.port, &button_2_data);

    gpio_init_callback(&button_3_data, button_3_handler, BIT(button_3.pin));
    gpio_add_callback(button_3.port, &button_3_data);

    gpio_init_callback(&button_4_data, button_4_handler, BIT(button_4.pin));
    gpio_add_callback(button_4.port, &button_4_data);

    printk("Set up all buttons ok\n");
    return 0;
}

void red_led_task(void *, void *, void*) {
    printk("Red led thread started\n");
    while (true) {
        if(LED_STATE == 0){
            gpio_pin_set_dt(&red,1);
            printk("Red on\n");
            k_sleep(K_SECONDS(1));
            gpio_pin_set_dt(&red,0);
            printk("Red off\n");
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
        if(LED_STATE == 1 || YELLOW_SEQ == 1){
            gpio_pin_set_dt(&green,1);
            gpio_pin_set_dt(&red,1);
            printk("Yellow on\n");
            k_sleep(K_SECONDS(1));
            gpio_pin_set_dt(&green,0);
            gpio_pin_set_dt(&red,0);
            printk("Yellow off\n");
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
            gpio_pin_set_dt(&green,1);
            printk("Green on\n");
            k_sleep(K_SECONDS(1));
            gpio_pin_set_dt(&green,0);
            printk("Green off\n");
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

void pause_task(void *, void *, void*){
    while(true){
        if(LED_STATE == 4){
			if(YELLOW_SEQ == 0){
				if(LED_YELLOW == 1){
                gpio_pin_set_dt(&red, 1);
                gpio_pin_set_dt(&green, 1);
            } else {
                if (LED_RED != 0) {
    				gpio_pin_set_dt(&red, 1);
				} else {
    				gpio_pin_set_dt(&red, 0);
				}
                if (LED_GREEN != 0) {
    				gpio_pin_set_dt(&green, 1);
				} else {
    				gpio_pin_set_dt(&green, 0);
				}
            }
        }
        k_msleep(10);
    	}
	}
}

void button_0_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins){
    if (LED_STATE == 4) {
        LED_STATE = OLD_LED_STATE;
        printk("Pause ended, resuming state %d\n", LED_STATE);
        
        LED_RED = 0;
        LED_GREEN = 0;
        LED_YELLOW = 0;
        gpio_pin_set_dt(&red, 0);
        gpio_pin_set_dt(&green, 0);
    } else {
        OLD_LED_STATE = LED_STATE;
        LED_STATE = 4;
        printk("Button pressed, entering pause\n");
    }
}

void button_1_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins){
    if(LED_STATE == 4){
        LED_RED = !LED_RED;
    }
}

void button_2_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins){
    if(LED_STATE == 4){
        LED_GREEN = !LED_GREEN;
    }
}

void button_3_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins){
    if(LED_STATE == 4){
        LED_YELLOW = !LED_YELLOW;
    }
}

void button_4_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins){
    if(LED_STATE == 4){
        YELLOW_SEQ = !YELLOW_SEQ;
    }
}