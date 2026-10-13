#include <Adafruit_LiquidCrystal.h>

const int pinSuelo = A0;
const int pinLdr = A2;
const int pinRele = 8;
const int pinLedAlerta = 6;
const int pinBuzzer = 5;

const int releOn = LOW;
const int releOff = HIGH;

const int sueloSeco = 0;
const int sueloMojado = 876;
const int ldrOscuro = 0;
const int ldrSol = 1023;

const int umbralEncendido = 35;
const int umbralApagado = 55;
const int umbralCritico = 15;
const bool regarSiCritico = true;

const int umbralLuzAlta = 70;
const int umbralLuzBaja = 60;

const int umbralTanqueBajo = 20;
const int umbralTempAlta = 35;

const unsigned long tiempoMaxRiego = 30000;
const unsigned long tiempoPausa = 60000;
const unsigned long intervaloLectura = 1000;
const unsigned long intervaloLcd = 500;
const unsigned long intervaloPulso = 400;

const int frecTanque = 1000;
const int frecTemp = 2000;

Adafruit_LiquidCrystal lcd(0);

int humedadSuelo;
int luz;
int nivelTanque;
int temperatura;

bool luzAlta = false;
bool tanqueBajo = false;
bool temperaturaAlta = false;
bool bombaEncendida = false;
bool enPausa = false;
bool estadoPulso = false;

unsigned long ultimaLectura = 0;
unsigned long ultimoLcd = 0;
unsigned long ultimoPulso = 0;
unsigned long inicioRiego = 0;
unsigned long finRiego = 0;

void leerSensores() {
  if (ultimaLectura == 0 || millis() - ultimaLectura >= intervaloLectura) {
    ultimaLectura = millis();
    humedadSuelo = constrain(map(analogRead(pinSuelo), sueloSeco, sueloMojado, 0, 100), 0, 100);
    luz = constrain(map(analogRead(pinLdr), ldrOscuro, ldrSol, 0, 100), 0, 100);
    if (luz >= umbralLuzAlta) luzAlta = true;
    else if (luz <= umbralLuzBaja) luzAlta = false;
    tanqueBajo = nivelTanque <= umbralTanqueBajo;
    temperaturaAlta = temperatura >= umbralTempAlta;
  }
}

void controlRiego() {
  bool sueloCritico = regarSiCritico && humedadSuelo <= umbralCritico;
  bool solPermite = !luzAlta || sueloCritico;

  if (bombaEncendida) {
    bool tiempoExcedido = millis() - inicioRiego >= tiempoMaxRiego;
    if (humedadSuelo >= umbralApagado || tanqueBajo || !solPermite || tiempoExcedido) {
      bombaEncendida = false;
      finRiego = millis();
      enPausa = tiempoExcedido;
    }
  } else {
    if (enPausa && millis() - finRiego >= tiempoPausa) enPausa = false;
    if (humedadSuelo <= umbralEncendido && !tanqueBajo && solPermite && !enPausa) {
      bombaEncendida = true;
      inicioRiego = millis();
    }
  }

  digitalWrite(pinRele, bombaEncendida ? releOn : releOff);
}

void actualizarLCD() {
  if (millis() - ultimoLcd >= intervaloLcd) {
    ultimoLcd = millis();

    lcd.setCursor(0, 0);
    lcd.print("hum:");
    lcd.print(humedadSuelo);
    lcd.print("%   ");
    lcd.setCursor(8, 0);
    lcd.print("luz:");
    lcd.print(luz);
    lcd.print("%  ");

    lcd.setCursor(0, 1);
    lcd.print("riego:");
    if (bombaEncendida) lcd.print("on ");
    else if (enPausa) lcd.print("esp");
    else if (luzAlta && humedadSuelo <= umbralEncendido) lcd.print("sol");
    else lcd.print("off");

    lcd.setCursor(10, 1);
    if (tanqueBajo) lcd.print("tanque");
    else if (temperaturaAlta) lcd.print("temp  ");
    else lcd.print("ok    ");
  }
}

void gestionAlertas() {
  if (tanqueBajo || temperaturaAlta) {
    if (millis() - ultimoPulso >= intervaloPulso) {
      ultimoPulso = millis();
      estadoPulso = !estadoPulso;
      digitalWrite(pinLedAlerta, estadoPulso);
      if (estadoPulso) tone(pinBuzzer, tanqueBajo ? frecTanque : frecTemp);
      else noTone(pinBuzzer);
    }
  } else if (estadoPulso) {
    estadoPulso = false;
    digitalWrite(pinLedAlerta, LOW);
    noTone(pinBuzzer);
  }
}

void setup() {
  pinMode(pinRele, OUTPUT);
  digitalWrite(pinRele, releOff);
  pinMode(pinLedAlerta, OUTPUT);
  pinMode(pinBuzzer, OUTPUT);
  lcd.begin(16, 2);
  lcd.setBacklight(1);
}

void loop() {
  leerSensores();
  controlRiego();
  actualizarLCD();
  gestionAlertas();
}
