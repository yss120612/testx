#ifndef __ENDPOINTTASK
#define __ENDPOINTTASK
#include "Task.h"

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"

class EndpointTask : public Task
{
public:
EndpointTask(const char *name, uint32_t stack, QueueHandle_t q, gpio_num_t data, gpio_num_t src):
Task(name, stack){que=q;data_pin=data;src_pin=src;};

protected:
void cleanup() override;
void setup() override;
static void sensorActivatedStatic(void *arg);
void sensorActivated();
void loop() override;
static void timerCb(TimerHandle_t tm);
void timerCallback();
void endpoint_arm(bool arm);
QueueHandle_t que;
gpio_num_t data_pin, src_pin;
TickType_t last_time;
TimerHandle_t _timer;
};

#endif