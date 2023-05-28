#include <EEPROM.h>
#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

#define RECV_PIN 14 // Input from IR receiver
#define PWM_PIN 12 // Output to IR LED
#define CAPTURE_PIN 5 // Button to trigger capturing
#define BUTTON_PIN_1 19 // Button 1 to trigger sending signal 1
#define BUTTON_PIN_2 18 // Button 2 to trigger sending signal 2
#define LED 13

#define PWM_FREQUENCY 38000 // 38kHz carrier frequency

#define CAPTURE_SIZE 200 // Maximum number of intervals to capture
#define END_OF_SIGNAL 30000 // Length of pause (in microseconds) that signifies end of signal

#define SIGNAL1_LENGTH_ADDRESS 0 // Address for storing length of signal 1
#define SIGNAL1_DATA_ADDRESS 4 // Address for storing data of signal 1
#define SIGNAL2_LENGTH_ADDRESS CAPTURE_SIZE * 2 + 4 // Address for storing length of signal 2
#define SIGNAL2_DATA_ADDRESS SIGNAL2_LENGTH_ADDRESS + 4 // Address for storing data of signal 2

uint16_t captured1[CAPTURE_SIZE];
uint16_t captured2[CAPTURE_SIZE];

uint16_t length1 = 0;
uint16_t length2 = 0;

void setup() {
  Serial.begin(115200);
  EEPROM.begin(CAPTURE_SIZE * 4 + 32); // Initialize EEPROM

  pinMode(RECV_PIN, INPUT);
  pinMode(PWM_PIN, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(CAPTURE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN_1, INPUT_PULLUP);
  pinMode(BUTTON_PIN_2, INPUT_PULLUP);

  // If EEPROM contains valid data, use it
  length1 = EEPROM.readUInt(SIGNAL1_LENGTH_ADDRESS);
  //if(length1 != 0 && length1 <= CAPTURE_SIZE) {
    for(uint16_t i = 0; i < length1; i++) {
      captured1[i] = EEPROM.readUShort(SIGNAL1_DATA_ADDRESS + i * 2);
    }
    Serial.print("Retrieved Button 1 Words from EEPROM = "); Serial.println(length1);
    displayCaptured(captured1, length1);
  //}

  length2 = EEPROM.readUInt(SIGNAL2_LENGTH_ADDRESS);
  //if(length2 != 0 && length2 <= CAPTURE_SIZE) {
    for(uint16_t i = 0; i < length2; i++) {
      captured2[i] = EEPROM.readUShort(SIGNAL2_DATA_ADDRESS + i * 2);
    }
    Serial.print("Retrieved Button 2 Words from EEPROM = "); Serial.println(length2);
    displayCaptured(captured2, length2);
  //}
}

void loop() {
mcpwm_config_t pwm_config;

  if (digitalRead(CAPTURE_PIN) == LOW) {
    delay(1000);
    while((digitalRead(CAPTURE_PIN) == LOW)) { //Wait for button release
      delay(1000);
    }
    // Capture signals
    Serial.println("Press Button 1");
    captureSignal(captured1, &length1);
    displayCaptured(captured1, length1);

    Serial.println("Press Button 2");
    captureSignal(captured2, &length2);
    displayCaptured(captured2, length2);

    Serial.println("Use GPIOs to send");
    // Store signals to EEPROM
    EEPROM.writeUInt(SIGNAL1_LENGTH_ADDRESS, length1);
    for(uint16_t i = 0; i < length1; i++) {
      EEPROM.writeUShort(SIGNAL1_DATA_ADDRESS + i * 2, captured1[i]);
    }
    EEPROM.writeUInt(SIGNAL2_LENGTH_ADDRESS, length2);
    for(uint16_t i = 0; i < length2; i++) {
      EEPROM.writeUShort(SIGNAL2_DATA_ADDRESS + i * 2, captured2[i]);
    }
    EEPROM.commit(); // Save changes to EEPROM
    Serial.print("Written to EEPROM - Button 1 = "); Serial.print(length1); Serial.print(" Words. Button 2 = "); Serial.print(length2); Serial.println(" Words");
    // Initialize MCPWM after capturing signals
  
  }

  if (digitalRead(BUTTON_PIN_1) == LOW) {
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, PWM_PIN);
    pwm_config.frequency = PWM_FREQUENCY;
    pwm_config.cmpr_a = 50.0;    //duty cycle of PWMxA = 50.0%
    pwm_config.cmpr_b = 50.0;    //duty cycle of PWMxb = 50.0%
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 0.0);
    replaySignal(captured1, length1);
    delay(1000); // Delay to avoid multiple triggering
  }

  if (digitalRead(BUTTON_PIN_2) == LOW) {
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, PWM_PIN);
    pwm_config.frequency = PWM_FREQUENCY;
    pwm_config.cmpr_a = 50.0;    //duty cycle of PWMxA = 50.0%
    pwm_config.cmpr_b = 50.0;    //duty cycle of PWMxb = 50.0%
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 0.0);
    
    replaySignal(captured2, length2);
    delay(1000); // Delay to avoid multiple triggering
  }
}

void captureSignal(uint16_t *captured, uint16_t *length) {
  uint16_t i = 0;
  uint32_t currentTime, lastTime = 0;
  uint32_t start;
  bool current;

  digitalWrite(LED, HIGH);
  while (digitalRead(RECV_PIN) == HIGH);
 
  

  current = digitalRead(RECV_PIN);

  
    while (i < CAPTURE_SIZE - 1) {
    start = micros();

    while (digitalRead(RECV_PIN) == current) { // Wait for pin state to change
      if (micros() - start > END_OF_SIGNAL) { // If gap between signals is too long, break
        captured[i++] = micros() - start;
        *length = i;
        digitalWrite(LED, LOW);
        return;
      }
    }

    captured[i++] = micros() - start;
    current = !current; // Change expected pin state
  }
  
  
}

void displayCaptured(uint16_t *captured, uint16_t length) {
  for (uint16_t i = 0; i < length; i++) {
    int count = captured[i] / 100; // Scale the length for display
    for(int j = 0; j < count; j++) {
      if (i % 2 == 0) {
        Serial.print("_");
      } else {
        Serial.print("-");
      }
    }
    if (i % 2 == 0) {
      Serial.print("|");
    } else {
      Serial.print("|");
    }
  }
  Serial.println("");
  delay(1000);
}


void replaySignal(uint16_t *captured, uint16_t length) {
  for (uint16_t i = 0; i < length; i++) {
    if (i % 2 == 0) {
      mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 50.0);
    } else {
      mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 0.0);
    }
    delayMicroseconds(captured[i]);
  }
  mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 0.0);
}
