#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include "fsm.h"

#define PERIOD_TICK 100/portTICK_RATE_MS

portTickType REBOUND_TICK = 100;
static portTickType time_ini = 0;
static portTickType time_end = 0;
long TIMEOUT = 600; // 1 minuto

#define GPIO_BUTTON 0 //D3
#define GPIO_BUTTON_LED 2 //LED

uint32 user_rf_cal_sector_set(void){
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;
    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

enum fsm_state {
    LED_ON,
    LED_OFF
};

int button_pressed(fsm_t *this){
  if(!GPIO_INPUT_GET(GPIO_BUTTON)){
    static portTickType xLastISRTick0 = 0;
    portTickType now = xTaskGetTickCount ();
    if (now > xLastISRTick0) {
      xLastISRTick0 = now + REBOUND_TICK;
        printf("%s\n","presion" );
      return 1;
    }
    else{ return 0; }
  }
  else{ return 0; }
};

void led_on (fsm_t *this) {
  printf("%s\n", "ON" );
  GPIO_OUTPUT_SET(GPIO_BUTTON_LED, 0);
   time_ini = xTaskGetTickCount ();
};

void led_off (fsm_t *this) {
  printf("%s\n", "OFF" );
  GPIO_OUTPUT_SET(GPIO_BUTTON_LED, 1);
};

int  timeout (fsm_t *this){
  time_end = xTaskGetTickCount ();
  if (((time_end - time_ini)/portTICK_RATE_MS)>TIMEOUT){
    return 1;
  }
  else{ return 0; }
}

static fsm_trans_t interruptor[] = {
  {LED_OFF, button_pressed, LED_ON, led_on},
  {LED_ON, timeout, LED_OFF, led_off},
  {-1, NULL, -1, NULL}
};

void inter(void* ignore){
  fsm_t* fsm = fsm_new(interruptor);
  led_off(fsm);
  portTickType xLastWakeTime;

  while(true) {
    xLastWakeTime = xTaskGetTickCount ();
    fsm_fire(fsm);
    vTaskDelayUntil(&xLastWakeTime, PERIOD_TICK);
  }
}

void user_init(void){
    xTaskCreate(&inter, "startup", 2048, NULL, 1, NULL);
}
