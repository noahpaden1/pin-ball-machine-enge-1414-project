#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// piezo sensors
#define PIEZO_10_PIN A0
#define PIEZO_25_PIN A1
#define PIEZO_50_PIN A2
#define PIEZO_100_PIN A3

// reset button
#define RESET_BUTTON_PIN 6

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// game variables
int score = 0;
int timeLeft = 60;

// Game automatically starts when Arduino turns on
bool gameRunning = true;

// used for 1-second timer
unsigned long previousMillis = 0;

// used to prevent one hit from registering
// multiple times
unsigned long lastHitTime = 0;

// minimum time between sensor hits
const unsigned long hitCooldown = 200;

// how hard the target must be hit
const int PIEZO_THRESHOLD = 100;


void setup() {

  Wire.begin();

  // -------------------------
  // PIEZO SENSORS
  // -------------------------
  pinMode(PIEZO_10_PIN, INPUT);
  pinMode(PIEZO_25_PIN, INPUT);
  pinMode(PIEZO_50_PIN, INPUT);
  pinMode(PIEZO_100_PIN, INPUT);

  // -------------------------
  // RESET BUTTON
  // -------------------------
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);

  // -------------------------
  // LCD
  // -------------------------
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // -------------------------
  // AUTO START GAME
  // -------------------------
  score = 0;
  timeLeft = 60;
  gameRunning = true;

  // Start the 60-second timer
  previousMillis = millis();

  updateLCD();
}


void loop() {

  // -------------------------
  // RESET BUTTON
  // -------------------------

  int resetButtonState = digitalRead(RESET_BUTTON_PIN);

  if (resetButtonState == LOW) {

    score = 0;
    timeLeft = 60;
    gameRunning = true;

    // Restart timer
    previousMillis = millis();

    updateLCD();

    delay(50);
  }


  // -------------------------
  // PIEZO SENSOR SCORING
  // -------------------------

  if (gameRunning) {

    int piezo10 = analogRead(PIEZO_10_PIN);
    int piezo25 = analogRead(PIEZO_25_PIN);
    int piezo50 = analogRead(PIEZO_50_PIN);
    int piezo100 = analogRead(PIEZO_100_PIN);

    // Prevent multiple scores from one impact
    if (millis() - lastHitTime >= hitCooldown) {

      // -------------------------
      // 10 POINT TARGET
      // -------------------------

      if (piezo10 > PIEZO_THRESHOLD) {

        score += 10;

        lastHitTime = millis();

        updateLCD();
      }

      // -------------------------
      // 25 POINT TARGET
      // -------------------------

      else if (piezo25 > PIEZO_THRESHOLD) {

        score += 25;

        lastHitTime = millis();

        updateLCD();
      }

      // -------------------------
      // 50 POINT TARGET
      // -------------------------

      else if (piezo50 > PIEZO_THRESHOLD) {

        score += 50;

        lastHitTime = millis();

        updateLCD();
      }

      // -------------------------
      // 100 POINT TARGET
      // -------------------------

      else if (piezo100 > PIEZO_THRESHOLD) {

        score += 100;

        lastHitTime = millis();

        updateLCD();
      }
    }
  }


  // -------------------------
  // TIMER
  // -------------------------

  if (gameRunning) {

    unsigned long currentMillis = millis();

    // One second has passed
    if (currentMillis - previousMillis >= 1000) {

      previousMillis += 1000;

      timeLeft--;

      updateLCD();

      // Game is over
      if (timeLeft <= 0) {

        timeLeft = 0;

        gameRunning = false;

        updateLCD();
      }
    }
  }


  // Small delay to prevent excessive processing
  delay(10);
}


// -------------------------
// LCD UPDATE FUNCTION
// -------------------------

void updateLCD() {

  lcd.clear();

  // -------------------------
  // SCORE
  // -------------------------

  lcd.setCursor(0, 0);
  lcd.print("Score:");

  lcd.setCursor(6, 0);
  lcd.print(score);


  // -------------------------
  // TIMER
  // -------------------------

  lcd.setCursor(11, 0);
  lcd.print("T:");

  // Add leading zero
  // Example: T:09
  if (timeLeft < 10) {
    lcd.print("0");
  }

  lcd.print(timeLeft);


  // -------------------------
  // GAME STATUS
  // -------------------------

  if (gameRunning) {

    lcd.setCursor(0, 1);
    lcd.print("GAME RUNNING");

  }

  else {

    lcd.setCursor(0, 1);
    lcd.print("FINAL SCORE:");

    lcd.setCursor(12, 1);
    lcd.print(score);
  }
}