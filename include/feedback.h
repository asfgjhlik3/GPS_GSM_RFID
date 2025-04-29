#include <Arduino.h>
#ifndef FEEDBACK_H
#define FEEDBACK_H

// Define pins
#define GREEN_LED_PIN 12
#define RED_LED_PIN 27
#define YELLOW_LED_PIN 14
#define BUZZER_PIN 26

void setupFeedback() {
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
}

void showSuccessFeedback() {
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100); // short beep
  digitalWrite(BUZZER_PIN, LOW);
  delay(500);
  digitalWrite(GREEN_LED_PIN, LOW);
}

void showFailFeedback() {
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(3000); // long beep (3 seconds)
  digitalWrite(BUZZER_PIN, LOW);
  delay(500);
  digitalWrite(RED_LED_PIN, LOW);
}

void setStandbyMode() {
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, HIGH);
}

void clearStandbyMode() {
  digitalWrite(YELLOW_LED_PIN, LOW);
}

#endif
