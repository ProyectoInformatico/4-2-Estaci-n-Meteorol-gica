const int PINTEMP = A2;
const int PINBUZZER = 8;
const int PINLED = A3;

float leerTemp() {
  float voltage = analogRead(PINTEMP) * (5.0 / 1023.0);
  return (voltage - 0.5) * 100.0;
}

void setup() {
  pinMode(PINBUZZER, OUTPUT);
  pinMode(PINLED, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  float tempC = leerTemp();

  if (tempC > 38.0 || tempC < 3.0) {
    digitalWrite(PINLED, HIGH);
    tone(PINBUZZER, 3000, 200);
    delay(300);
    tone(PINBUZZER, 2500, 200);
    delay(500);
  } else {
    digitalWrite(PINLED, LOW);
    noTone(PINBUZZER);
    delay(500);
  }

  Serial.println(tempC);
}





