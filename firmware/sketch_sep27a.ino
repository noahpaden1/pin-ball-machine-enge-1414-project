// this is just a prototype and is not fully functional yet
// it includes code for a reset button, 4 buttons representing point scoring components, and the lcd screen int

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// btn pin def
#define BUTTON_10_PIN 5
#define BUTTON_25_PIN 7
#define BUTTON_50_PIN 8
#define BUTTON_100_PIN 9
#define RESET_BUTTON_PIN 6

// lcd object
LiquidCrystal_I2C lcd(0x27, 16, 2);

// game var init
int score = 0;
int timeLeft = 60;
bool gameRunning = false;

// used for 1s timer
unsigned long previousMillis = 0;

// init btn states
int lastButton10State = HIGH;
int lastButton25State = HIGH;
int lastButton50State = HIGH;
int lastButton100State = HIGH;
int lastResetButtonState = HIGH;

int resetPressCount = 0;

void setup() {
  Wire.begin();
  // intiate btn pins
  pinMode(BUTTON_10_PIN, INPUT_PULLUP);
  pinMode(BUTTON_25_PIN, INPUT_PULLUP);
  pinMode(BUTTON_50_PIN, INPUT_PULLUP);
  pinMode(BUTTON_100_PIN, INPUT_PULLUP);
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  // initiate lcd
  lcd.init();
  lcd.backlight();
  lcd.clear();
  updateLCD();
}

void loop() {
  // reading logic for reset btn and pressure activated point scoring sensors (also btns)
  int button10State = digitalRead(BUTTON_10_PIN);
  int button25State = digitalRead(BUTTON_25_PIN);
  int button50State = digitalRead(BUTTON_50_PIN);
  int button100State = digitalRead(BUTTON_100_PIN);
  int resetButtonState = digitalRead(RESET_BUTTON_PIN);
  // if statement handling reset btn logic
  if (resetButtonState == LOW && lastResetButtonState == HIGH) {
    resetPressCount++;
    // Second press reset the game
    if(resetPressCount == 2){
    // variable reset for when new game started
      timeLeft = 60;
      gameRunning = false;

      updateLCD();
    }
      
    // Third press starts new game
    if (resetPressCount == 3){
      score = 0;
      gameRunning=true;
      previousMillis = millis();
      updateLCD();
      resetPressCount = 0;
    }
    delay(50);
  }
  if (gameRunning) {
    // +10 btn logic
    if (button10State == LOW && lastButton10State == HIGH) {
      score += 10;
      updateLCD();
    }
    // +25 btn logic
    if (button25State == LOW && lastButton25State == HIGH) {
      score += 25;
      updateLCD();
    }
    // +50 btn logic
    if (button50State == LOW && lastButton50State == HIGH) {
      score += 50;
      updateLCD();
    }
    // +100 btn logic
    if (button100State == LOW && lastButton100State == HIGH) {
      score += 100;
      updateLCD();
    }
  }
  // lcd timer update logic
  if (gameRunning) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 1000) {
      previousMillis += 1000;
      timeLeft--;
      updateLCD();
      if (timeLeft <= 0) {
        timeLeft = 0;
        gameRunning = false;
        updateLCD();
      }
    }
  }
  // storing last collected state for all btns in circuit (including reset btn)
  lastButton10State = button10State;
  lastButton25State = button25State;
  lastButton50State = button50State;
  lastButton100State = button100State;
  lastResetButtonState = resetButtonState;
  delay(50);
}

// primary lcd update logic
void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Score:");
  lcd.setCursor(6, 0);
  lcd.print(score);
  lcd.setCursor(11, 0);
  lcd.print("T:");
  if (timeLeft < 10) {
    lcd.print("0");
  }
  lcd.print(timeLeft);
  if (gameRunning) {
    lcd.setCursor(0, 1);
    lcd.print("GAME RUNNING");
  } else {
    lcd.setCursor(0, 1);
    lcd.print("FINAL SCORE:");
    lcd.setCursor(12, 1);
    lcd.print(score);
  }
}