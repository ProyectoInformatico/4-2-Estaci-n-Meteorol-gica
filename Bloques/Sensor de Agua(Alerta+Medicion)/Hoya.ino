#define BUZZ 3
#define Sensor A1
void setup() {
   pinMode(Sensor, INPUT);
   pinMode(BUZZ, OUTPUT);
   Serial.begin(9600); // abre el puerto serie
}


void loop() {
   int water = analogRead(Sensor);
   water = map(water, 0, 450, 0 ,3);
   if(water ==  3){
    Serial.println("Nivel De Agua: Alto");
    digitalWrite(BUZZ, LOW);
   }else if(water == 2){
    Serial.println("Nivel De Agua: Medio");
    digitalWrite(BUZZ, LOW);
   }else if(water == 1){
    Serial.println("Nivel De Agua: Bajo");
    Serial.print("    ¡Rellenar!     ");
    digitalWrite(BUZZ, HIGH);
   }else if(water == 0)
   {
    Serial.println("No Se Detecta Agua");
    digitalWrite(BUZZ, LOW);
   }
 
   delay(30); // espera un segundo
}
