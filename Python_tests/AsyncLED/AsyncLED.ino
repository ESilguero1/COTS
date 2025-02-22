#include <AsyncTask.h>

AsyncTask asyncTask;

float frequency;

void setup() {
  Serial.begin(9600);
  // Initialize pins and frequency
  pinMode(13, OUTPUT);
  pinMode(8, INPUT);
  setFrequency(.5);
}

void loop() {
  // Check and run scheduled tasks
  asyncTask.loop();
   if ((frequency == .5) && (PINB & 1)){
     setFrequency(1);
   }
   else if (frequency == 1 && !(PINB & 1)){
    setFrequency(.5);
   }
}

// Toggles LED
void toggleLED() {
  PINB |= (1<<5);
}

void setFrequency(float freq){
  frequency = freq; // Save frequency in global variable
  freq = ((1/freq)*1000)/2; // Convert frequency to task period
  asyncTask.clearAllTasks();
  asyncTask.repeat(toggleLED, freq); // Create new task for led with given frequency
}