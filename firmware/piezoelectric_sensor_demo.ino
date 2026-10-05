const int piezoPin = A0;
const int threshold = 50;

void setup() {
  Serial.begin(9600);
}

void loop() {

  int value = analogRead(piezoPin);
  Serial.println(value);

  if (value > threshold) {
    Serial.println("HIT!");
    delay(50);
  }
}