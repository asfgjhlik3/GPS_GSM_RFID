#ifndef FEEDBACK_H
#define FEEDBACK_H

#define GREEN_LED_PIN 12
#define RED_LED_PIN 27
#define YELLOW_LED_PIN 14
#define BUZZER_PIN 26

void setupFeedback();
void showSuccessFeedback();
void showFailFeedback();
void setStandbyMode();
void clearStandbyMode();

#endif
