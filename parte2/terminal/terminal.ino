/*
 * ============================================================
 *  Laboratorio Comunicacion Serial - Parte II
 *  Arduino TERMINAL (arriba)
 *  - Servo en pin 9
 *  - Potenciometro en A0
 *  - Recibe comandos del HMI por Serial
 *  - Responde resultados al HMI
 * ============================================================
 *  Protocolo:
 *    Recibe:  "SERVO:90"      mueve servo
 *             "SENSOR:1000"   envia 5 lecturas
 *             "WELCOME:"      lee EEPROM
 *             "WELCOME:texto" guarda en EEPROM
 *    Envia:   "OK:servo:90"
 *             "DATA:t:raw:v"  (5 veces)
 *             "DONE:sensor"
 *             "OK:welcome:texto"
 * ============================================================
 */

#include <Servo.h>
#include <EEPROM.h>

#define PIN_SERVO          9
#define PIN_SENSOR         A0
#define EEPROM_MAGIC_ADDR  0
#define EEPROM_MSG_ADDR    1
#define EEPROM_MSG_LEN     40
#define EEPROM_MAGIC_VAL   0xA5
#define MAX_CMD_LEN        40
#define CMD_TIMEOUT_MS     300

Servo myServo;

char          cmdBuf[MAX_CMD_LEN + 1];
uint8_t       cmdLen      = 0;
unsigned long lastByte    = 0;
bool          timerActive = false;

void executeCmd(const char* cmd);
void doServo(int angle);
void doSensor(int ms);
void doWelcome(const char* args);
void setServoAngle(int angle);
int  toInt(const char* s);
bool sw(const char* s, const char* p);

// ═══════════════════════════════════════════════════════════
void setup() {
  Serial.begin(9600);
  setServoAngle(90);
}

void loop() {
  // Timeout
  if (timerActive && cmdLen > 0) {
    if ((millis() - lastByte) >= CMD_TIMEOUT_MS) {
      timerActive = false;
      cmdBuf[cmdLen] = '\0';
      executeCmd(cmdBuf);
      cmdLen = 0;
      memset(cmdBuf, 0, sizeof(cmdBuf));
      return;
    }
  }

  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    lastByte    = millis();
    timerActive = true;

    if (c == '\n' || c == '\r') {
      if (cmdLen == 0) { timerActive = false; continue; }
      timerActive = false;
      cmdBuf[cmdLen] = '\0';
      executeCmd(cmdBuf);
      cmdLen = 0;
      memset(cmdBuf, 0, sizeof(cmdBuf));
      return;
    }

    if (cmdLen < MAX_CMD_LEN) cmdBuf[cmdLen++] = c;
  }
}

// ═══════════════════════════════════════════════════════════
void executeCmd(const char* cmd) {
  if (sw(cmd, "SERVO:")) {
    int a = toInt(cmd + 6);
    if (a >= 0 && a <= 180) doServo(a);
    else { Serial.println(F("ERR:angulo")); }
    return;
  }
  if (sw(cmd, "SENSOR:")) {
    int ms = toInt(cmd + 7);
    if (ms <= 0) ms = 1000;
    doSensor(ms);
    return;
  }
  if (sw(cmd, "WELCOME:")) {
    doWelcome(cmd + 8);
    return;
  }
  Serial.print(F("ERR:desconocido"));
}

void doServo(int angle) {
  setServoAngle(angle);
  Serial.print(F("OK:servo:"));
  Serial.println(angle);
}

void doSensor(int ms) {
  Serial.print(F("OK:sensor:"));
  Serial.println(ms);
  unsigned long t0 = millis();
  for (int i = 0; i < 5; i++) {
    unsigned long t = millis() - t0;
    int raw = analogRead(PIN_SENSOR);
    float v = raw * (5.0 / 1023.0);
    Serial.print(F("DATA:"));
    Serial.print(t);
    Serial.print(':');
    Serial.print(raw);
    Serial.print(':');
    Serial.println(v, 2);
    delay(ms);
  }
  Serial.println(F("DONE:sensor"));
}

void doWelcome(const char* args) {
  if (!args || strlen(args) == 0) {
    if (EEPROM.read(EEPROM_MAGIC_ADDR) == EEPROM_MAGIC_VAL) {
      char msg[EEPROM_MSG_LEN];
      for (uint8_t i = 0; i < EEPROM_MSG_LEN; i++) {
        msg[i] = (char)EEPROM.read(EEPROM_MSG_ADDR + i);
        if (!msg[i]) break;
      }
      Serial.print(F("OK:welcome:"));
      Serial.println(msg);
    } else {
      Serial.println(F("OK:welcome:vacio"));
    }
  } else {
    EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VAL);
    uint8_t i = 0;
    while (args[i] && i < EEPROM_MSG_LEN - 1) {
      EEPROM.write(EEPROM_MSG_ADDR + i, (uint8_t)args[i]);
      i++;
    }
    EEPROM.write(EEPROM_MSG_ADDR + i, 0);
    Serial.print(F("OK:guardado:"));
    Serial.println(args);
  }
}

void setServoAngle(int angle) {
  myServo.detach();
  delay(50);
  myServo.attach(PIN_SERVO);
  delay(50);
  myServo.write(angle);
  delay(600);
}

int toInt(const char* s) {
  if (!s || !*s) return -1;
  int r = 0; bool ok = false;
  for (uint8_t i = 0; s[i]; i++) {
    if (s[i] >= '0' && s[i] <= '9') { r = r*10+(s[i]-'0'); ok=true; }
    else return -1;
  }
  return ok ? r : -1;
}

bool sw(const char* s, const char* p) {
  while (*p) { if (*s++ != *p++) return false; }
  return true;
}
