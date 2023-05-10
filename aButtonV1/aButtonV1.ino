/*
Copyright (c) 2023 Partymaster88

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <Arduino.h>
#include <arduino_homekit_server.h>
#include "wifi_info.h"
#include "ESPButton.h"

#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);

void setup() {
  Serial.begin(115200);
  wifisetup(); // in wifi_info.h
  //masterreset(); //factory reset device 
  my_homekit_setup();
}

void loop() {
  wifiloop();
  my_homekit_loop();
  delay(10);
}

void masterreset()  {

  homekit_storage_reset(); // reset homekit storage
  delay(5000);             //delay for homekit Reset
  EEPROM.write(0, 0);      // clear EEPROM
  EEPROM.commit();         // save changes

}

//==============================
// HomeKit setup and loop
//==============================

extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t cha_programmable_switch_event1;
extern "C" homekit_characteristic_t cha_programmable_switch_event2;
extern "C" homekit_characteristic_t cha_programmable_switch_event3;

#define PIN_BUTTON_1 0 // button 1 pin
#define PIN_BUTTON_2 1 // button 2 pin
#define PIN_BUTTON_3 2 // button 3 pin
#define PIN_LED 4      // led pin

#define HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_SINGLE_PRESS   0
#define HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_DOUBLE_PRESS   1
#define HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_LONG_PRESS     2

// Called when the value is read by iOS Home APP
homekit_value_t cha_programmable_switch_event_getter() {
  // Should always return "null" for reading, see HAP section 9.75
  return HOMEKIT_NULL_CPP();
}

void my_homekit_setup() {
  pinMode(PIN_BUTTON_1, INPUT_PULLUP);
  pinMode(PIN_BUTTON_2, INPUT_PULLUP);
  pinMode(PIN_BUTTON_3, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);

  digitalWrite(PIN_LED, LOW);
  
  ESPButton.add(0, PIN_BUTTON_1, LOW, true, true);
  ESPButton.add(1, PIN_BUTTON_2, LOW, true, true);
  ESPButton.add(2, PIN_BUTTON_3, LOW, true, true);
  
  ESPButton.setCallback([&](uint8_t id, ESPButtonEvent event) {
    LOG_D("Button %d Event: %s", id + 1, ESPButton.getButtonEventDescription(event));
    uint8_t cha_value = 0;
    if (event == ESPBUTTONEVENT_SINGLECLICK) {
      cha_value = HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_SINGLE_PRESS;
      digitalWrite(PIN_LED, HIGH);
      delay(100);
      digitalWrite(PIN_LED, LOW);
    } else if (event == ESPBUTTONEVENT_DOUBLECLICK) {
      cha_value = HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_DOUBLE_PRESS;
      digitalWrite(PIN_LED, HIGH);
      delay(100);
      digitalWrite(PIN_LED, LOW);
      delay(100);
      digitalWrite(PIN_LED, HIGH);
      delay(100);
      digitalWrite(PIN_LED, LOW);
    } else if (event == ESPBUTTONEVENT_LONGCLICK) {
      cha_value = HOMEKIT_PROGRAMMABLE_SWITCH_EVENT_LONG_PRESS;
      digitalWrite(PIN_LED, HIGH);
      delay(1000);
      digitalWrite(PIN_LED, LOW);
    }
    
    switch (id) {
      case 0:
        cha_programmable_switch_event1.value.uint8_value = cha_value;
        homekit_characteristic_notify(&cha_programmable_switch_event1, cha_programmable_switch_event1.value);
        break;
      case 1:
        cha_programmable_switch_event2.value.uint8_value = cha_value;
        homekit_characteristic_notify(&cha_programmable_switch_event2, cha_programmable_switch_event2.value);
        break;
      case 2:
        cha_programmable_switch_event3.value.uint8_value = cha_value;
        homekit_characteristic_notify(&cha_programmable_switch_event3, cha_programmable_switch_event3.value);
        break;
}
});
ESPButton.begin();

cha_programmable_switch_event1.getter = cha_programmable_switch_event_getter;
cha_programmable_switch_event2.getter = cha_programmable_switch_event_getter;
cha_programmable_switch_event3.getter = cha_programmable_switch_event_getter;

arduino_homekit_setup(&config);
}

static uint32_t next_heap_millis = 0;

void my_homekit_loop() {
ESPButton.loop();
arduino_homekit_loop();
const uint32_t t = millis();
if (t > next_heap_millis) {
// Show heap info every 5 seconds
next_heap_millis = t + 5 * 1000;
LOG_D("Free heap: %d, HomeKit clients: %d",
ESP.getFreeHeap(), arduino_homekit_connected_clients_count());
}
}
