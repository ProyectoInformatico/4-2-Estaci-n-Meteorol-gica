int rele = 7;

void setup() {
  pinMode(rele, OUTPUT);
  digitalWrite(rele, HIGH);
}

void loop() {
  digitalWrite(rele, LOW);
  delay(2000);

  digitalWrite(rele, HIGH);

  delay(10000);
}

