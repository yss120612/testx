/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include <driver/gpio.h>
#include "GlobalSettings.h"
#include "Settings.h"
#include "MotorTask.h"
#include "EndpointTask.h"
#include "RMTTask.h"
#include "WiFiTask.h"
#include "HTTPServer.h"
#include "SpeakerTask.h"
#include "ButtonTask.h"
#include "RX433Task.h"
#include "TX433Task.h"
#include "ENS160Task.h"
#include "DISPTask.h"
#include <esp_log.h>
#include "driver/rmt_tx.h"


//esp32-c6 pins (SCREEN)
// #define GPIO_MOTOR_UP GPIO_NUM_11                                                                                                                    
// #define GPIO_MOTOR_DOWN GPIO_NUM_10

// #define GPIO_IR GPIO_NUM_1

// #define GPIO_SPEAKER GPIO_NUM_2

// #define GPIO_ENPIONT_SUPPLY GPIO_NUM_5
// #define GPIO_ENPIONT_DATA GPIO_NUM_4

// #define GPIO_RX433 GPIO_NUM_9

// #define GPIO_BUTTON GPIO_NUM_22

//esp32-c6 pins (STAND)
#define GPIO_MOTOR_UP GPIO_NUM_11                                                                                                                    
#define GPIO_MOTOR_DOWN GPIO_NUM_10

#define GPIO_IR GPIO_NUM_1

#define GPIO_SPEAKER GPIO_NUM_2

//#define GPIO_ENPIONT_SUPPLY GPIO_NUM_5
//#define GPIO_ENPIONT_DATA GPIO_NUM_4

#define GPIO_RX433 GPIO_NUM_9
#define GPIO_TX433 GPIO_NUM_4

#define GPIO_BUTTON GPIO_NUM_14                                                                                                                               

#define GPIO_SCL GPIO_NUM_6
#define GPIO_SDA GPIO_NUM_7      

//ESP32 pins
// #define GPIO_MOTOR_UP GPIO_NUM_33                                                                                                                      
// #define GPIO_MOTOR_DOWN GPIO_NUM_32

// #define GPIO_IR GPIO_NUM_4

// #define GPIO_SPEAKER GPIO_NUM_16

// #define GPIO_ENPIONT_SUPPLY GPIO_NUM_26
// #define GPIO_ENPIONT_DATA GPIO_NUM_27

QueueHandle_t queue;
State state;
EventGroupHandle_t flags;
EndpointTask * endpoint;
RMTTask *ir;
MotorTask *motor;
WiFiTask *wifi;
HTTPServer * http;
SpeakerTask *speaker;
ButtonTask *button;
RX433Task * rx433;
TX433Task * tx433;
ENS160Task * ens160;
Display * display;

i2c_master_bus_handle_t bus_handle;
bool repair_mode;
TickType_t ticks;

extern "C" {
    void app_main(void);
}

void endpoint_event(yss_event_t event){
    notify_t notify;
    switch (event.button)
    {
    case 0:
        
        break;
    case ENDPOINT_TRIPPED:
        notify.title=MOTOR_STOP_EP;
        motor->notify(notify);
        notify.title=ENDPOINT_DISARM;
        endpoint->notify(notify);
        notify.title=MELODY_STOP;
        speaker->notify(notify);
        ESP_LOGW("ENTPOINT","ENDPOINT WORKED!");
        break;
    default:
        break;
    }

}

void motor_event(yss_event_t event){
    notify_t notify;
    switch (event.button)
    {
    case MOTOR_STOP_EMERG:
        notify.title=ENDPOINT_DISARM;
        endpoint->notify(notify);
        notify.title=MELODY_STOP;
        speaker->notify(notify);
        break;
    case MOTOR_STOP_BTN:
        notify.title=ENDPOINT_DISARM;
        endpoint->notify(notify);
        notify.title=MELODY_STOP;
        speaker->notify(notify);
        break;
    default:
        break;
    }

}

void pult_event(yss_event_t event){
    notify_t notify;
    switch (event.button)
    {
    case BUTTON_UP:
        ESP_LOGW("PULT","24 WORKED!");
        notify.title=MOTOR_UP;
        motor->notify(notify);
        if (!repair_mode){
        notify.title=ENDPOINT_ARM;
        endpoint->notify(notify);
        notify.title=MELODY1_START;
        speaker->notify(notify);
        }
        break;
    case BUTTON_DOWN:
        ESP_LOGW("PULT","22 WORKED!");
        notify.title=MOTOR_DOWN;
        motor->notify(notify);
        if (!repair_mode){
        notify.title=ENDPOINT_ARM;
        endpoint->notify(notify);
        notify.title=MELODY2_START;
        speaker->notify(notify);
        }
        break;
    case BUTTON_STOP:
        repair_mode=false;
        ESP_LOGW("PULT","23 WORKED!");
        notify.title=MOTOR_STOP_BTN;
        motor->notify(notify);
        notify.title=MELODY_STOP;
        speaker->notify(notify);
        break;
    default:
        ESP_LOGW("PULT","DEVICE=%d COMMAND=%d",event.count,event.button);
        break;
    }


}

void rx433_event(yss_event_t event){
    notify_t notify;
    switch (event.data)
    {
    case 100:
        if (event.count==0){
        ESP_LOGW("PULT","23 WORKED!");
        notify.title=MOTOR_STOP_BTN;
        motor->notify(notify);
        notify.title=MELODY_STOP;
        speaker->notify(notify);
        }
        break;
        default:
          ESP_LOGW("RX433","DATA=%d (%d) LENGTH=%d PROTOCOL=%d",event.data,event.data<<1 | 0x1 ,event.count,event.button);  
        break;
    }

  }

void button_event(yss_event_t event){
    notify_t notify;
    switch (event.state)
    {
    case BTN_CLICK:
        if (event.count==0){
        ESP_LOGW("PULT","23 WORKED!");
        notify.title=MOTOR_STOP_BTN;
        motor->notify(notify);
        notify.title=MELODY_STOP;
        speaker->notify(notify);
        }
        break;
     case BTN_LONGCLICK:
        repair_mode=true;
        ticks=xTaskGetTickCount();
     break;   
    // case BUTTON_DOWN:
    //     ESP_LOGW("PULT","22 WORKED!");
    //     notify.title=MOTOR_DOWN;
    //     motor->notify(notify);
    //     notify.title=ENDPOINT_ARM;
    //     endpoint->notify(notify);
    //     notify.title=MELODY2_START;
    //     speaker->notify(notify);
    //     break;
    // case BUTTON_STOP:
    //     ESP_LOGW("PULT","23 WORKED!");
    //     notify.title=MOTOR_STOP_BTN;
    //     motor->notify(notify);
    //     notify.title=MELODY_STOP;
    //     speaker->notify(notify);
    //     break;
    default:
        ESP_LOGW("BUTTON","IS_LONG=%s COUNT=%d",event.state==BTN_LONGCLICK?"YES":"NO",event.count);
        break;
    }


}

#define NUM_LEDS 1
#define WS2812_T0H_TICKS (4)  // 0.4us
#define WS2812_T0L_TICKS (9)  // 0.9us (Total 1.3us for '0')
#define WS2812_T1H_TICKS (8)  // 0.8us
#define WS2812_T1L_TICKS (5)  // 0.5us (Total 1.3us for '1')
static uint8_t led_strip_pixels[NUM_LEDS * 3];
rmt_channel_handle_t rmt_tx_channel;
rmt_encoder_handle_t rmt_bytes_encoder;
rmt_transmit_config_t transmit_config;
void led_init(){
rmt_tx_channel_config_t tx_chan_config = {
        .gpio_num = GPIO_NUM_8,
        .clk_src = RMT_CLK_SRC_DEFAULT, // Use default clock source (usually APB CLK)
        .resolution_hz = 10000000,
        .mem_block_symbols = 64, // ESP32/S3/S2 typically have 64 symbols per block.
                                 // C3/C6/H2 might have 48. Driver can chain blocks if needed.
                                 // Set to a value >= (bytes_to_transmit * 8) if you want to ensure one transaction.
                                 // Or let driver manage it.
        .trans_queue_depth = 4, // Allows queuing of transactions
        
        .flags={
          .invert_out = false,
         } // WS2812 data is not typically inverted
    };

    //tx_chan_config.flags.invert_out=false;
    rmt_tx_channel = NULL;
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &rmt_tx_channel));

    //ESP_LOGI(TAG, "Install RMT bytes encoder for WS2812");
    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .duration0 = WS2812_T0H_TICKS,
            .level0 = 1,
            .duration1 = WS2812_T0L_TICKS,
            .level1 = 0,
        },
        .bit1 = {
            .duration0 = WS2812_T1H_TICKS,
            .level0 = 1,
            .duration1 = WS2812_T1L_TICKS,
            .level1 = 0,
        },
        .flags={
          .msb_first = 1 
        }// WS2812 data is MSB first
    };
    //bytes_encoder_config.flags.msb_first=1;
    rmt_bytes_encoder = NULL;
    ESP_ERROR_CHECK(rmt_new_bytes_encoder(&bytes_encoder_config, &rmt_bytes_encoder));

    //ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(rmt_tx_channel));

    transmit_config.loop_count = 0;
    

}


void app_main(void)
{
    //gpio_set_direction(GPIO_NUM_12,GPIO_MODE_INPUT);
    //gpio_set_pull_mode(GPIO_NUM_12,GPIO_FLOATING);
    
led_init();
repair_mode=false;
queue= xQueueCreate(QUEUE_LENGTH,sizeof(yss_event_t));
flags=xEventGroupCreate();
xEventGroupClearBits(flags,0xFFFF);
    /* Print chip information */
esp_chip_info_t chip_info;
uint32_t flash_size;
esp_chip_info(&chip_info);

    // Настройка и запуск шины I2C #0
i2c_master_bus_config_t i2c_master_config;
i2c_master_config.clk_source = I2C_CLK_SRC_DEFAULT; // Источник синхронизации для шины
i2c_master_config.i2c_port = I2C_NUM_0;   
i2c_master_config.scl_io_num=GPIO_SCL;
i2c_master_config.sda_io_num=GPIO_SDA;
i2c_master_config.flags.enable_internal_pullup = 1; // Использовать встроенную подтяжку GPIO
i2c_master_config.glitch_ignore_cnt = 7;            // Период сбоя данных на шине, стандартное значение 7
i2c_master_config.intr_priority = 0;                // Приоритет прерывания: авто
i2c_master_config.trans_queue_depth = 0;            // Глубина внутренней очереди. Действительно только при асинхронной передач
i2c_new_master_bus(&i2c_master_config, &bus_handle); 

//display = new Display(&bus_handle);
//display->setup("Privet!");

    vTaskDelay(pdMS_TO_TICKS(1000));       
    wifi=new WiFiTask("WiFi",4096,queue,flags);
    wifi->run();
    wifi->resume();
    
    vTaskDelay(pdMS_TO_TICKS(1000));       
    ir=new RMTTask("IR task",2048,queue,GPIO_IR);
    ir->run();
    ir->resume();
    
     vTaskDelay(pdMS_TO_TICKS(1000));       
     motor=new MotorTask("Motor task",1024+256,queue,GPIO_MOTOR_UP,GPIO_MOTOR_DOWN);
     motor->run();
     motor->resume();

     vTaskDelay(pdMS_TO_TICKS(1000));       
     http=new HTTPServer("HTTP",4096,flags,queue,&state.st);
     http->run();
     http->resume();
    
    // vTaskDelay(pdMS_TO_TICKS(1000));       
    // endpoint= new EndpointTask("Endpoints", 1024+256, queue, GPIO_ENPIONT_DATA, GPIO_ENPIONT_SUPPLY);
    // endpoint->run();
    // endpoint->resume();

     vTaskDelay(pdMS_TO_TICKS(1000));       
    speaker= new SpeakerTask("Speaker", 1024*3, queue, GPIO_SPEAKER);
    speaker->run();
    speaker->resume();

    vTaskDelay(pdMS_TO_TICKS(1000));       
    button= new ButtonTask("Button", 1024*2, queue, GPIO_BUTTON);
    button->run();
    button->resume();


    vTaskDelay(pdMS_TO_TICKS(1000));       
    rx433= new RX433Task("rx433", 1024*2, queue, GPIO_RX433);
    rx433->run();
    rx433->resume();

    vTaskDelay(pdMS_TO_TICKS(1000));       
    tx433= new TX433Task("tx433", 1024*2, queue, GPIO_TX433);
    tx433->run();
    tx433->resume();


    // vTaskDelay(pdMS_TO_TICKS(1000));       
    // ens160= new ENS160Task("ens160", 1024*2, queue, &bus_handle);
    // ens160->run();
    // ens160->resume();
    
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    
    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");


     int8_t red = 0, green = 0, blue = 0, max=32;
     bool color_up = true; 

    while (1){   
   for (int i = 0; i < NUM_LEDS; i++) {
            // GRB order
            led_strip_pixels[i * 3 + 0] = green;
            led_strip_pixels[i * 3 + 1] = red;
            led_strip_pixels[i * 3 + 2] = blue;
        }
    if (!repair_mode)    
    {
     red=0;
     blue=0; 
    if (color_up){
    green++;
      if (green==max) color_up=false;
    }else{
    green--;
      if (green==0)  color_up=true;
    }
  }
  else{
    green=0;
    red=max;
  }
       // Transmit the pixel data
        ESP_ERROR_CHECK(rmt_transmit(rmt_tx_channel, rmt_bytes_encoder, led_strip_pixels, sizeof(led_strip_pixels), &transmit_config));
        
        // Wait for the transmission to complete to ensure the ~50us+ reset time
        // This also ensures the data is latched by the LEDs before changing the buffer for the next frame.
        // The RMT driver automatically handles the reset time if the bus is idle after transmission.
        // rmt_tx_wait_all_done is good practice here.
        esp_err_t ret = rmt_tx_wait_all_done(rmt_tx_channel, pdMS_TO_TICKS(100)); // Timeout 100ms
        if (ret != ESP_OK) {
            ESP_LOGW("RMT", "RMT TX wait all done failed: %s", esp_err_to_name(ret));
            // Potentially re-initialize or handle error
        }

if (repair_mode){
  if ((xTaskGetTickCount()-ticks)>pdMS_TO_TICKS(10000)) repair_mode=false;
}
yss_event_t command;
if (xQueueReceive(queue,&command,10)==pdTRUE)
{
  
  switch((uint16_t)(command.state))
  {
   case BTN_CLICK:
   case BTN_LONGCLICK:
       button_event(command);
   break; 
   case ENDPOINT_EVENT:
       endpoint_event(command);
   break;
      case MOTOR_EVENT:
       motor_event(command);
   break;
   break;
      case RX433_EVENT:
       rx433_event(command);
   break;
   case PULT_BUTTON:
       pult_event(command);
   break;
  default:
  break;
  }
  
}

    // printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    // for (int i = 10; i >= 0; i--) {
    //     printf("Restarting in %d seconds...\n", i);
    //     vTaskDelay(500 / portTICK_PERIOD_MS);
    // }
    //  gpio_set_level(PIN_UP,gpio_get_level(PIN_DOWN));
     
    //  gpio_set_level(PIN_DOWN,!gpio_get_level(PIN_UP));
    //  //printf("GPIO_32=%d GPIO_33=%d\n", gpio_ _level(GPIO_NUM_32),gpio_get_level(GPIO_NUM_33));

}
    //printf("Restarting now.\n");
    //fflush(stdout);
    //esp_restart();
}

/*
static void IRAM_ATTR rxIsrHandler(void* arg)
{
  static uint64_t usTimePrev = 0;
  static uint64_t usTimeCurr = 0;
  static uint16_t cntChanges = 0;
  static uint16_t cntRepeats = 0;

  usTimeCurr = esp_timer_get_time();
  uint16_t usDuration = usTimeCurr - usTimePrev;
  if (usDuration > nSeparationLimit) {
    // A long stretch without signal level change occurred. 
    // This could be the gap between two transmission.
    if ((cntRepeats == 0) || (diff(usDuration, _timings[0]) < 200)) {
      // This long signal is close in length to the long signal which
      // started the previously recorded _timings; this suggests that
      // it may indeed by a a gap between two transmissions (we assume
      // here that a sender will send the signal multiple times,
      // with roughly the same gap between them).
      cntRepeats++;
      if (cntRepeats == 2) {
        for(uint8_t i = 1; i <= numProtocols; i++) {
          if (rxDetectProtocol(i, cntChanges)) {
            // receive succeeded for protocol i, post data to external queue
            QueueHandle_t queueProc = (QueueHandle_t)arg;
            if (queueProc) {
              input_data_t data;
              data.source = IDS_RX433;
              data.rx433.protocol = i;
              data.rx433.value = rx433_GetReceivedValue();
              data.count = rx433_GetReceivedBitLength();
              // reset recieved value
              rx433_ResetAvailable();
              // we have not woken a task at the start of the ISR
              BaseType_t xHigherPriorityTaskWoken = pdFALSE;
              // post data
              xQueueSendFromISR(queueProc, &data, &xHigherPriorityTaskWoken);
              // now the buffer is empty we can switch context if necessary.
              if (xHigherPriorityTaskWoken) {
                portYIELD_FROM_ISR();
              };
            } else {
              // reset recieved value
              rx433_ResetAvailable();
            };
            break;
          };
        };
        cntRepeats = 0;
      };
    };
    cntChanges = 0;
  };

  // Detect overflow
  if (cntChanges >= RX433_SWITCH_MAX_CHANGES) {
    cntChanges = 0;
    cntRepeats = 0;
  };
  _timings[cntChanges++] = usDuration;
  usTimePrev = usTimeCurr;
}
*/

