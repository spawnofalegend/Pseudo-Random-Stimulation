#include <Arduino.h>

//-----------------------Pins-------------------------------
const int FSinputPin = 9;        // Input pin for foot switch data
const int outputPin = 2;         // Output pin for stimulus trigger
const int ledPin = LED_BUILTIN;  // Built-in LED for status indication

//-----------------------Parameters-------------------------
const int numEdges = 10;         // Number of heel strikes to average step duration
const int totalbins = 10;        // Number of bins dividing the gait cycle
const int pulseDuration = 100;               // Duration of stimulus pulse (ms)

//-----------------Constants (No need to change)------------
int count = 0;                  // Number of completed full bin rotations
int edgeCount = 0;                            // Counter for detected heel strikes
int bins[totalbins];           // Tracks available bins for randomized selection
int remainingBins = totalbins; // How many bins are left to be selected before a reset
int previousBin = -1;          // Stores the last selected bin

bool trained = false;          // Becomes true once average step duration is learned
bool previousState = LOW;      // Stores the previous state of the foot switch

unsigned long timeDifferences[numEdges - 1];  // Stores step intervals (time between heel strikes)
unsigned long binsize = 0;     // Time duration of one bin (step duration / totalbins)
unsigned long previousTime = 0; // Timestamp of the previous rising edge (heel strike)


//--------------------------SETUP---------------------------
void setup() {
  Serial.begin(9600);             // Start serial communication

  pinMode(FSinputPin, INPUT);     // Foot switch input pin
  pinMode(outputPin, OUTPUT);     // Stimulus output pin
  pinMode(ledPin, OUTPUT);        // LED output for visual cue

  // Initialize bins array with sequential bin indices (0 to totalbins-1)
  for (int i = 0; i < totalbins; i++) {
    bins[i] = i;
  }
}

//---------------------------LOOP---------------------------
void loop() {

  //----------TRAINING PHASE: Measure average step time----------
  while (trained == false) {
    bool currentState = digitalRead(FSinputPin);

    // Detect rising edge: transition from LOW to HIGH
    if (currentState == HIGH && previousState == LOW) {
      unsigned long currentTime = millis(); // Record current time

      if (previousTime != 0 && edgeCount < numEdges - 1) {
        // Calculate time difference between current and previous step
        timeDifferences[edgeCount] = currentTime - previousTime;
        edgeCount++;
      } else if (edgeCount == numEdges - 1) {
        // Once enough steps collected, calculate average step duration
        unsigned long totalTime = 0;
        for (int i = 0; i < numEdges - 1; i++) {
          totalTime += timeDifferences[i];
        }

        unsigned long averageTime = totalTime / (numEdges - 1);
        binsize = averageTime / totalbins;  // Derive duration of one bin
        Serial.print("Average time between ");
        Serial.print(numEdges);
        Serial.print(" steps: ");
        Serial.print(averageTime);
        Serial.println(" ms");

        edgeCount = 0;     // Reset for next phase
        trained = true;    // Move to stimulation phase
      }

      previousTime = currentTime;  // Store current time for next step
    }

    previousState = currentState;  // Update the state tracker
  }


  //----------STIMULATION PHASE----------
  while (trained == true) {
    bool currentState = digitalRead(FSinputPin);

    // On rising edge (heel strike), select a bin and stimulate
    if (currentState == HIGH && previousState == LOW) {
      // Pick random bin index from remainingBins pool
      int randomIndex = random(0, remainingBins);
      int selectedBin = bins[randomIndex];

      // Prevent back-to-back transition from last bin to first (bin 9 → bin 0)
      if (previousBin == totalbins - 1 && selectedBin == 0) {
        while (selectedBin == 0) {
          randomIndex = random(0, remainingBins);
          selectedBin = bins[randomIndex];
        }
      }

      // Remove selected bin from pool by swapping with last unpicked bin
      bins[randomIndex] = bins[remainingBins - 1];
      remainingBins--;

      // Wait for the correct time based on selected bin
      delay(selectedBin * binsize);

      // Trigger stimulation pulse
      digitalWrite(outputPin, HIGH);
      delay(pulseDuration);
      digitalWrite(outputPin, LOW);

      // If all bins used, reset for next round
      if (remainingBins == 0) {
        remainingBins = totalbins;
        Serial.println("rotation complete!");
        count++;

        // Refill bin list
        for (int i = 0; i < totalbins; i++) {
          bins[i] = i;
        }

        // After 20 full rotations, indicate completion
        if (count == 20) {
          Serial.println("20 stim in each bin complete!");
          digitalWrite(ledPin, HIGH); // Turn on LED
        }
      }

      // Save selected bin as previous for next check
      previousBin = selectedBin;
    }

    previousState = currentState; // Update foot switch state
  }
}
