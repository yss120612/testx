#ifndef __MOTORTASK
#define __MOTORTASK
#include "Task.h"

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "Settings.h"

#define EMERGENCY 40000
#define ROLLBACK 300

class MotorTask : public Task
{
public:
MotorTask(const char *name, uint32_t stack, QueueHandle_t q, gpio_num_t pup, gpio_num_t pdown):
Task(name, stack){que=q;up_pin=pup;down_pin=pdown;};

protected:
void cleanup() override;
void setup() override;
void loop() override;
void upMotion();
void downMotion();
//0 stop withowt message in queue
//1 stop by emergency period
//2 stop by endstop

void stopMotion(notify_title_t stop_reason);
void shift_up();
void shift_down();
static void timerCb(TimerHandle_t tm);
void timerCallback();


QueueHandle_t que;
gpio_num_t up_pin, down_pin;
bool in_motion_up,in_motion_down;
TimerHandle_t _timer;
bool LEVEL;
//TickType_t last_time;
};

#endif