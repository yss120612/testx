#include "EndpointTask.h"
#include "esp_log.h"
#include "Settings.h"


void EndpointTask::setup(){
    gpio_set_direction(src_pin,GPIO_MODE_OUTPUT);
    endpoint_arm(false);

    esp_err_t err = gpio_install_isr_service(0);
    if (err == ESP_ERR_INVALID_STATE) {
      ESP_LOGW("ISR", "GPIO isr service already installed");
    };

    gpio_set_direction(data_pin,GPIO_MODE_INPUT);
    gpio_set_pull_mode(data_pin,GPIO_PULLDOWN_ONLY);
    gpio_isr_handler_add(data_pin, sensorActivatedStatic, this);
    gpio_set_intr_type(data_pin, GPIO_INTR_NEGEDGE);
    gpio_intr_enable(data_pin);

    last_time=xTaskGetTickCount();
    _timer = xTimerCreate("EndpointTimer", pdMS_TO_TICKS(20), pdFALSE, static_cast<void *>(this), timerCb);
}

void EndpointTask::sensorActivatedStatic(void * arg){
  EndpointTask *et = static_cast<EndpointTask *>(arg);
  if (et)
    et->sensorActivated();
}

void EndpointTask::sensorActivated(){
  //ESP_LOGW("ISR", "Stensor activated");
  //if (xTaskGetTickCountFromISR()-last_time < 20 ) return;
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xTimerStartFromISR(_timer,&xHigherPriorityTaskWoken);
  //last_time=xTaskGetTickCountFromISR();
  gpio_intr_disable(data_pin);
}

void EndpointTask::timerCb(TimerHandle_t tm)
{
  EndpointTask *et = static_cast<EndpointTask *>(pvTimerGetTimerID(tm));
  if (et)
    et->timerCallback();
}

void EndpointTask::timerCallback()
{
  if (gpio_get_level(data_pin)==0){
    yss_event_t event;
    event.state=ENDPOINT_EVENT;
    event.button=ENDPOINT_TRIPPED;
    //gpio_get_level(data_pin);
    xQueueSend(que,&event,0);
  }else{
   gpio_intr_enable(data_pin);
  }
}

void EndpointTask::cleanup(){
  gpio_intr_disable(data_pin);
  xTimerDelete(_timer, 0);
}

void EndpointTask::endpoint_arm(bool arm){
  gpio_set_level(src_pin,arm?1:0);
  vTaskDelay(pdMS_TO_TICKS(10));
  if (arm){
    gpio_intr_enable(data_pin);
  }
  else{
    gpio_intr_disable(data_pin);
  }
}

void EndpointTask::loop(){
 uint32_t command;
  notify_t nt;
  if (xTaskNotifyWait(0, 0, &command, portMAX_DELAY))
  {
    memcpy(&nt, &command, sizeof(command));

    switch (nt.title)
    {
        case ENDPOINT_ARM:
          endpoint_arm(true);
        break;
        case ENDPOINT_DISARM:
          endpoint_arm(false);
        break;
    }
}
vTaskDelay(pdMS_TO_TICKS(10));
}