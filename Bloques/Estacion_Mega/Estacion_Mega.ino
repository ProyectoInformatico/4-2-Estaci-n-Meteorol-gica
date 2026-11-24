// Estacion Meteorologica con Riego Automatizado - programa integrado
// Placa: Arduino Mega 2560 | Base: Integrado/Proyecto.ino
// I2C en el Mega: SDA = 20, SCL = 21 (A4/A5 NO son I2C en el Mega)

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_BMP280.h>
#include <DHT11.h>

// ===== PINES (PinMap Mega) =====
const int pinSuelo = A0;
const int pinNivel = A1;
const int pinLdr = A2;
const int pinBoton = A3;
const int pinEncoderClk = 2;
const int pinEncoderDt = 3;
const int pinEncoderSw = 4;
const int pinBuzzer = 5;
const int pinLedAlerta = 6;
const int pinDht = 7;
const int pinRele = 8;
const int pinLuzExtra = 10;
// LCD 16x2 y BMP280: SDA 20 / SCL 21 (bus I2C, sin constante)
// ===== FIN PINES =====

// ===== CONFIGURACION GENERAL =====
const unsigned long intervaloSensores = 1000;
const unsigned long tiempoDebounce = 50;
// ===== FIN CONFIGURACION GENERAL =====

// ===== DHT11 (temperatura y humedad del aire) =====
const unsigned long intervaloDht = 2000;
const int umbralTempAlta = 38;
const int umbralTempBaja = 3;

DHT11 dht11(pinDht);

int temperatura;
int humedadAire;
bool dhtOk = false;
bool temperaturaAlta = false;
bool temperaturaBaja = false;
unsigned long ultimoDht = 0;

void dhtInit() {
  dht11.setDelay(0);
}

void dhtLeer() {
  if (ultimoDht == 0 || millis() - ultimoDht >= intervaloDht) {
    ultimoDht = millis();
    int temp;
    int hum;
    dhtOk = dht11.readTemperatureHumidity(temp, hum) == 0;
    if (dhtOk) {
      temperatura = temp;
      humedadAire = hum;
    }
    temperaturaAlta = dhtOk && temperatura > umbralTempAlta;
    temperaturaBaja = dhtOk && temperatura < umbralTempBaja;
  }
}
// ===== FIN DHT11 =====

// ===== BMP280 (presion atmosferica) =====
Adafruit_BMP280 bmp;

int presion;
bool bmpOk = false;
unsigned long ultimoBmp = 0;

void bmpInit() {
  bmpOk = bmp.begin(0x76) || bmp.begin(0x77);
}

void bmpLeer() {
  if (bmpOk && (ultimoBmp == 0 || millis() - ultimoBmp >= intervaloSensores)) {
    ultimoBmp = millis();
    presion = (int)(bmp.readPressure() / 100.0 + 0.5);
  }
}
// ===== FIN BMP280 =====

// ===== LDR (luz ambiental) =====
const int ldrOscuro = 0;
const int ldrSol = 1023;
const int umbralLuzAlta = 70;
const int umbralLuzBaja = 60;

int luz;
bool luzAlta = false;
unsigned long ultimaLuz = 0;

void luzLeer() {
  if (ultimaLuz == 0 || millis() - ultimaLuz >= intervaloSensores) {
    ultimaLuz = millis();
    luz = constrain(map(analogRead(pinLdr), ldrOscuro, ldrSol, 0, 100), 0, 100);
    if (luz >= umbralLuzAlta) luzAlta = true;
    else if (luz <= umbralLuzBaja) luzAlta = false;
  }
}
// ===== FIN LDR =====

// ===== HUMEDAD DE SUELO =====
const int sueloSeco = 900;
const int sueloMojado = 300;

int humedadSuelo;
unsigned long ultimoSuelo = 0;

void sueloLeer() {
  if (ultimoSuelo == 0 || millis() - ultimoSuelo >= intervaloSensores) {
    ultimoSuelo = millis();
    humedadSuelo = constrain(map(analogRead(pinSuelo), sueloSeco, sueloMojado, 0, 100), 0, 100);
  }
}
// ===== FIN HUMEDAD DE SUELO =====

// ===== NIVEL DE TANQUE =====
const int nivelVacio = 0;
const int nivelLleno = 450;
const int umbralTanqueBajo = 20;
const int nivelMedio = 33;
const int nivelAlto = 66;

int nivelTanque;
bool tanqueBajo = false;
unsigned long ultimoNivel = 0;

void nivelLeer() {
  if (ultimoNivel == 0 || millis() - ultimoNivel >= intervaloSensores) {
    ultimoNivel = millis();
    nivelTanque = constrain(map(analogRead(pinNivel), nivelVacio, nivelLleno, 0, 100), 0, 100);
    tanqueBajo = nivelTanque <= umbralTanqueBajo;
  }
}

const char* textoNivel() {
  if (nivelTanque >= nivelAlto) return "alto";
  if (nivelTanque >= nivelMedio) return "medio";
  if (nivelTanque > 0) return "bajo";
  return "vacio";
}
// ===== FIN NIVEL DE TANQUE =====

// ===== ENCODER + MENU DE CONFIGURACION =====
const unsigned long antirreboteGiro = 3000;
const unsigned long tiempoSalidaMenu = 15000;
const int histeresisSuelo = 20;

const int cantidadOpciones = 5;
const char* opciones[cantidadOpciones] = {"Umbral riego", "Intervalo riego", "Duracion riego", "Luz extra", "Salir"};
const char* modosLuz[3] = {"AUTO", "ON", "OFF"};

volatile int pasosEncoder = 0;
volatile unsigned long ultimoPasoEncoder = 0;

int umbralEncendido = 35;
int intervaloRiegoHoras = 12;
int duracionRiegoSeg = 10;
int modoLuzExtra = 0;

bool menuActivo = false;
bool editando = false;
int opcionMenu = 0;
bool estadoSw = HIGH;
bool lecturaSwAnterior = HIGH;
unsigned long ultimoCambioSw = 0;
unsigned long ultimaAccionMenu = 0;
bool menuRefrescar = false;

void encoderISR() {
  if (micros() - ultimoPasoEncoder > antirreboteGiro) {
    pasosEncoder += digitalRead(pinEncoderDt) ? 1 : -1;
    ultimoPasoEncoder = micros();
  }
}

void encoderInit() {
  pinMode(pinEncoderClk, INPUT_PULLUP);
  pinMode(pinEncoderDt, INPUT_PULLUP);
  pinMode(pinEncoderSw, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pinEncoderClk), encoderISR, FALLING);
}

int encoderPasos() {
  noInterrupts();
  int pasos = pasosEncoder;
  pasosEncoder = 0;
  interrupts();
  return pasos;
}

bool encoderSwPresionado() {
  bool lectura = digitalRead(pinEncoderSw);
  if (lectura != lecturaSwAnterior) ultimoCambioSw = millis();
  lecturaSwAnterior = lectura;

  if (millis() - ultimoCambioSw >= tiempoDebounce && lectura != estadoSw) {
    estadoSw = lectura;
    return estadoSw == LOW;
  }
  return false;
}

int umbralApagado() {
  return umbralEncendido + histeresisSuelo;
}

void menuCambiarValor(int pasos) {
  if (opcionMenu == 0) umbralEncendido = constrain(umbralEncendido + pasos * 5, 5, 75);
  else if (opcionMenu == 1) intervaloRiegoHoras = constrain(intervaloRiegoHoras + pasos, 1, 24);
  else if (opcionMenu == 2) duracionRiegoSeg = constrain(duracionRiegoSeg + pasos * 5, 5, 60);
  else if (opcionMenu == 3) modoLuzExtra = (modoLuzExtra + pasos % 3 + 3) % 3;
}

void menuTextoValor(char* destino, int tam) {
  if (opcionMenu == 0) snprintf(destino, tam, "%d%%", umbralEncendido);
  else if (opcionMenu == 1) snprintf(destino, tam, "%d h", intervaloRiegoHoras);
  else if (opcionMenu == 2) snprintf(destino, tam, "%d s", duracionRiegoSeg);
  else if (opcionMenu == 3) snprintf(destino, tam, "%s", modosLuz[modoLuzExtra]);
  else destino[0] = '\0';
}

void menuActualizar() {
  int pasos = encoderPasos();
  bool presionado = encoderSwPresionado();
  if (pasos != 0 || presionado) {
    ultimaAccionMenu = millis();
    menuRefrescar = true;
  }

  if (!menuActivo) {
    if (presionado) {
      menuActivo = true;
      editando = false;
      opcionMenu = 0;
    }
    return;
  }

  if (pasos != 0) {
    if (editando) menuCambiarValor(pasos);
    else opcionMenu = ((opcionMenu + pasos) % cantidadOpciones + cantidadOpciones) % cantidadOpciones;
  }
  if (presionado) {
    if (opcionMenu == cantidadOpciones - 1) menuActivo = false;
    else editando = !editando;
  }
  if (millis() - ultimaAccionMenu >= tiempoSalidaMenu) {
    menuActivo = false;
    menuRefrescar = true;
  }
}
// ===== FIN ENCODER + MENU DE CONFIGURACION =====

// ===== BOTON MANUAL =====
const unsigned long tiempoMaxManual = 30000;

bool riegoManual = false;
bool estadoBoton = HIGH;
bool lecturaBotonAnterior = HIGH;
unsigned long ultimoCambioBoton = 0;
unsigned long inicioManual = 0;

void botonInit() {
  pinMode(pinBoton, INPUT_PULLUP);
}

void botonActualizar() {
  bool lectura = digitalRead(pinBoton);
  if (lectura != lecturaBotonAnterior) ultimoCambioBoton = millis();
  lecturaBotonAnterior = lectura;

  if (millis() - ultimoCambioBoton >= tiempoDebounce && lectura != estadoBoton) {
    estadoBoton = lectura;
    if (estadoBoton == LOW) {
      riegoManual = !riegoManual;
      inicioManual = millis();
    }
  }

  if (riegoManual && (tanqueBajo || millis() - inicioManual >= tiempoMaxManual)) riegoManual = false;
}
// ===== FIN BOTON MANUAL =====

// ===== TEMPORIZADOR DE RIEGO (millis, sin RTC) =====
bool riegoProgramado = false;
bool programadoPendiente = false;
unsigned long ultimoProgramado = 0;
unsigned long inicioProgramado = 0;

unsigned long intervaloRiegoMs() {
  return intervaloRiegoHoras * 3600000UL;
}

void temporizadorActualizar() {
  if (!riegoProgramado && !programadoPendiente && millis() - ultimoProgramado >= intervaloRiegoMs()) {
    programadoPendiente = true;
    ultimoProgramado = millis();
  }

  if (programadoPendiente) {
    if (humedadSuelo >= umbralApagado()) programadoPendiente = false;
    else if (!luzAlta && !tanqueBajo) {
      programadoPendiente = false;
      riegoProgramado = true;
      inicioProgramado = millis();
    }
  }

  if (riegoProgramado && (tanqueBajo || millis() - inicioProgramado >= duracionRiegoSeg * 1000UL)) riegoProgramado = false;
}

unsigned long minutosProximoRiego() {
  if (programadoPendiente || riegoProgramado) return 0;
  return (intervaloRiegoMs() - (millis() - ultimoProgramado)) / 60000UL;
}
// ===== FIN TEMPORIZADOR DE RIEGO =====

// ===== RIEGO AUTOMATICO + RELE BOMBA =====
const int releOn = LOW;
const int releOff = HIGH;
const int umbralCritico = 15;
const bool regarSiCritico = true;
const unsigned long tiempoMaxRiego = 30000;
const unsigned long tiempoPausa = 60000;

bool riegoAuto = false;
bool bombaEncendida = false;
bool enPausa = false;
unsigned long inicioRiego = 0;
unsigned long finRiego = 0;

void riegoInit() {
  digitalWrite(pinRele, releOff);
  pinMode(pinRele, OUTPUT);
}

void riegoActualizar() {
  bool sueloCritico = regarSiCritico && humedadSuelo <= umbralCritico;
  bool solPermite = !luzAlta || sueloCritico;

  if (riegoAuto) {
    bool tiempoExcedido = millis() - inicioRiego >= tiempoMaxRiego;
    if (humedadSuelo >= umbralApagado() || tanqueBajo || !solPermite || tiempoExcedido) {
      riegoAuto = false;
      finRiego = millis();
      enPausa = tiempoExcedido;
    }
  } else {
    if (enPausa && millis() - finRiego >= tiempoPausa) enPausa = false;
    if (humedadSuelo <= umbralEncendido && !tanqueBajo && solPermite && !enPausa) {
      riegoAuto = true;
      inicioRiego = millis();
    }
  }

  bombaEncendida = !tanqueBajo && (riegoAuto || riegoManual || riegoProgramado);
  digitalWrite(pinRele, bombaEncendida ? releOn : releOff);
}

const char* textoRiego() {
  if (bombaEncendida) {
    if (riegoManual) return "manual";
    if (riegoProgramado) return "program";
    return "auto";
  }
  if (enPausa) return "esp";
  if (luzAlta && (humedadSuelo <= umbralEncendido || programadoPendiente)) return "sol";
  return "off";
}
// ===== FIN RIEGO AUTOMATICO + RELE BOMBA =====

// ===== LUZ EXTRA =====
const int umbralLuzExtraOn = 25;
const int umbralLuzExtraOff = 35;

bool luzExtraEncendida = false;

void luzExtraInit() {
  pinMode(pinLuzExtra, OUTPUT);
  digitalWrite(pinLuzExtra, LOW);
}

void luzExtraActualizar() {
  if (modoLuzExtra == 1) luzExtraEncendida = true;
  else if (modoLuzExtra == 2) luzExtraEncendida = false;
  else if (luz <= umbralLuzExtraOn) luzExtraEncendida = true;
  else if (luz >= umbralLuzExtraOff) luzExtraEncendida = false;

  digitalWrite(pinLuzExtra, luzExtraEncendida);
}
// ===== FIN LUZ EXTRA =====

// ===== ALARMA (LED + BUZZER) =====
const unsigned long intervaloPulso = 400;
const int frecTanque = 1000;
const int frecCalor = 3000;
const int frecFrio = 2500;

bool estadoPulso = false;
unsigned long ultimoPulso = 0;

void alarmaInit() {
  pinMode(pinLedAlerta, OUTPUT);
  pinMode(pinBuzzer, OUTPUT);
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

void alarmaActualizar() {
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
// ===== FIN ALARMA =====

// ===== LCD 16x2 I2C =====
const int direccionLcd = 0x27;
const unsigned long tiempoBienvenida = 2000;
const unsigned long intervaloLcd = 500;
const unsigned long intervaloPantalla = 3000;
const int pantallaAlerta = 4;

LiquidCrystal_I2C lcd(direccionLcd, 16, 2);

int pantalla = 0;
bool alertaMostrada = false;
unsigned long ultimoLcd = 0;
unsigned long ultimoCambioPantalla = 0;

void escribirLinea(int fila, const char* texto) {
  char linea[17];
  snprintf(linea, sizeof(linea), "%-16s", texto);
  lcd.setCursor(0, fila);
  lcd.print(linea);
}

void lcdInit() {
  lcd.init();
  lcd.backlight();
  escribirLinea(0, "Estacion Meteo");
  escribirLinea(1, "Iniciando...");
}

void lcdMenu(char* linea1, char* linea2) {
  char valor[10];
  menuTextoValor(valor, sizeof(valor));
  snprintf(linea1, 17, ">%s", opciones[opcionMenu]);
  if (editando) snprintf(linea2, 17, " [%s]", valor);
  else snprintf(linea2, 17, "  %s", valor);
}

void lcdPantallaDatos(char* linea1, char* linea2) {
  if (pantalla == 0) {
    if (dhtOk) snprintf(linea1, 17, "T:%dC H:%d%%", temperatura, humedadAire);
    else snprintf(linea1, 17, "DHT11: error");
    if (bmpOk) snprintf(linea2, 17, "P:%d hPa", presion);
    else snprintf(linea2, 17, "BMP280: error");
  } else if (pantalla == 1) {
    snprintf(linea1, 17, "Suelo:%d%%", humedadSuelo);
    snprintf(linea2, 17, "Luz:%d%%", luz);
  } else if (pantalla == 2) {
    snprintf(linea1, 17, "Riego:%s", textoRiego());
    snprintf(linea2, 17, "Agua:%d%% %s", nivelTanque, textoNivel());
  } else if (pantalla == 3) {
    unsigned long minutos = minutosProximoRiego();
    if (riegoProgramado) snprintf(linea1, 17, "Prog: regando");
    else if (programadoPendiente) snprintf(linea1, 17, "Prog: en espera");
    else snprintf(linea1, 17, "Prog:%luh %lum", minutos / 60, minutos % 60);
    snprintf(linea2, 17, "Luz ext:%s %s", luzExtraEncendida ? "on" : "off", modosLuz[modoLuzExtra]);
  } else {
    snprintf(linea1, 17, "!! ALERTA !!");
    snprintf(linea2, 17, "%s", textoAlerta());
  }
}

void lcdActualizar() {
  if (millis() < tiempoBienvenida) return;

  if (hayAlerta() && !alertaMostrada) {
    pantalla = pantallaAlerta;
    ultimoCambioPantalla = millis();
  }
  alertaMostrada = hayAlerta();

  if (millis() - ultimoCambioPantalla >= intervaloPantalla) {
    ultimoCambioPantalla = millis();
    pantalla++;
    if (pantalla > pantallaAlerta || (pantalla == pantallaAlerta && !hayAlerta())) pantalla = 0;
  }

  if (menuRefrescar || millis() - ultimoLcd >= intervaloLcd) {
    menuRefrescar = false;
    ultimoLcd = millis();
    char linea1[17];
    char linea2[17];
    if (menuActivo) lcdMenu(linea1, linea2);
    else lcdPantallaDatos(linea1, linea2);
    escribirLinea(0, linea1);
    escribirLinea(1, linea2);
  }
}
// ===== FIN LCD 16x2 I2C =====

// ===== SERIAL (monitoreo) =====
const unsigned long intervaloSerial = 2000;

unsigned long ultimoSerial = 0;

void serialInit() {
  Serial.begin(9600);
}

void serialImprimir() {
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
    Serial.print(F(" Prox:"));
    Serial.print(minutosProximoRiego());
    Serial.print(F("min | LuzExtra:"));
    Serial.print(luzExtraEncendida ? F("on") : F("off"));
    Serial.print(F(" | Alerta:"));
    Serial.println(textoAlerta());
  }
}
// ===== FIN SERIAL =====

void setup() {
  riegoInit();
  alarmaInit();
  luzExtraInit();
  botonInit();
  encoderInit();
  serialInit();
  dhtInit();
  bmpInit();
  lcdInit();
}

void loop() {
  dhtLeer();
  bmpLeer();
  luzLeer();
  sueloLeer();
  nivelLeer();

  menuActualizar();
  botonActualizar();
  temporizadorActualizar();
  riegoActualizar();
  luzExtraActualizar();
  alarmaActualizar();

  lcdActualizar();
  serialImprimir();
}
