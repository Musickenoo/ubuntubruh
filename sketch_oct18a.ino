#include <EasyUltrasonic.h>

#define TRIGPIN_mid 32 // Digital pin connected to the trig pin of the ultrasonic sensor
#define ECHOPIN_mid 25 // Digital pin connected to the echo pin of the ultrasonic sensor
#define TRIGPIN_right 26 // Digital pin connected to the trig pin of the ultrasonic sensor
#define ECHOPIN_right 27 // Digital pin connected to the echo pin of the ultrasonic sensor
#define OUTPUT_PIN 15 // Pin 15 as output port

EasyUltrasonic ultrasonicM; // Create the ultrasonic object for mid sensor
EasyUltrasonic ultrasonicR; // Create the ultrasonic object for right sensor

void setup() {
  Serial.begin(115200); // Open the serial port
  pinMode(OUTPUT_PIN, OUTPUT); // Set pin 15 as output
  ultrasonicM.attach(TRIGPIN_mid, ECHOPIN_mid); // Attach the mid ultrasonic sensor
  ultrasonicR.attach(TRIGPIN_right, ECHOPIN_right); // Attach the right ultrasonic sensor
}

void loop() {
  float distanceINM = ultrasonicM.getDistanceIN(); // Read the distance in inches from mid sensor
  float distanceINR = ultrasonicR.getDistanceIN(); // Read the distance in inches from right sensor
  float distanceCMM = distanceINM * 2.54; // Convert the distance to centimeters for mid sensor
  float distanceCMR = distanceINR * 2.54; // Convert the distance to centimeters for right sensor

  Serial.print(" cm, Mid: "); // Print label for mid distance
  Serial.print(distanceCMM); // Print distance from mid sensor
  Serial.print(" cm, Right: "); // Print label for right distance
  Serial.print(distanceCMR); // Print distance from right sensor
  Serial.println(" cm"); // New line

  // Check if any distance is less than 20 cm
  if (distanceCMM < 50 || distanceCMR < 20) {
    digitalWrite(OUTPUT_PIN, HIGH); // Set pin 15 to HIGH
  } else {
    digitalWrite(OUTPUT_PIN, LOW); // Set pin 15 to LOW
  }

  delay(100); // Delay for 100 ms
}
