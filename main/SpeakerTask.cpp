#include "SpeakerTask.h"
#include "esp_log.h"
#include "Settings.h"

void SpeakerTask::setup()
{
  gpio_set_direction(_pin, GPIO_MODE_OUTPUT);
  gpio_set_level(_pin, 0);
  //_timer = xTimerCreate("SpkTimer", pdMS_TO_TICKS(1000), pdFALSE, static_cast<void *>(this), timerCb);

   ledc_timer = {
        .speed_mode = SPEED,
        .duty_resolution = RESOLUTION,
        .timer_num = TIMER_NO,
        .freq_hz = 1000,      // Установка частоты здесь
        .clk_cfg = LEDC_AUTO_CLK,    // Автоматический выбор источника тактовой частоты
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // 2. Конфигурация канала ШИМ
    ledc_channel = {
        .gpio_num = _pin,
        .speed_mode = SPEED,
        .channel = CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = TIMER_NO,
        .duty = 0,      // Начальный коэффициент заполнения GJKJDBYF
        .hpoint = 0            // Точка переключения (обычно 0)
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    current_note=-1;
}

void SpeakerTask::cleanup()
{
  //xTimerDelete(_timer, 0);
}

void SpeakerTask::timerCb(TimerHandle_t tm)
{
  SpeakerTask *st = static_cast<SpeakerTask *>(pvTimerGetTimerID(tm));
  if (st)
    st->timerCallback();
}

void SpeakerTask::timerCallback()
{
  
}
 esp_err_t SpeakerTask::setFreq(uint32_t f)
 {
    ledc_timer.freq_hz = f;
    return ledc_timer_config(&ledc_timer);
}

//0..99
esp_err_t SpeakerTask::setVolume(uint8_t v)
 {
    ledc_set_duty(SPEED,CHANNEL,(1<<(RESOLUTION))/100*(v>99?99:v));
    return ledc_update_duty(SPEED,CHANNEL);
}

// uint16_t melody[] = { NOTE_E5, NOTE_D5, NOTE_F4, NOTE_G4, NOTE_C5, NOTE_B4, NOTE_D4, NOTE_E4, NOTE_B4, NOTE_A4, NOTE_C4, NOTE_E4, NOTE_A4 };
// uint8_t noteDurations[] = { 8, 8, 4, 4, 8, 8, 4, 4, 8, 8, 4, 4, 1 };

int16_t melody1[] = {
 // Fur Elise - Ludwig van Beethovem
  // Score available at https://musescore.com/user/28149610/scores/5281944

  //starts from 1 ending on 9
  NOTE_E5, 16, NOTE_DS5, 16, //1
  NOTE_E5, 16, NOTE_DS5, 16, NOTE_E5, 16, NOTE_B4, 16, NOTE_D5, 16, NOTE_C5, 16,
  NOTE_A4, -8, NOTE_C4, 16, NOTE_E4, 16, NOTE_A4, 16,
  NOTE_B4, -8, NOTE_E4, 16, NOTE_GS4, 16, NOTE_B4, 16,
  NOTE_C5, 8,  REST, 16, NOTE_E4, 16, NOTE_E5, 16,  NOTE_DS5, 16,
  
  NOTE_E5, 16, NOTE_DS5, 16, NOTE_E5, 16, NOTE_B4, 16, NOTE_D5, 16, NOTE_C5, 16,//6
  NOTE_A4, -8, NOTE_C4, 16, NOTE_E4, 16, NOTE_A4, 16, 
  NOTE_B4, -8, NOTE_E4, 16, NOTE_C5, 16, NOTE_B4, 16, 
  NOTE_A4 , 4, REST, 8, //9 - 1st ending

  //repaets from 1 ending on 10
  NOTE_E5, 16, NOTE_DS5, 16, //1
  NOTE_E5, 16, NOTE_DS5, 16, NOTE_E5, 16, NOTE_B4, 16, NOTE_D5, 16, NOTE_C5, 16,
  NOTE_A4, -8, NOTE_C4, 16, NOTE_E4, 16, NOTE_A4, 16,
  NOTE_B4, -8, NOTE_E4, 16, NOTE_GS4, 16, NOTE_B4, 16,
  NOTE_C5, 8,  REST, 16, NOTE_E4, 16, NOTE_E5, 16,  NOTE_DS5, 16,
  
  NOTE_E5, 16, NOTE_DS5, 16, NOTE_E5, 16, NOTE_B4, 16, NOTE_D5, 16, NOTE_C5, 16,//6
  NOTE_A4, -8, NOTE_C4, 16, NOTE_E4, 16, NOTE_A4, 16, 
  NOTE_B4, -8, NOTE_E4, 16, NOTE_C5, 16, NOTE_B4, 16, 
  NOTE_A4, 8, REST, 16, NOTE_B4, 16, NOTE_C5, 16, NOTE_D5, 16, //10 - 2nd ending
  //continues from 11
  NOTE_E5, -8, NOTE_G4, 16, NOTE_F5, 16, NOTE_E5, 16, 
  NOTE_D5, -8, NOTE_F4, 16, NOTE_E5, 16, NOTE_D5, 16, //12
  
  NOTE_C5, -8, NOTE_E4, 16, NOTE_D5, 16, NOTE_C5, 16, //13
  NOTE_B4, 8, REST, 16, NOTE_E4, 16, NOTE_E5, 16, REST, 16,
  REST, 16, NOTE_E5, 16, NOTE_E6, 16, REST, 16, REST, 16, NOTE_DS5, 16,
  NOTE_E5, 16, REST, 16, REST, 16, NOTE_DS5, 16, NOTE_E5, 16, NOTE_DS5, 16,
  NOTE_E5, 16, NOTE_DS5, 16, NOTE_E5, 16, NOTE_B4, 16, NOTE_D5, 16, NOTE_C5, 16,
  NOTE_A4, 8, REST, 16, NOTE_C4, 16, NOTE_E4, 16, NOTE_A4, 16,
  
  NOTE_B4, 8, REST, 16, NOTE_E4, 16, NOTE_GS4, 16, NOTE_B4, 16, //19
  NOTE_C5, 8, REST, 16, NOTE_E4, 16, NOTE_E5, 16,  NOTE_DS5, 16,
  NOTE_E5, 16, NOTE_DS5, 16, NOTE_E5, 16, NOTE_B4, 16, NOTE_D5, 16, NOTE_C5, 16,
  NOTE_A4, 8, REST, 16, NOTE_C4, 16, NOTE_E4, 16, NOTE_A4, 16,
  NOTE_B4, 8, REST, 16, NOTE_E4, 16, NOTE_C5, 16, NOTE_B4, 16,
  NOTE_A4, 8, REST, 16, NOTE_B4, 16, NOTE_C5, 16, NOTE_D5, 16, //24 (1st ending)
};


int16_t melody2[] = {

  
  // Game of Thrones
  // Score available at https://musescore.com/user/8407786/scores/2156716

  NOTE_G4,8, NOTE_C4,8, NOTE_DS4,16, NOTE_F4,16, NOTE_G4,8, NOTE_C4,8, NOTE_DS4,16, NOTE_F4,16, //1
  NOTE_G4,8, NOTE_C4,8, NOTE_DS4,16, NOTE_F4,16, NOTE_G4,8, NOTE_C4,8, NOTE_DS4,16, NOTE_F4,16,
  NOTE_G4,8, NOTE_C4,8, NOTE_E4,16, NOTE_F4,16, NOTE_G4,8, NOTE_C4,8, NOTE_E4,16, NOTE_F4,16,
  NOTE_G4,8, NOTE_C4,8, NOTE_E4,16, NOTE_F4,16, NOTE_G4,8, NOTE_C4,8, NOTE_E4,16, NOTE_F4,16,
  NOTE_G4,-4, NOTE_C4,-4,//5

  NOTE_DS4,16, NOTE_F4,16, NOTE_G4,4, NOTE_C4,4, NOTE_DS4,16, NOTE_F4,16, //6
  NOTE_D4,-1, //7 and 8
  NOTE_F4,-4, NOTE_AS3,-4,
  NOTE_DS4,16, NOTE_D4,16, NOTE_F4,4, NOTE_AS3,-4,
  NOTE_DS4,16, NOTE_D4,16, NOTE_C4,-1, //11 and 12

  //repeats from 5
  NOTE_G4,-4, NOTE_C4,-4,//5

  NOTE_DS4,16, NOTE_F4,16, NOTE_G4,4, NOTE_C4,4, NOTE_DS4,16, NOTE_F4,16, //6
  NOTE_D4,-1, //7 and 8
  NOTE_F4,-4, NOTE_AS3,-4,
  NOTE_DS4,16, NOTE_D4,16, NOTE_F4,4, NOTE_AS3,-4,
  NOTE_DS4,16, NOTE_D4,16, NOTE_C4,-1, //11 and 12
  NOTE_G4,-4, NOTE_C4,-4,
  NOTE_DS4,16, NOTE_F4,16, NOTE_G4,4,  NOTE_C4,4, NOTE_DS4,16, NOTE_F4,16,

  NOTE_D4,-2,//15
  NOTE_F4,-4, NOTE_AS3,-4,
  NOTE_D4,-8, NOTE_DS4,-8, NOTE_D4,-8, NOTE_AS3,-8,
  NOTE_C4,-1,
  NOTE_C5,-2,
  NOTE_AS4,-2,
  NOTE_C4,-2,
  NOTE_G4,-2,
  NOTE_DS4,-2,
  NOTE_DS4,-4, NOTE_F4,-4, 
  NOTE_G4,-1,
};

void SpeakerTask::playNote(){
 int16_t note=melody[current_note];
 int16_t dvd=melody[current_note+1];   
 uint16_t dur=wholenote/dvd*(dvd<0?-1.5:1);
 if (note!=REST){
        setFreq(note);
        setVolume(70);
        vTaskDelay(pdMS_TO_TICKS(dur * 0.9));
        setVolume(0);
        vTaskDelay(pdMS_TO_TICKS(dur * 0.1));
    }else{
        setVolume(0);
        vTaskDelay(pdMS_TO_TICKS(dur));
    }
}

void SpeakerTask::start_play(int16_t mld[],int len, uint16_t tempo){
wholenote = (60000 * 4) / tempo;
melody_length = len;
current_note=0;
melody=mld;
ESP_LOGE("SPK","melody=%d",len);
ledc_timer_resume(SPEED,TIMER_NO);
//int noteDuration; 
//int16_t divider;
//if (continuePlay){
// for (int thisNote = 0; thisNote < melody_length; thisNote = thisNote + 2) {
 
//     // // calculates the duration of each note
//     // divider = melody[thisNote + 1];
//     // noteDuration = (wholenote) / abs(divider);
//     // if (divider > 0) {
//     //   // regular note, just proceed
//     // } else if (divider < 0) {
//     //   // dotted notes are represented with negative durations!!
//     //   noteDuration *= 1.5; // increases the duration in half for dotted notes
//     // }
//    playNote(melody[thisNote],melody[thisNote]);
//   //}
//   //continuePlay=false;
// }
 
}

void SpeakerTask::stop_melody(){
    current_note=-1;
    melody=NULL;
    setVolume(0);
    ledc_timer_pause(SPEED,TIMER_NO);
}

void SpeakerTask::loop()
{

   uint32_t command;
   notify_t nt;
   
if (xTaskNotifyWait(0, 0, &command, 1))
   {
     memcpy(&nt, &command, sizeof(command));

     switch (nt.title)
     {
     case MELODY1_START:
       start_play(melody1,sizeof(melody1) / sizeof(melody1[0]),80); 
       break;
     case MELODY2_START:
        start_play(melody2,sizeof(melody2) / sizeof(melody2[0]),108); 
       break;
     case MELODY_STOP:
          stop_melody();  
       break;
     case MOTOR_STOP_EP:
     break;
     }
   }
  if (current_note>=0){
    if (current_note<melody_length) {
        playNote();
        current_note+=2;
    }else{
        stop_melody();
    }
  }else{
        vTaskDelay(pdMS_TO_TICKS(10));
  }

  
}

