#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>
#include <string.h>
#include <stdlib.h>

// piezo sensors
#define PIEZO_10_PIN A0
#define PIEZO_25_PIN A1
#define PIEZO_50_PIN A2
#define PIEZO_100_PIN A3

// button pin assignments
#define LEFT_BUTTON      3
#define DOWN_BUTTON      4
#define RIGHT_BUTTON     5
#define UP_BUTTON        7
#define SELECT_BUTTON    8

// SD card
#define SD_CS_PIN 10

// LCD
LiquidCrystal_I2C lcd(0x27, 20, 4);

// button debounce
const unsigned long debounceDelay = 50;

int lastReadingLeft = HIGH;
int stableStateLeft = HIGH;
unsigned long debounceTimeLeft = 0;

int lastReadingDown = HIGH;
int stableStateDown = HIGH;
unsigned long debounceTimeDown = 0;

int lastReadingRight = HIGH;
int stableStateRight = HIGH;
unsigned long debounceTimeRight = 0;

int lastReadingUp = HIGH;
int stableStateUp = HIGH;
unsigned long debounceTimeUp = 0;

int lastReadingSelect = HIGH;
int stableStateSelect = HIGH;
unsigned long debounceTimeSelect = 0;

// nickname settings
char nickname[4] = {'_', '_', '_', '\0'};
int nicknameLength = 0;
int selectedLetter = 0;  // A = 0, Z = 25

// game variables

int score = 0;
int timeLeft = 60;

// game no longer automatically starts when arduino turns on
bool gameRunning = false;
bool gameFinished = false;

// used for 1-second timer
unsigned long previousMillis = 0;

// used to prevent one hit from registering
// multiple times
unsigned long lastHitTime = 0;

// minimum time between sensor hits
const unsigned long hitCooldown = 200;

// how hard the target must be hit
const int PIEZO_THRESHOLD = 100;

// -------------------------
// GAME OVER AND LEADERBOARD SCREEN
// -------------------------
bool showingLeaderboard = false;                 
unsigned long gameOverTime = 0;                   
const unsigned long leaderboardDelay = 5000;     

// sd card 
const char SCORE_FILE[] = "SCORES.TXT";
const int TOP_N = 5;

bool sdOk = false;

unsigned long topScore[TOP_N];
char topNickname[TOP_N][4];


bool buttonPressed(
  int buttonPin,
  int &lastReading,
  int &stableState,
  unsigned long &debounceTime
);

int getRow(int letter);
int getColumn(int letter);

void displayNicknameEntry();
void updateLCD();
void startGame();
void endGame();
void resetToNicknameEntry();
void showLeaderboard();
void saveScore();
void loadTopFive();

void insertTop(
  unsigned long sc,
  const char* nick
);

unsigned long getTopScore(int rank);

// -------------------------
// SETUP
// -------------------------
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
  // BUTTONS 
  // -------------------------
  pinMode(LEFT_BUTTON, INPUT_PULLUP);
  pinMode(DOWN_BUTTON, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON, INPUT_PULLUP);
  pinMode(UP_BUTTON, INPUT_PULLUP);
  pinMode(SELECT_BUTTON, INPUT_PULLUP);
  
  // -------------------------
  // LCD 
  // -------------------------
  lcd.init();     
  lcd.backlight();
  lcd.display();    
  lcd.clear();      

  // -------------------------
  // SD CARD
  // -------------------------
  sdOk = SD.begin(SD_CS_PIN);  // Try to start the SD card, reads true if found

  if (sdOk) {
    loadTopFive();   
  }
  score = 0;           
  timeLeft = 60;        
  gameRunning = false;   
  gameFinished = false;  

  displayNicknameEntry();  // Show the name-entry screen
}


void loop() {

  // -------------------------
  // select button during game ends the game early
  // -------------------------
  if (gameRunning) {
    if (buttonPressed(
          SELECT_BUTTON,
          lastReadingSelect,
          stableStateSelect,
          debounceTimeSelect)) {

      endGame();    
      updateLCD(); 
    }
  }

 
  if (gameFinished) {
    bool selectPressed = buttonPressed(
        SELECT_BUTTON,
        lastReadingSelect,
        stableStateSelect,
        debounceTimeSelect);

    if (!showingLeaderboard) {
      if (millis() - gameOverTime >= leaderboardDelay) {
        showingLeaderboard = true; 
        showLeaderboard();      
      }
    } else if (selectPressed) {
      resetToNicknameEntry(); 
    }
  }
  
  if (!gameRunning && !gameFinished) {

    // LEFT: previous letter
    if (buttonPressed(
          LEFT_BUTTON,
          lastReadingLeft,
          stableStateLeft,
          debounceTimeLeft)) {

      selectedLetter = (selectedLetter + 25) % 26;  // Go back one (wraps A -> Z)
      displayNicknameEntry();                       // Redraw the highlight
    }

    // DOWN: move to row below
    if (buttonPressed(
          DOWN_BUTTON,
          lastReadingDown,
          stableStateDown,
          debounceTimeDown)) {

      int row = getRow(selectedLetter);     // Which row the cursor is on
      int col = getColumn(selectedLetter);  // Which column it is in

      if (row == 0) {
        selectedLetter = 9 + col;        // Row 1 (A-I) goes to Row 2 (J-R)
      } else if (row == 1) {
        selectedLetter = 18 + (col % 8); // Row 2 goes to Row 3 (S-Z has only 8 letters)
      } else {
        selectedLetter = col % 9;        // Row 3 wraps back to Row 1
      }

      displayNicknameEntry();  // Redraw the highlight
    }

    // RIGHT: next letter
    if (buttonPressed(
          RIGHT_BUTTON,
          lastReadingRight,
          stableStateRight,
          debounceTimeRight)) {

      selectedLetter = (selectedLetter + 1) % 26;  // Go forward one (wraps Z -> A)
      displayNicknameEntry();                      // Redraw the highlight
    }

    // UP: move to row above
    if (buttonPressed(
          UP_BUTTON,
          lastReadingUp,
          stableStateUp,
          debounceTimeUp)) {

      int row = getRow(selectedLetter);     // Which row the cursor is on
      int col = getColumn(selectedLetter);  // Which column it is in

      if (row == 2) {
        selectedLetter = 9 + col;        // Row 3 goes to Row 2
      } else if (row == 1) {
        selectedLetter = col;            // Row 2 goes to Row 1
      } else {
        selectedLetter = 18 + (col % 8); // Row 1 wraps up to Row 3
      }

      displayNicknameEntry();  // Redraw the highlight
    }

    // SELECT: confirm the current letter
    if (buttonPressed(
          SELECT_BUTTON,
          lastReadingSelect,
          stableStateSelect,
          debounceTimeSelect)) {

      nickname[nicknameLength] =
          (char)('A' + selectedLetter);  // Store the chosen letter

      nicknameLength++;                  // Move to the next slot
      nickname[nicknameLength] = '\0';   // Keep the string properly ended

      if (nicknameLength >= 3) {
        startGame();  // Name is complete, so begin the game
      } else {
        selectedLetter = 0;      // Cursor back to A for the next letter
        displayNicknameEntry();  // Redraw the screen
      }
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
        endGame();
        updateLCD();
      }
    }
  }


  // Small delay to prevent excessive processing
  delay(10);
}

// -------------------------
// BUTTON DEBOUNCE FUNCTION
// Returns true once each time the button is pressed
// -------------------------
bool buttonPressed(
  int buttonPin,
  int &lastReading,
  int &stableState,
  unsigned long &debounceTime
) {
  int reading = digitalRead(buttonPin);

  if (reading != lastReading) {
    debounceTime = millis();
    lastReading = reading;
  }

  if (millis() - debounceTime > debounceDelay) { 
    if (reading != stableState) {
      stableState = reading;
      if (stableState == LOW) {
        return true;  // LOW means pressed, so report a press
      }
    }
  }

  return false;  // No new press
}

// -------------------------
// ALPHABET ROW AND COLUMN
// -------------------------
int getRow(int letter) {
  if (letter < 9) return 0;   // A-I are on row 0
  if (letter < 18) return 1;  // J-R are on row 1
  return 2;                   // S-Z are on row 2
}

int getColumn(int letter) {
  if (letter < 9) return letter;        // Row 0: column = letter number
  if (letter < 18) return letter - 9;   // Row 1: subtract the 9 letters before it
  return letter - 18;                   // Row 2: subtract the 18 letters before it
}

// -------------------------
// DISPLAY NICKNAME ENTRY
// -------------------------
void displayNicknameEntry() {
  lcd.clear();  // Wipe the screen before redrawing

  // Row 0: nickname
  lcd.setCursor(0, 0);       
  lcd.print("NAME:");       
  lcd.print(nickname);      
  lcd.print(" ");            
  lcd.print(nicknameLength); 
  lcd.print("/3");           

  // Row 1: A-I
  lcd.setCursor(0, 1);
  lcd.print(" A B C D E F G H I");

  // Row 2: J-R
  lcd.setCursor(0, 2);
  lcd.print(" J K L M N O P Q R");

  // Row 3: S-Z
  lcd.setCursor(0, 3);
  lcd.print(" S T U V W X Y Z");

  // Highlight the selected letter
  int row = getRow(selectedLetter) + 1;
  int col = getColumn(selectedLetter);

  // Each letter is two positions apart
  lcd.setCursor(col * 2, row);               
  lcd.print('|');                             // Left bracket
  lcd.print((char)('A' + selectedLetter));    // The letter itself
  lcd.print('|');                             // Right bracket
}

// -------------------------
// RESET TO NICKNAME ENTRY
// Clears everything and returns to the name-entry screen
// -------------------------
void resetToNicknameEntry() {
  nickname[0] = '_';  // Clear first letter
  nickname[1] = '_';  // Clear second letter
  nickname[2] = '_';  // Clear third letter
  nickname[3] = '\0'; // End of string

  nicknameLength = 0;  // No letters chosen yet
  selectedLetter = 0;  // Cursor back to 'A'

  score = 0;                  // Clear the score
  timeLeft = 60;              // Reset the timer
  gameFinished = false;       // Leave the "game over" state
  showingLeaderboard = false; // Leave the leaderboard screen

  displayNicknameEntry();  // Show the name-entry screen
}

// -------------------------
// START GAME
// -------------------------
void startGame() {
  score = 0;             // Start from zero points
  timeLeft = 60;         // Full 60 seconds

  // Countdown: 3, 2, 1
  for (int i = 3; i >= 1; i--) {
    lcd.clear();
    lcd.setCursor(3, 1);
    lcd.print("GAME STARTS IN");
    lcd.setCursor(9, 2);
    lcd.print(i);                
    delay(1000); 
  }

  
  lcd.clear();
  lcd.setCursor(8, 1);
  lcd.print("GO!");
  delay(700);

  gameRunning = true;
  gameFinished = false;
  showingLeaderboard = false;

  previousMillis = millis();
  lastHitTime = 0;

  updateLCD();
}

// -------------------------
// END GAME
// -------------------------
void endGame() {
  gameRunning = false;
  gameFinished = true;
  showingLeaderboard = false;
  gameOverTime = millis();

  saveScore();
}

// -------------------------
// UPDATE GAME LCD
// -------------------------
void updateLCD() {
  lcd.clear();

  // Row 0: nickname
  lcd.setCursor(0, 0);
  lcd.print("NAME: ");
  lcd.print(nickname);  // Show the player's 3 letters

  // Row 1: score
  lcd.setCursor(0, 1);
  lcd.print("SCORE: ");
  lcd.print(score);  // Show the current score

  // Row 2: timer
  lcd.setCursor(0, 2);
  lcd.print("TIME LEFT: ");
  if (timeLeft < 10) {
    lcd.print('0');
  }
  lcd.print(timeLeft);
  lcd.print(" sec");

  // Row 3: game status
  lcd.setCursor(0, 3);
  if (gameRunning) {
    lcd.print("GAME RUNNING");
  }
  else if (gameFinished) {
    lcd.print("GAME OVER!");
  }
  else {
    lcd.print("ENTER NICKNAME");
  }
}

// -------------------------
// SHOW LEADERBOARD
// -------------------------
void showLeaderboard() {
  // Reload the saved scores so the leaderboard reflects the SD card contents.
  if (sdOk) {
    loadTopFive();
  }

  lcd.clear();


  for (int rank = 1; rank <= 4; rank++) {
    lcd.setCursor(0, rank - 1);
    lcd.print(rank);
    lcd.print(".");

    if (getTopScore(rank) > 0) {
      lcd.print(topNickname[rank - 1]);
      lcd.print(" ");
      lcd.print(getTopScore(rank));
    } else {
      lcd.print("---");
    }
  }

  lcd.setCursor(13, 0);
  lcd.print("PRESS");
  lcd.setCursor(13, 1);
  lcd.print("SELECT");
  lcd.setCursor(13, 2);
  lcd.print("TO");
  lcd.setCursor(13, 3);
  lcd.print("START");
}

// -------------------------
// SAVE FINAL SCORE
// Format: nickname,score
// -------------------------
void saveScore() {
  if (!sdOk) {
    return; 
  }

  File f = SD.open(SCORE_FILE, FILE_WRITE);
  if (f) {
    f.print(nickname); 
    f.print(",");
    f.println(score);
    f.close();

    insertTop(score, nickname);
  }
}

// -------------------------
// LOAD TOP FIVE AT BOOT
// -------------------------
void loadTopFive() {
  for (int i = 0; i < TOP_N; i++) {
    topScore[i] = 0;              
    strcpy(topNickname[i], "---");
  }

  File f = SD.open(SCORE_FILE, FILE_READ);

  if (!f) {
    return;  // No file means no scores to load
  }

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();                           

    int comma = line.indexOf(',');
    if (comma < 0) continue;

    String nickString = line.substring(0, comma);
    nickString.trim();

    unsigned long savedScore =
      strtoul(line.substring(comma + 1).c_str(), NULL, 10);

    char savedNick[4] = {'_', '_', '_', '\0'};

    for (int i = 0; i < 3 && i < nickString.length(); i++) {
      savedNick[i] = nickString[i];
    }

    insertTop(savedScore, savedNick);
  }

  f.close();
}
// -------------------------
// KEEP TOP FIVE SORTED
// -------------------------
void insertTop(
  unsigned long sc,
  const char* nick
) {
  int pos = TOP_N;
  for (int i = 0; i < TOP_N; i++) {
    if (sc > topScore[i]) {
      pos = i;
      break;
    }
  }
  if (pos == TOP_N) return;
  for (int i = TOP_N - 1; i > pos; i--) {
    topScore[i] = topScore[i - 1];
    strcpy(topNickname[i], topNickname[i - 1]);
  }
  topScore[pos] = sc;
  for (int i = 0; i < 3; i++) {
    topNickname[pos][i] = nick[i];
  }
  topNickname[pos][3] = '\0';
}
// -------------------------
// LEADERBOARD INDEXING
// -------------------------
unsigned long getTopScore(int rank) {
  if (rank < 1 || rank > TOP_N) {
    return 0;
  }
  return topScore[rank - 1];
}
