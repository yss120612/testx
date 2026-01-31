#include "MotorTask.h"
#include "esp_log.h"
#include "Settings.h"

void MotorTask::setup()
{
  LEVEL = false;
  gpio_set_direction(up_pin, GPIO_MODE_OUTPUT);
  gpio_set_level(up_pin, !LEVEL);
  gpio_set_direction(down_pin, GPIO_MODE_OUTPUT);
  gpio_set_level(down_pin, !LEVEL);
  in_motion_up = false;
  in_motion_down = false;
  _timer = xTimerCreate("ProtectTimer", pdMS_TO_TICKS(EMERGENCY), pdFALSE, static_cast<void *>(this), timerCb);
  //  last_time=xTaskGetTickCount();
}

void MotorTask::cleanup()
{
  xTimerDelete(_timer, 0);
}

void MotorTask::timerCb(TimerHandle_t tm)
{
  MotorTask *mt = static_cast<MotorTask *>(pvTimerGetTimerID(tm));
  if (mt)
    mt->timerCallback();
}

void MotorTask::timerCallback()
{
  if (xTimerGetPeriod(_timer) > pdMS_TO_TICKS(ROLLBACK) * 2)
  {
    stopMotion(MOTOR_STOP_EMERG);
  }
  else
  {
    stopMotion(MOTOR_STOP_ROLLBACK);
  }
}

void MotorTask::stopMotion(notify_title_t stop_reason)
{
  if (stop_reason == MOTOR_STOP_EMERG || stop_reason == MOTOR_STOP_BTN) 
  {
    yss_event_t event;
    event.state = MOTOR_EVENT;
    event.button = stop_reason;
    xQueueSend(que,&event,0);
  }
  xTimerStop(_timer, 0);
  gpio_set_level(up_pin, !LEVEL);
  in_motion_up = false;
  gpio_set_level(down_pin, !LEVEL);
  in_motion_down = false;
}

void MotorTask::upMotion()
{
  if (in_motion_down)
  {
    stopMotion(MOTOR_STOP_BTN);
    return;
  }else if (in_motion_up){
    return;
  }
  else
  {
    gpio_set_level(up_pin, LEVEL);
    gpio_set_level(down_pin, !LEVEL);
    in_motion_up = true;
    xTimerChangePeriod(_timer, pdMS_TO_TICKS(EMERGENCY), 0);
  }
}

void MotorTask::downMotion()
{
  if (in_motion_up)
  {
    stopMotion(MOTOR_STOP_BTN);
    return;
  }else if (in_motion_down){
    return;
  }
  else
  {
    gpio_set_level(up_pin,!LEVEL);
    gpio_set_level(down_pin, LEVEL);
    in_motion_down = true;
    xTimerChangePeriod(_timer, pdMS_TO_TICKS(EMERGENCY), 0);
  }
}

void MotorTask::shift_down()
{
  vTaskDelay(pdMS_TO_TICKS(100));
  gpio_set_level(up_pin, !LEVEL);
  vTaskDelay(pdMS_TO_TICKS(100));
  gpio_set_level(down_pin, LEVEL);
  in_motion_up = false;
  in_motion_down = true;
  xTimerChangePeriod(_timer, pdMS_TO_TICKS(ROLLBACK), 0);
}

void MotorTask::shift_up()
{
  vTaskDelay(pdMS_TO_TICKS(100));
  gpio_set_level(up_pin, LEVEL);
  vTaskDelay(pdMS_TO_TICKS(100));
  gpio_set_level(down_pin, !LEVEL);
  in_motion_up = true;
  in_motion_down = false;
  xTimerChangePeriod(_timer, pdMS_TO_TICKS(ROLLBACK), 0);
}

void MotorTask::loop()
{
  uint32_t command;
  notify_t nt;
  if (xTaskNotifyWait(0, 0, &command, portMAX_DELAY))
  {
    memcpy(&nt, &command, sizeof(command));

    switch (nt.title)
    {
    case MOTOR_UP:
      upMotion();
      break;
    case MOTOR_DOWN:
      downMotion();
      break;
    case MOTOR_STOP_BTN:
      if (in_motion_down || in_motion_up)
        stopMotion(MOTOR_STOP_BTN);
      break;
    case MOTOR_STOP_EP:
      if (in_motion_up)
      {
        stopMotion(MOTOR_STOP_EP);
        shift_down();
      }
      else if (in_motion_down)
      {
        stopMotion(MOTOR_STOP_EP);
        shift_up();
      }
      break;
    }
  }
  vTaskDelay(pdMS_TO_TICKS(10));
}