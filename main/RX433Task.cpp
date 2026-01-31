#include "RX433Task.h"
#include "esp_log.h"
//#include "esp_timer.h"

#include "Settings.h"

#define TAG "RX433"
static const rxProtocol_t rxProtocols[] = {
  { 350, {  1, 31 }, {  1,  3 }, {  3,  1 }, false },    // protocol 1 (Generic PIR)
  { 650, {  1, 10 }, {  1,  2 }, {  2,  1 }, false },    // protocol 2
  { 100, { 30, 71 }, {  4, 11 }, {  9,  6 }, false },    // protocol 3
  { 380, {  1,  6 }, {  1,  3 }, {  3,  1 }, false },    // protocol 4
  { 500, {  6, 14 }, {  1,  2 }, {  2,  1 }, false },    // protocol 5
  { 450, { 23,  1 }, {  1,  2 }, {  2,  1 }, true },     // protocol 6 (HT6P20B)
  { 150, {  2, 62 }, {  1,  6 }, {  6,  1 }, false },    // protocol 7 (HS2303-PT, i. e. used in AUKEY Remote)
  { 200, {  3, 130}, {  7, 16 }, {  3,  16}, false},     // protocol 8 Conrad RS-200 RX
  { 200, { 130, 7 }, {  16, 7 }, { 16,  3 }, true},      // protocol 9 Conrad RS-200 TX
  { 365, { 18,  1 }, {  3,  1 }, {  1,  3 }, true },     // protocol 10 (1ByOne Doorbell)
  { 270, { 36,  1 }, {  1,  2 }, {  2,  1 }, true },     // protocol 11 (HT12E)
  { 320, { 36,  1 }, {  1,  2 }, {  2,  1 }, true }      // protocol 12 (SM5212)
};

// enum {
//    numProtocols = sizeof(rxProtocols) / sizeof(rxProtocols[0])
// };

static const uint8_t numProtocols = sizeof(rxProtocols) / sizeof(rxProtocols[0]);
//#define numProtocols 12







static inline uint16_t diff(uint16_t A, uint16_t B) 
{
  return abs(A - B);
} 

bool RX433Task::rxDetectProtocol(const uint8_t p, uint16_t changeCount) 
{
  const rxProtocol_t &pro = rxProtocols[p-1];
  uint32_t code = 0;
  
  // Assuming the longer pulse length is the pulse captured in _timings[0]
  const uint16_t syncLengthInPulses = ((pro.syncFactor.low) > (pro.syncFactor.high)) ? (pro.syncFactor.low) : (pro.syncFactor.high);
  const uint16_t delay = RX433Task::_timings[0] / syncLengthInPulses;
  const uint16_t delayTolerance = delay * nReceiveTolerance / 100;

  /* For protocols that start low, the sync period looks like
    *               _________
    * _____________|         |XXXXXXXXXXXX|
    *
    * |--1st dur--|-2nd dur-|-Start data-|
    *
    * The 3rd saved duration starts the data.
    *
    * For protocols that start high, the sync period looks like
    *
    *  ______________
    * |              |____________|XXXXXXXXXXXXX|
    *
    * |-filtered out-|--1st dur--|--Start data--|
    *
    * The 2nd saved duration starts the data
    */
  const unsigned int firstDataTiming = (pro.invertedSignal) ? (2) : (1);

  for (unsigned int i = firstDataTiming; i < changeCount - 1; i += 2) {
    code <<= 1;
    if (diff(RX433Task::_timings[i], delay * pro.zero.high) < delayTolerance &&
        diff(RX433Task::_timings[i + 1], delay * pro.zero.low) < delayTolerance) {
      // zero
    } 
    else if (diff(RX433Task::_timings[i], delay * pro.one.high) < delayTolerance &&
             diff(RX433Task::_timings[i + 1], delay * pro.one.low) < delayTolerance) {
      // one
      code |= 1;
    } 
    else {
      // failed
      return false;
    }
  }

  // Ignore very short transmissions: no device sends them, so this must be noise
  if (changeCount > RX433_SWITCH_MIN_CHANGES) {    
    RX433Task::_receivedValue = code;
    RX433Task::_receivedBitlength = (changeCount - 1) / 2;
    //RX433Task::_receivedDelay = delay;
    //RX433Task::_receivedProtocol = p;
    return true;
  };

  return false;
}

void RX433Task::rxIsrHandler(void* arg)
{
  static uint64_t usTimePrev = 0;
  static uint64_t usTimeCurr = 0;
  static uint16_t cntChanges = 0;
  static uint16_t cntRepeats = 0;

  usTimeCurr =pdTICKS_TO_MS(xTaskGetTickCount());
  uint16_t usDuration = usTimeCurr - usTimePrev;
  if (usDuration<101) return;
   
  if (usDuration > nSeparationLimit) {
    // A long stretch without signal level change occurred. 
    // This could be the gap between two transmission.
    if ((cntRepeats == 0) || (diff(usDuration, RX433Task::_timings[0]) < 200)) {
      // This long signal is close in length to the long signal which
      // started the previously recorded _timings; this suggests that
      // it may indeed by a a gap between two transmissions (we assume
      // here that a sender will send the signal multiple times,
      // with roughly the same gap between them).
      cntRepeats++;
      if (cntRepeats == 2) {
        ESP_DRAM_LOGE("INT","Check!");
        for(uint8_t i = 1; i <= numProtocols; i++) {
          if (rxDetectProtocol(i, cntChanges)) {
            RX433Task * rxt = static_cast<RX433Task *>(arg);
            yss_event_t event;
            event.state=RX433_EVENT;

            event.data=_receivedValue;
            event.count=_receivedBitlength;
            event.button=i;//protocol
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            // post data
            xQueueSendFromISR(rxt->que, &event, &xHigherPriorityTaskWoken);
               if (xHigherPriorityTaskWoken) {
                 portYIELD_FROM_ISR();
               };

            //receive succeeded for protocol i, post data to external queue
            // QueueHandle_t queueProc = (QueueHandle_t)arg;

            // if (queueProc) {
            //   input_data_t data;
            //   data.source = IDS_RX433;
            //   data.rx433.protocol = i;
            //   data.rx433.value = rx433_GetReceivedValue();
            //   data.count = rx433_GetReceivedBitLength();
            //   // reset recieved value
            //   rx433_ResetAvailable();
            //   // we have not woken a task at the start of the ISR
            //    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            //   // post data
            //   xQueueSendFromISR(queueProc, &data, &xHigherPriorityTaskWoken);
            //   now the buffer is empty we can switch context if necessary.
            //   if (xHigherPriorityTaskWoken) {
            //     portYIELD_FROM_ISR();
            //   };
            // } else {
            //   // reset recieved value
            //   ResetAvailable();
            // };
            RX433Task::_receivedValue = 0;
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
  RX433Task::_timings[cntChanges++] = usDuration;
  usTimePrev = usTimeCurr;
}

void RX433Task::setup(){
  esp_err_t err;
  gpio_reset_pin(data_pin);  
  gpio_set_direction(data_pin,GPIO_MODE_INPUT);
  gpio_set_pull_mode(data_pin,GPIO_PULLDOWN_ONLY);
  gpio_set_intr_type(data_pin, GPIO_INTR_ANYEDGE);
  err = gpio_isr_handler_add(data_pin, rxIsrHandler, this);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to attach interrupt!");
  };
  err = gpio_intr_enable(data_pin);
  if (err == ESP_OK) {
    ESP_LOGI(TAG, "Receiver 433MHz started");
  } else {
    ESP_LOGE(TAG, "Failed to start 433MHz receiver");
  };
    
    //ERR_CHECK(gpio_set_intr_type(data_pin, GPIO_INTR_ANYEDGE), ERR_GPIO_SET_ISR);
    //ERR_CHECK(gpio_isr_handler_add(data_pin, rxIsrHandler, this), ERR_GPIO_SET_ISR);

    
    // gpio_isr_handler_add(data_pin, sensorActivatedStatic, this);
    // gpio_set_intr_type(data_pin, GPIO_INTR_POSEDGE);
    // gpio_intr_enable(data_pin);

   
    _timer = xTimerCreate("EndpointTimer", pdMS_TO_TICKS(50), pdFALSE, static_cast<void *>(this), timer_callback_static);
}



void RX433Task::timer_callback_static(TimerHandle_t tm)
{
  RX433Task *rt = static_cast<RX433Task *>(pvTimerGetTimerID(tm));
  if (rt)
    rt->timer_callback();
}

void RX433Task::timer_callback()
{

}

void RX433Task::cleanup(){
  gpio_intr_disable(data_pin);
  xTimerDelete(_timer, 0);
}



void RX433Task::loop(){
 uint32_t command;
  notify_t nt;
  if (xTaskNotifyWait(0, 0, &command, portMAX_DELAY))
  {
    memcpy(&nt, &command, sizeof(command));

    switch (nt.title)
    {
        case ENDPOINT_ARM:
         
        break;
        case ENDPOINT_DISARM:
         
        break;
    }
}
vTaskDelay(pdMS_TO_TICKS(10));
}

// bool RX433Task::IsAvailable()
// {
//   return _receivedValue != 0;
// }

// void RX433Task::ResetAvailable()
// {
//   _receivedValue = 0;
// }

// uint32_t RX433Task::GetReceivedValue()
// {
//   return _receivedValue;
// }

// uint16_t RX433Task::GetReceivedBitLength()
// {
//   return _receivedBitlength;
// }

// uint16_t RX433Task::GetReceivedDelay()
// {
//   return _receivedDelay;
// }

// uint16_t RX433Task::GetReceivedProtocol()
// {
//   return _receivedProtocol;
// }