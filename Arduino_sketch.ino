#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD I2C address (typically 0x27 or 0x3F, change if needed)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Ultrasonic Sensor 1 pins
const int trigPin1 = 2;  // D2
const int echoPin1 = 7;  // D7

// Ultrasonic Sensor 2 pins
const int trigPin2 = 5;  // D5
const int echoPin2 = 9;  // D9

// Variables for counting
int inCount = 0;
int outCount = 0;
int totalCount = 0;

const int buzzerPin = 12;

// Variables for state tracking
bool sensor1Triggered = false;
bool sensor2Triggered = false;
bool sensor1First = false;
bool sensor2First = false;
bool countingComplete = true;

// Threshold for detection (in cm)
const int detectionThreshold = 90;  // Adjust based on your doorway width default = 50

// Timing variables
unsigned long lastDetectionTime = 0;
const unsigned long resetDelay = 3000;  // 3 second timeout
const unsigned long debounceDelay = 150; // Time between distance readings default = 200

void scrollMessage(const char* msg) {
  int len = strlen(msg);
  for (int i = 0; i < len - 15; i++) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(msg + i);
    delay(300);
  }
}

void setup() {
  // Initialize LCD
  lcd.init();
  lcd.backlight();

  scrollMessage("2DIRECTIONAL VISITOR COUNTER");
  
  // Initialize ultrasonic sensor pins
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
  
  // Initialize Serial for debugging
  Serial.begin(9600);
  Serial.println("Bidirectional Visitor Counter with Ultrasonic Sensors Initialized");
  
  // Initial LCD display
  updateDisplay();
}

void loop() {
  // Get current time
  unsigned long currentTime = millis();
  
  // Measure distances from both sensors
  int distance1 = getDistance(trigPin1, echoPin1);
  delay(10); // Small delay between sensor readings
  int distance2 = getDistance(trigPin2, echoPin2);
  
  // Print distances for debugging
  if (currentTime % 500 == 0) { // Print every 500ms to avoid flooding serial
    Serial.print("Distance 1: ");
    Serial.print(distance1);
    Serial.print(" cm, Distance 2: ");
    Serial.print(distance2);
    Serial.println(" cm");
  }
  
  // Check for timeout to reset the sequence
  if ((currentTime - lastDetectionTime > resetDelay) && 
      (sensor1First || sensor2First) && 
      !countingComplete) {
    Serial.println("Timeout - Sequence Reset");
    sensor1First = false;
    sensor2First = false;
    countingComplete = true;
  }
  
  // Detect object with Sensor 1
  bool currentSensor1 = (distance1 < detectionThreshold);
  if (currentSensor1 && !sensor1Triggered && countingComplete) {
    // Sensor 1 triggered first
    Serial.println("Sensor 1 Triggered First");
    sensor1First = true;
    sensor2First = false;
    countingComplete = false;
    lastDetectionTime = currentTime;
    delay(debounceDelay);
  }
  else if (currentSensor1 && !sensor1Triggered && sensor2First) {
    // Sensor 1 triggered after Sensor 2: Person exiting
    Serial.println("Person Exited (S2 then S1)");
    outCount++;
    totalCount--;
    
    if (totalCount == 0) {
      soundBuzzer(2);
    }

    if (totalCount < 0) totalCount = 0;
    
    // Reset sequence
    sensor1First = false;
    sensor2First = false;
    countingComplete = true;
    
    updateDisplay();
    delay(debounceDelay);
  }
  
  // Detect object with Sensor 2
  bool currentSensor2 = (distance2 < detectionThreshold);
  if (currentSensor2 && !sensor2Triggered && countingComplete) {
    // Sensor 2 triggered first
    Serial.println("Sensor 2 Triggered First");
    sensor2First = true;
    sensor1First = false;
    countingComplete = false;
    lastDetectionTime = currentTime;
    delay(debounceDelay);
  }
  else if (currentSensor2 && !sensor2Triggered && sensor1First) {
    // Sensor 2 triggered after Sensor 1: Person entering
    Serial.println("Person Entered (S1 then S2)");
    inCount++;
    
    bool wasEmpty = (totalCount == 0);
    totalCount++;
    
    if (wasEmpty){
      soundBuzzer(1);
    }
    // Reset sequence
    sensor1First = false;
    sensor2First = false;
    countingComplete = true;
    
    updateDisplay();
    delay(debounceDelay);
  }
  
  // Update sensor states
  sensor1Triggered = currentSensor1;
  sensor2Triggered = currentSensor2;
  
  // Small delay to stabilize readings
  delay(50);
}

// Function to measure distance using ultrasonic sensor
int getDistance(int trigPin, int echoPin) {
  // Clear the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Set the trigPin HIGH for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Read the echoPin (returns the sound wave travel time in microseconds)
  long duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance in centimeters
  // Speed of sound is 340 m/s or 0.034 cm/Âµs
  // Time is divided by 2 (to account for travel to and from the object)
  int distance = duration * 0.034 / 2;
  
  return distance;
}

void soundBuzzer(int pattern) {
  switch (pattern) {
    case 1:
      tone(buzzerPin, 1000);
      delay(500);
      noTone(buzzerPin);
      Serial.println("Buzzer: Someone entered empty room");
      break;
  
    case 2:
      for (int i = 0; i < 2; i++) {
        tone(buzzerPin, 1000);
        delay(200);
        noTone(buzzerPin);
        delay(200);
      }
      Serial.println("Buzzer: Last Person left");
      break;
    
    default:
      break;
  }
}

void updateDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IN: ");
  lcd.print(inCount);
  lcd.print(" OUT: ");
  lcd.print(outCount);
  
  lcd.setCursor(0, 1);
  lcd.print("TOTAL: ");
  lcd.print(totalCount);
  
  // Debug info
  Serial.print("IN: ");
  Serial.print(inCount);
  Serial.print(" OUT: ");
  Serial.print(outCount);
  Serial.print(" TOTAL: ");
  Serial.println(totalCount);
}