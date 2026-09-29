#include "Adafruit_BMP280.h"

Adafruit_BMP280 bmp;

void setup() {
  Serial.begin(9600);

  // Cambia 0x76 por 0x77 si no lee nada
  bmp.begin(0x76);
}

void loop() {
  Serial.print("Temp: ");
  Serial.print(bmp.readTemperature());
  Serial.println(" C");

  Serial.print("Presion: ");
  Serial.print(bmp.readPressure() / 100);
  Serial.println(" hPa");

  Serial.print("Altitud: ");
  Serial.print(bmp.readAltitude(1015)); // Ajusta el 1015 según tu clima local
  Serial.println(" m");

  Serial.println("---");
  delay(1000);
}
