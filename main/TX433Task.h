#ifndef __TX433TASK
#define __TX433TASK
#include "Task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define nSeparationLimit  4300
#define nReceiveTolerance  60
#define TX433_SWITCH_MAX 26*2 

#define HI_LEVEL 1
#define LO_LEVEL 0

typedef struct txHighLow_t{
    uint8_t high;
    uint8_t low;
} txHighLow_t;

/**
 * A "protocol" describes how zero and one bits are encoded into high/low pulses.
 * 
 * Format for protocol definitions:
 * {pulselength, Sync bit, "0" bit, "1" bit, invertedSignal}
 * 
 * pulselength: pulse length in microseconds, e.g. 350
 * Sync bit: {1, 31} means 1 high pulse and 31 low pulses
 *     (perceived as a 31*pulselength long pulse, total length of sync bit is
 *     32*pulselength microseconds), i.e:
 *      _
 *     | |_______________________________ (don't count the vertical bars)
 * "0" bit: waveform for a data bit of value "0", {1, 3} means 1 high pulse
 *     and 3 low pulses, total length (1+3)*pulselength, i.e:
 *      _
 *     | |___
 * "1" bit: waveform for a data bit of value "1", e.g. {3,1}:
 *      ___
 *     |   |_
 *
 * These are combined to form Tri-State bits when sending or receiving codes.
 */
typedef struct txProtocol_t {
  /** base pulse length in microseconds, e.g. 350 */
  uint16_t pulseLength;

  txHighLow_t syncFactor;
  txHighLow_t zero;
  txHighLow_t one;
  /**
   * If true, interchange high and low logic levels in all transmissions.
   *
   * By default, RCSwitch assumes that any signals it sends or receives
   * can be broken down into pulses which start with a high signal level,
   * followed by a a low signal level. This is e.g. the case for the
   * popular PT 2260 encoder chip, and thus many switches out there.
   *
   * But some devices do it the other way around, and start with a low
   * signal level, followed by a high signal level, e.g. the HT6P20B. To
   * accommodate this, one can set invertedSignal to true, which causes
   * RCSwitch to change how it interprets any HighLow struct FOO: It will
   * then assume transmissions start with a low signal lasting
   * FOO.high*pulseLength microseconds, followed by a high signal lasting
   * FOO.low*pulseLength microseconds.
   */
  bool invertedSignal;
} txProtocol_t; 

 
class TX433Task : public Task
{
public:
TX433Task(const char *name, uint32_t stack, QueueHandle_t q, gpio_num_t data):
Task(name, stack){que=q;data_pin=data;};
// static bool IsAvailable();
// static void ResetAvailable();
// static uint32_t GetReceivedValue();
// static uint16_t GetReceivedBitLength();
// static uint16_t GetReceivedDelay();
// static uint16_t GetReceivedProtocol();

protected:
void cleanup() override;
//static bool IRAM_ATTR rxDetectProtocol(const uint8_t p, uint16_t changeCount);
//static void IRAM_ATTR rxIsrHandler(void *arg);
//static bool rxDetectProtocol(const uint8_t p, uint16_t changeCount);
//static void rxIsrHandler(void *arg);
void setup() override;
void transmit(uint32_t data);
void loop() override;
static void timer_callback_static(void * arg);
void timer_callback();
QueueHandle_t que;
gpio_num_t data_pin;
//TimerHandle_t _timer;

static inline volatile uint32_t _current_pause;
static inline volatile uint32_t _current_cycle;

//static volatile uint16_t _receivedDelay;
//static volatile uint16_t _receivedProtocol;
static inline uint16_t _timings[TX433_SWITCH_MAX]{};

esp_timer_handle_t _timer;
};



#endif