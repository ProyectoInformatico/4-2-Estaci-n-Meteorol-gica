#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_BMP280.h>
#include <DHT11.h>

const int pinSuelo = A0;
const int pinNivel = A1;
const int pinLdr = A2;
const int pinBuzzer = 5;
const int pinLedAlerta = 6;
const int pinDht = 7;
const int pinRele = 8;

const int direccionLcd = 0x27;

const int releOn = LOW;
const int releOff = HIGH;

const int sueloSeco = 900;
const int sueloMojado = 300;
const int ldrOscuro = 0;
const int ldrSol = 1023;
const int nivelVacio = 0;
const int nivelLleno = 450;

const int umbralEncendido = 35;
const int umbralApagado = 55;
const int umbralCritico = 15;
const bool regarSiCritico = true;

const int umbralLuzAlta = 70;
const int umbralLuzBaja = 60;

const int umbralTanqueBajo = 20;
const int nivelMedio = 33;
const int nivelAlto = 66;

const int umbralTempAlta = 38;
const int umbralTempBaja = 3;

const unsigned long tiempoMaxRiego = 30000;
const unsigned long tiempoPausa = 60000;
const unsigned long tiempoBienvenida = 2000;
const unsigned long intervaloSensores = 1000;
const unsigned long intervaloDht = 2000;
const unsigned long intervaloLcd = 500;
const unsigned long intervaloPantalla = 3000;
const unsigned long intervaloPulso = 400;
const unsigned long intervaloSerial = 2000;

const int frecTanque = 1000;
const int frecCalor = 3000;
const int frecFrio = 2500;

LiquidCrystal_I2C lcd(direccionLcd, 16, 2);
Adafruit_BMP280 bmp;
DHT11 dht11(pinDht);

int temperatura;
int humedadAire;
int presion;
int luz;
int humedadSuelo;
int nivelTanque;
int pantalla = 0;

bool dhtOk = false;
bool bmpOk = false;
bool luzAlta = false;
bool tanqueBajo = false;
bool temperaturaAlta = false;
bool temperaturaBaja = false;
bool bombaEncendida = false;
bool enPausa = false;
bool estadoPulso = false;
bool alertaMostrada = false;

unsigned long ultimoDht = 0;
unsigned long ultimoBmp = 0;
unsigned long ultimaLuz = 0;
unsigned long ultimoSuelo = 0;
unsigned long ultimoNivel = 0;
unsigned long ultimoPulso = 0;
unsigned long ultimoLcd = 0;
unsigned long ultimoCambioPantalla = 0;
unsigned long ultimoSerial = 0;
unsigned long inicioRiego = 0;
unsigned long finRiego = 0;

void leerDHT11() {
  if (ultimoDht == 0 || millis() - ultimoDht >= intervaloDht) {
    ultimoDht = millis();
    int temp;
    int hum;
    if (dht11.readTemperatureHumidity(temp, hum) == 0) {
      temperatura = temp;
      humedadAire = hum;
      dhtOk = true;
    } else {
      dhtOk = false;
    }
    temperaturaAlta = dhtOk && temperatura > umbralTempAlta;
    temperaturaBaja = dhtOk && temperatura < umbralTempBaja;
  }
}

void leerBMP280() {
  if (bmpOk && (ultimoBmp == 0 || millis() - ultimoBmp >= intervaloSensores)) {
    ultimoBmp = millis();
    presion = (int)(bmp.readPressure() / 100.0 + 0.5);
  }
}

void leerLuz() {
  if (ultimaLuz == 0 || millis() - ultimaLuz >= intervaloSensores) {
    ultimaLuz = millis();
    luz = constrain(map(analogRead(pinLdr), ldrOscuro, ldrSol, 0, 100), 0, 100);
    if (luz >= umbralLuzAlta) luzAlta = true;
    else if (luz <= umbralLuzBaja) luzAlta = false;
  }
}

void leerHumedadSuelo() {
  if (ultimoSuelo == 0 || millis() - ultimoSuelo >= intervaloSensores) {
    ultimoSuelo = millis();
    humedadSuelo = constrain(map(analogRead(pinSuelo), sueloSeco, sueloMojado, 0, 100), 0, 100);
  }
}

void leerNivelTanque() {
  if (ultimoNivel == 0 || millis() - ultimoNivel >= intervaloSensores) {
    ultimoNivel = millis();
    nivelTanque = constrain(map(analogRead(pinNivel), nivelVacio, nivelLleno, 0, 100), 0, 100);
    tanqueBajo = nivelTanque <= umbralTanqueBajo;
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

bool hayAlerta() {
  return tanqueBajo || temperaturaAlta || temperaturaBaja;
}

int frecuenciaAlerta() {
  if (tanqueBajo) return frecTanque;
  if (temperaturaAlta) return frecCalor;
  return frecFrio;
}

const char* textoAlerta() {
  if (tanqueBajo) return "tanque bajo";
  if (temperaturaAlta) return "temp alta";
  if (temperaturaBaja) return "temp baja";
  return "ninguna";
}

const char* textoRiego() {
  if (bombaEncendida) return "on";
  if (enPausa) return "esp";
  if (luzAlta && humedadSuelo <= umbralEncendido) return "sol";
  return "off";
}

const char* textoNivel() {
  if (nivelTanque >= nivelAlto) return "alto";
  if (nivelTanque >= nivelMedio) return "medio";
  if (nivelTanque > 0) return "bajo";
  return "vacio";
}

void gestionAlertas() {
  if (hayAlerta()) {
    if (millis() - ultimoPulso >= intervaloPulso) {
      ultimoPulso = millis();
      estadoPulso = !estadoPulso;
      digitalWrite(pinLedAlerta, estadoPulso);
      if (estadoPulso) tone(pinBuzzer, frecuenciaAlerta());
      else noTone(pinBuzzer);
    }
  } else if (estadoPulso) {
    estadoPulso = false;
    digitalWrite(pinLedAlerta, LOW);
    noTone(pinBuzzer);
  }
}

void escribirLinea(int fila, const char* texto) {
  char linea[17];
  snprintf(linea, sizeof(linea), "%-16s", texto);
  lcd.setCursor(0, fila);
  lcd.print(linea);
}

void actualizarLCD() {
  if (millis() < tiempoBienvenida) return;

  if (hayAlerta() && !alertaMostrada) {
    pantalla = 3;
    ultimoCambioPantalla = millis();
  }
  alertaMostrada = hayAlerta();

  if (millis() - ultimoCambioPantalla >= intervaloPantalla) {
    ultimoCambioPantalla = millis();
    pantalla++;
    if (pantalla > 3 || (pantalla == 3 && !hayAlerta())) pantalla = 0;
  }

  if (millis() - ultimoLcd >= intervaloLcd) {
    ultimoLcd = millis();
    char linea1[17];
    char linea2[17];

    if (pantalla == 0) {
      if (dhtOk) snprintf(linea1, sizeof(linea1), "T:%dC H:%d%%", temperatura, humedadAire);
      else snprintf(linea1, sizeof(linea1), "DHT11: error");
      if (bmpOk) snprintf(linea2, sizeof(linea2), "P:%d hPa", presion);
      else snprintf(linea2, sizeof(linea2), "BMP280: error");
    } else if (pantalla == 1) {
      snprintf(linea1, sizeof(linea1), "Suelo:%d%%", humedadSuelo);
      snprintf(linea2, sizeof(linea2), "Luz:%d%%", luz);
    } else if (pantalla == 2) {
      snprintf(linea1, sizeof(linea1), "Riego:%s", textoRiego());
      snprintf(linea2, sizeof(linea2), "Agua:%d%% %s", nivelTanque, textoNivel());
    } else {
      snprintf(linea1, sizeof(linea1), "!! ALERTA !!");
      snprintf(linea2, sizeof(linea2), "%s", textoAlerta());
    }

    escribirLinea(0, linea1);
    escribirLinea(1, linea2);
  }
}

void imprimirSerial() {
  if (millis() - ultimoSerial >= intervaloSerial) {
    ultimoSerial = millis();
    Serial.print(F("T:"));
    if (dhtOk) Serial.print(temperatura); else Serial.print(F("err"));
    Serial.print(F("C H:"));
    if (dhtOk) Serial.print(humedadAire); else Serial.print(F("err"));
    Serial.print(F("% P:"));
    if (bmpOk) Serial.print(presion); else Serial.print(F("err"));
    Serial.print(F("hPa | Suelo:"));
    Serial.print(humedadSuelo);
    Serial.print(F("% Luz:"));
    Serial.print(luz);
    Serial.print(F("% Agua:"));
    Serial.print(nivelTanque);
    Serial.print(F("% | Riego:"));
    Serial.print(textoRiego());
    Serial.print(F(" | Alerta:"));
    Serial.println(textoAlerta());
  }
}

void setup() {
  digitalWrite(pinRele, releOff);
  pinMode(pinRele, OUTPUT);
  pinMode(pinLedAlerta, OUTPUT);
  pinMode(pinBuzzer, OUTPUT);

  Serial.begin(9600);
  dht11.setDelay(0);
  bmpOk = bmp.begin(0x76) || bmp.begin(0x77);

  lcd.init();
  lcd.backlight();
  escribirLinea(0, "Estacion Meteo");
  escribirLinea(1, "Iniciando...");
}

void loop() {
  leerDHT11();
  leerBMP280();
  leerLuz();
  leerHumedadSuelo();
  leerNivelTanque();
  controlRiego();
  gestionAlertas();
  actualizarLCD();
  imprimirSerial();
}
