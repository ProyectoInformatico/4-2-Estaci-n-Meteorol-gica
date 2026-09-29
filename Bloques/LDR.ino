void setup()
{
  pinMode(A0, INPUT);
  Serial.begin(9600);
}

void loop()
{
  Serial.print("LUZ: ");
  Serial.println(analogRead(A0));
  delay(100);
}
