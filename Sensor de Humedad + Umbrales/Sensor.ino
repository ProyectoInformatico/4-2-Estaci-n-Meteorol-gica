#define sensor A0


int seco = 900;
int mojado = 300;


void setup()
{
  Serial.begin(9600);
}


void loop()
{
  int lectura = analogRead(sensor);


  int humedad = map(lectura, seco, mojado, 0, 100);


  humedad = constrain(humedad, 0, 100);


  Serial.print("Lectura: ");
  Serial.print(lectura);


  Serial.print(" | Humedad: ");
  Serial.print(humedad);
  Serial.println("%");


  delay(1000);
}

