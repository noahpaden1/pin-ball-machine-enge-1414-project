#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>

// piezo sensors
#define PIEZO_10_PIN A0
#define PIEZO_25_PIN A1
#define PIEZO_50_PIN A2
#define PIEZO_100_PIN A3

// reset button
#define RESET_BUTTON_PIN 6

// SD card
#define SD_CS_PIN 10

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// game variables
unsigned long score = 0;
int timeLeft = 60;

// Game automatically starts when Arduino turns on
bool gameRunning = true;

// used for 1-second timer
unsigned long previousMillis = 0;

// used to prevent one hit from registering
// multiple times
unsigned long lastHitTime = 0;
const unsigned long hitCooldown = 200;

// minimum time between sensor hits
const int PIEZO_THRESHOLD = 100;

// SD card and leaderboard
const char SCORE_FILE[] = "SCORES.TXT";
const int TOP_N = 5;

bool sdOk = false;
unsigned long gameCount = 0;

unsigned long topScore[TOP_N];
unsigned long topGame[TOP_N];

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
  // SERIAL AND SD CARD
  // -------------------------
  Serial.begin(9600);

  sdOk = SD.begin(SD_CS_PIN);

  if (sdOk) {
    Serial.println("SD card ready.");
    loadTopFive();
    printTopFive();
  }
  else {
    Serial.println("SD card not found. Scores won't save.");
  }

  // -------------------------
  // AUTO START GAME
  // -------------------------
  score = 0;
  timeLeft = 60;
  gameRunning = true;

  // Start the 60-second timer
  previousMillis = millis();

  updateLCD();

  Serial.println("Commands:");
  Serial.println("t = show top five");
  Serial.println("r = show all saved records");
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
    previousMillis = millis();

    updateLCD();

    // Prevent repeated resets while button is held
    while (digitalRead(RESET_BUTTON_PIN) == LOW) {
      delay(10);
    }
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
      // 10 point target
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

        saveScore();
        updateLCD();
      }
    }
  }

  // -------------------------
  // TOP FIVE ACCESS
  // -------------------------
  if (Serial.available()) {
    char c = Serial.read();

    if (c == 't' || c == 'T') {
      printTopFive();
    }
    else if (c == 'r' || c == 'R') {
      readAllRecords();
    }
  }

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

// -------------------------
// SAVE THE FINAL SCORE
// -------------------------

void saveScore() {
  unsigned long thisGame = gameCount + 1;

  if (sdOk) {
    File f = SD.open(SCORE_FILE, FILE_WRITE);

    if (f) {
      f.print(thisGame);
      f.print(",");
      f.println(score);
      f.close();

      Serial.print("Saved game ");
      Serial.print(thisGame);
      Serial.print(", score: ");
      Serial.println(score);

      insertTop(score, thisGame);
      gameCount++;
    }
    else {
      Serial.println("Could not open score file.");
    }
  }
}

// -------------------------
// LOAD THE TOP FIVE AT BOOT
// -------------------------

void loadTopFive() {
  for (int i = 0; i < TOP_N; i++) {
    topScore[i] = 0;
    topGame[i] = 0;
  }

  gameCount = 0;

  File f = SD.open(SCORE_FILE, FILE_READ);

  // if the file cannot be opened function is not executed
  if (!f) {
    Serial.println("No saved scores yet.");
    return;
  }

  // repeatedly runs until the file has been completely read
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();

    // searches the text for the first comma and saves its position in the variable comma
    int comma = line.indexOf(',');

    if (comma < 0) {
      continue;
    }
    // converts game number and converts it from text into an unsigned long integer
    unsigned long gameNo =
        strtoul(line.substring(0, comma).c_str(), NULL, 10);
    // converts the score into an unsigned long number
    unsigned long savedScore =
        strtoul(line.substring(comma + 1).c_str(), NULL, 10);

    insertTop(savedScore, gameNo);

    if (gameNo > gameCount) {
      gameCount = gameNo;
    }
  }

  f.close();

  Serial.print("Games loaded: ");
  Serial.println(gameCount);
}

// -------------------------
// KEEP THE TOP FIVE SORTED
// -------------------------

void insertTop(unsigned long sc, unsigned long gameNo) {
  int pos = TOP_N;
  // loop that checks every position in the leaderboard to find the correct position for the new score
  for (int i = 0; i < TOP_N; i++) {
    if (sc > topScore[i]) {
      pos = i;
      break;
    }
  }
  // if the loop fails to find a position function is exited immediately
  if (pos == TOP_N) {
    return;
  }

  for (int i = TOP_N - 1; i > pos; i--) {
    topScore[i] = topScore[i - 1]; // copies the score from the position above into the current position
    topGame[i] = topGame[i - 1]; // does the same thing for the game number
  }

  topScore[pos] = sc;
  topGame[pos] = gameNo;
}

// -------------------------
// INDEXING FUNCTIONS
// -------------------------

unsigned long getTopScore(int rank) {
  if (rank < 1 || rank > TOP_N) {
    return 0;
  }

  return topScore[rank - 1];
}

unsigned long getTopGame(int rank) {
  if (rank < 1 || rank > TOP_N) {
    return 0;
  }

  return topGame[rank - 1];
}

// -------------------------
// SHOW THE TOP FIVE
// -------------------------

void printTopFive() {
  Serial.println("--- TOP 5 ---");

  //repeats the loop five times, once for each position on the leaderboard
  for (int rank = 1; rank <= TOP_N; rank++) {
    Serial.print(rank);
    Serial.print(". ");

    // checks whether the score exists
    if (getTopScore(rank) > 0) {
      Serial.print(getTopScore(rank));
      Serial.print(" (game ");
      Serial.print(getTopGame(rank));
      Serial.println(")");
    }
    else {
      Serial.println("---");
    }
  }
}

// -------------------------
// READ ALL SAVED RECORDS
// -------------------------

void readAllRecords() {
  File f = SD.open(SCORE_FILE, FILE_READ);

  if (!f) {
    Serial.println("No saved records found.");
    return;
  }

  Serial.println("--- ALL SAVED RECORDS ---");
  

  while (f.available()) {
    String line = f.readStringUntil('\n'); // reads the characters from the file until it reaches a newline (\n)
    line.trim();

    int comma = line.indexOf(',');

    // prevents incorrectly formatted lines from being processed
    if (comma < 0) {
      continue;
    }

    unsigned long gameNo =
        strtoul(line.substring(0, comma).c_str(), NULL, 10);

    unsigned long savedScore =
        strtoul(line.substring(comma + 1).c_str(), NULL, 10);

    Serial.print("Game ");
    Serial.print(gameNo);
    Serial.print(": Score ");
    Serial.println(savedScore);
  }

  f.close();
}