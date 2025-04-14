#include <Arduino.h>

const int FSinputPin = 9; // Digital input pin
const int numEdges = 10; // Number of rising edges to calculate the average time
const int outputPin = 2;
const int BUTinputPin = 8;
const int ledPin = LED_BUILTIN;

unsigned long previousTime = 0; // Variable to store the time of the previous rising edge
bool previousState = LOW; // Variable to store the previous state of the input pin
unsigned long timeDifferences[numEdges - 1]; // Array to store time differences
int edgeCount = 0; // Counter for the number of rising edges detected
const int totalbins = 10;
bool trained = false;
const int pulseDuration = 100; // Pulse duration in milliseconds
unsigned long binsize = 0;
bool prevbutstate = LOW;
int count = 0;

int bins[totalbins]; // Array to track bins
int remainingBins = totalbins; // Counter for remaining bins to be selected
int previousBin = -1;  // Variable to store the previously selected bin

void setup() {
  // Initialize the Serial monitor
  Serial.begin(9600);

  // Initialize the input pin
  pinMode(FSinputPin, INPUT);
  pinMode(BUTinputPin, INPUT);

  pinMode(outputPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  // Initialize the bins array
  for (int i = 0; i < totalbins; i++) {
    bins[i] = i;
  }
}

void loop() {

  bool butcurrentState = digitalRead(BUTinputPin);
  Serial.print("pressed");
  
  while (trained == false) {

    bool currentState = digitalRead(FSinputPin);
    // Check for a rising edge (low to high transition)
    if (currentState == HIGH && previousState == LOW) {
      unsigned long currentTime = millis(); // Record the current time
      if (previousTime != 0 && edgeCount < numEdges - 1) { // Check if this is not the first rising edge
        timeDifferences[edgeCount] = currentTime - previousTime; // Calculate and store the time difference
        edgeCount++; // Increment the edge counter
      } else if (edgeCount == numEdges - 1) { // If the required number of edges is reached
        // Calculate the average time difference
        unsigned long totalTime = 0;
        for (int i = 0; i < numEdges - 1; i++) {
          totalTime += timeDifferences[i];
        }
        unsigned long averageTime = totalTime / (numEdges - 1);
        binsize = averageTime / totalbins;
        Serial.print("Average time between ");
        Serial.print(numEdges);
        Serial.print(" steps: ");
        Serial.print(averageTime);
        Serial.println(" ms");

        // Reset the edge counter
        edgeCount = 0;
        trained = true;
      }

      previousTime = currentTime; // Update the previous time
    }

    // Update the previous state
    previousState = currentState;
  }
  
  prevbutstate = butcurrentState;

  while (trained == true) {

    bool currentState = digitalRead(FSinputPin);

    if (currentState == HIGH && previousState == LOW) {
      // Select a random index from the remaining bins
      int randomIndex = random(0, remainingBins);
      int selectedBin = bins[randomIndex];

      // Ensure the next bin is not 0 if the last selected bin was the final bin
      if (previousBin == totalbins - 1 && selectedBin == 0) {
        // Re-roll until the selected bin is not 0
        while (selectedBin == 0) {
          randomIndex = random(0, remainingBins);
          selectedBin = bins[randomIndex];
        }
      }

      // Move the last element to the selected spot to avoid duplicates
      bins[randomIndex] = bins[remainingBins - 1];
      remainingBins--;

      // Delay based on the selected bin
      delay(selectedBin * binsize);
      digitalWrite(outputPin, HIGH);
      delay(pulseDuration);
      digitalWrite(outputPin, LOW);

      // Reset bins if all have been picked
      if (remainingBins == 0) {
        remainingBins = totalbins;
        Serial.println("rotation complete!");
        count++;

        for (int i = 0; i < totalbins; i++) {
          bins[i] = i;
        }

        if (count == 20) {
          Serial.println("20 stim in each bin complete!");
          digitalWrite(ledPin, HIGH);
        }
      }

      // Store the current bin as the previous bin for the next iteration
      previousBin = selectedBin;
    }

    previousState = currentState;
  }
}