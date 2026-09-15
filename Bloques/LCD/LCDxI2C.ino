
#include <Adafruit_LiquidCrystal.h>

Adafruit_LiquidCrystal lcd(0x20);

void setup()
{ 
  
  lcd.begin(16, 2);
  lcd.setBacklight(HIGH);
  lcd.print("Estacion Meteo");

 
}

void loop()
{
  
  int i;
  
  for ( int i = 0; i < 5; i++ ) {
    lcd.scrollDisplayRight();
    delay (1000);
  }

  for ( int i = 0; i < 5; i++ ) {
    lcd.scrollDisplayLeft();
    delay (1000);
  }
}