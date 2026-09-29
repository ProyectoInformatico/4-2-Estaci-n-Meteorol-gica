void setup() {
   pinMode(A1, INPUT);
   Serial.begin(9600); // abre el puerto serie
}


void loop() {
   water = analogRead(A1);
   Serial.println(water);
 
   delay(30); // espera un segundo
}
