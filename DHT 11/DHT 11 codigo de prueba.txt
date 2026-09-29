

#include <DHT11.h>


DHT11 dht11(2); // DATA del DHT11 azul
#define LED_PIN 7 // LED del cable morado


void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  dht11.setDelay(1000);
  Serial.println("=== Sistema DHT11 con Reaccion SERIAL ===");
}


void loop() {
  int temp = dht11.readTemperature();
  int hum = dht11.readHumidity();


  // Si la lectura es valida
  if (temp != DHT11::ERROR_CHECKSUM && temp != DHT11::ERROR_TIMEOUT &&
      hum != DHT11::ERROR_CHECKSUM && hum != DHT11::ERROR_TIMEOUT) {


    Serial.print("Temperatura: ");
    Serial.print(temp);
    Serial.print(" C | Humedad: ");
    Serial.print(hum);
    Serial.println(" %");


    // --- REACCION CON SERIAL PRINT ---
    if (temp > 27) {
      digitalWrite(LED_PIN, HIGH);
      Serial.print("REACCION -> ");
      Serial.print("ALTA TEMPERATURA detectada (");
      Serial.print(temp);
      Serial.println("C). LED ENCENDIDO!");
    }
    else if (hum > 70) {
      digitalWrite(LED_PIN, HIGH);
      Serial.print("REACCION -> ");
      Serial.print("ALTA HUMEDAD detectada (");
      Serial.print(hum);
      Serial.println("%). LED ENCENDIDO!");
    }
    else {
      digitalWrite(LED_PIN, LOW);
      Serial.println("Estado: Normal - LED APAGADO");
    }
   
    Serial.println("---------------------------");


  } else {
    Serial.println("Error al leer el DHT11");
    Serial.println(DHT11::getErrorString(temp));
  }
}


