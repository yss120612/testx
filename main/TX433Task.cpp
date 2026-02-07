
#include "TX433Task.h"
#include "esp_log.h"
#include "esp_timer.h"

//#include "Settings.h"

#define TAG "TX433"
// static const rxProtocol_t rxProtocols[] = {
//   { 350, {  1, 31 }, {  1,  3 }, {  3,  1 }, false },    // protocol 1 (Generic PIR)
//   { 650, {  1, 10 }, {  1,  2 }, {  2,  1 }, false },    // protocol 2
//   { 100, { 30, 71 }, {  4, 11 }, {  9,  6 }, false },    // protocol 3
//   { 380, {  1,  6 }, {  1,  3 }, {  3,  1 }, false },    // protocol 4
//   { 500, {  6, 14 }, {  1,  2 }, {  2,  1 }, false },    // protocol 5
//   { 450, { 23,  1 }, {  1,  2 }, {  2,  1 }, true },     // protocol 6 (HT6P20B)
//   { 150, {  2, 62 }, {  1,  6 }, {  6,  1 }, false },    // protocol 7 (HS2303-PT, i. e. used in AUKEY Remote)
//   { 200, {  3, 130}, {  7, 16 }, {  3,  16}, false},     // protocol 8 Conrad RS-200 RX
//   { 200, { 130, 7 }, {  16, 7 }, { 16,  3 }, true},      // protocol 9 Conrad RS-200 TX
//   { 365, { 18,  1 }, {  3,  1 }, {  1,  3 }, true },     // protocol 10 (1ByOne Doorbell)
//   { 270, { 36,  1 }, {  1,  2 }, {  2,  1 }, true },     // protocol 11 (HT12E)
//   { 320, { 36,  1 }, {  1,  2 }, {  2,  1 }, true }      // protocol 12 (SM5212)
// };

static const txProtocol_t protocol = { 350, {  1, 31 }, {  1,  3 }, {  3,  1 }, false };

void TX433Task::setup(){
  esp_err_t err;
  err=gpio_set_direction(data_pin,GPIO_MODE_OUTPUT);
  if (err!=ESP_OK){
        ESP_LOGE("TX", "Error set direction  pin err=%d (%s)",err,esp_err_to_name(err));     
    }
  esp_timer_create_args_t create_args;
  create_args.callback=timer_callback_static;
  create_args.arg=this;
  create_args.dispatch_method=ESP_TIMER_ISR;
  create_args.name="tx433";
  create_args.skip_unhandled_events = false;
  err=esp_timer_create(&create_args, &_timer);
  if (err!=ESP_OK){
        ESP_LOGE("TX", "Error creating timer err=%d (%s)",err,esp_err_to_name(err));     
    }
}

void TX433Task::timer_callback_static(void * arg)
{
  TX433Task *tt = static_cast<TX433Task *>(arg);
  if (tt)
    tt->timer_callback();
}

void TX433Task::timer_callback()
{
   // static int64_t usTimeCurr =0;
   if (_current_pause==TX433_SWITCH_MAX){
    _current_pause=0;
    _current_cycle+=1;
    gpio_set_level(data_pin,LO_LEVEL);     
    if (_current_cycle<3) {
        esp_timer_start_once(_timer,(protocol.syncFactor.low-1)*protocol.pulseLength);
    }
    return;
   }
   gpio_set_level(data_pin,_current_pause%2?LO_LEVEL:HI_LEVEL);
   esp_timer_start_once(_timer,_timings[_current_pause]);
   _current_pause+=1;
   
}

void TX433Task::cleanup(){
  esp_timer_delete(_timer);
}



void TX433Task::transmit(uint32_t data){

//start bit
uint8_t idx=0;
_timings[idx++]=protocol.syncFactor.high*protocol.pulseLength;
_timings[idx++]=protocol.syncFactor.low*protocol.pulseLength;

//24 bit of data
for (int8_t i=23;i>=0;i--){
    if (data>>i & 0x01)
    {
        _timings[idx++]=protocol.one.high*protocol.pulseLength;
        _timings[idx++]=protocol.one.low*protocol.pulseLength;
    }else{
        _timings[idx++]=protocol.zero.high*protocol.pulseLength;
        _timings[idx++]=protocol.zero.low*protocol.pulseLength;
    }
}
_timings[idx++]=protocol.zero.high*protocol.pulseLength;
_timings[idx++]=protocol.zero.high*protocol.pulseLength;
_current_pause=0;
_current_cycle=0;
gpio_set_level(data_pin,LO_LEVEL);
esp_timer_start_once(_timer,10500);
ESP_LOGE("TX", "Transmit=%d idx=%d",data,idx);     
}

void TX433Task::loop(){
 uint32_t command;
  notify_t nt;
  static uint32_t x = 5225810;
  if (xTaskNotifyWait(0, 0, &command, 100))
  {
    memcpy(&nt, &command, sizeof(command));

    switch (nt.title)
    {
         case TRANSMIT_NOW:
            transmit(((nt.packet.value<<16) & 0xFFFF0000) | nt.packet.var);
         break;
   
    }
}

transmit(x);
x+=2;
vTaskDelay(pdMS_TO_TICKS(5000));
}

