/*
 * ============================================================
 *  Laboratorio Comunicacion Serial - Parte I
 *  Terminal CLI interactiva para Arduino Uno
 * ============================================================
 *  Comandos:
 *    servo               pide angulo interactivo (0-180)
 *    sensor <ms>         lee potenciometro cada <ms>, 'q' detiene
 *    welcome             muestra/guarda mensaje en EEPROM
 *    help                lista de comandos
 *    clear               limpia pantalla
 * ============================================================
 */

 //update 
#include <Servo.h>
#include <EEPROM.h>

// ── Pines ──────────────────────────────────────────────────
#define PIN_SERVO   9
#define PIN_SENSOR  A0

// ── EEPROM ─────────────────────────────────────────────────
#define EEPROM_MAGIC_ADDR  0
#define EEPROM_MSG_ADDR    1
#define EEPROM_MSG_LEN     40
#define EEPROM_MAGIC_VAL   0xA5

// ── Terminal ───────────────────────────────────────────────
#define MAX_CMD_LEN    40
#define CMD_TIMEOUT_MS 300
#define CTRL_C         0x03

Servo myServo;

char          cmdBuf[MAX_CMD_LEN + 1];
uint8_t       cmdLen       = 0;
bool          waitAngle    = false;
unsigned long lastByteTime = 0;
bool          timerActive  = false;
int           currentAngle = 90;

// ── Prototipos ─────────────────────────────────────────────
void executeCmd(const char* cmd);
void doServo();
void setServoAngle(int angle);
void doSensor(int ms);
void doWelcome(const char* args);
void doHelp();
void printBanner();
void prompt();
int  toInt(const char* s);
bool sw(const char* s, const char* p);

// ═══════════════════════════════════════════════════════════
void setup() {
  Serial.begin(9600);
  setServoAngle(90);
  printBanner();
  prompt();
}

// ── Mover servo: detach -> attach -> write -> delay ────────
// En Tinkercad este ciclo es necesario para que cada
// movimiento se ejecute correctamente
void setServoAngle(int angle) {
  myServo.detach();
  delay(50);
  myServo.attach(PIN_SERVO);
  delay(50);
  myServo.write(angle);
  delay(600);   // tiempo para que llegue a la posicion
  currentAngle = angle;
}

// ═══════════════════════════════════════════════════════════
void loop() {

  if (timerActive && cmdLen > 0) {
    if ((millis() - lastByteTime) >= CMD_TIMEOUT_MS) {
      timerActive = false;
      Serial.println();
      cmdBuf[cmdLen] = '\0';
      executeCmd(cmdBuf);
      cmdLen = 0;
      memset(cmdBuf, 0, sizeof(cmdBuf));
      return;
    }
  }

  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    lastByteTime = millis();
    timerActive  = true;

    if ((uint8_t)c == CTRL_C) {
      Serial.println(F("\n[cancelado]"));
      cmdLen      = 0;
      waitAngle   = false;
      timerActive = false;
      memset(cmdBuf, 0, sizeof(cmdBuf));
      prompt();
      return;
    }

    if (c == '\b' || c == 127) {
      if (cmdLen > 0) {
        cmdLen--;
        cmdBuf[cmdLen] = '\0';
        Serial.print(F("\b \b"));
      }
      continue;
    }

    if (c == '\r' || c == '\n' || c == ';') {
      if (cmdLen == 0) { timerActive = false; continue; }
      timerActive = false;
      Serial.println();
      cmdBuf[cmdLen] = '\0';
      executeCmd(cmdBuf);
      cmdLen = 0;
      memset(cmdBuf, 0, sizeof(cmdBuf));
      delay(2);
      while (Serial.available() && Serial.peek() == '\n') Serial.read();
      return;
    }

    if (cmdLen < MAX_CMD_LEN) {
      cmdBuf[cmdLen++] = c;
      Serial.print(c);
    }
  }
}

// ═══════════════════════════════════════════════════════════
void executeCmd(const char* cmd) {

  if (waitAngle) {
    int a = toInt(cmd);
    if (a < 0 || a > 180) {
      Serial.println(F("Error: numero entre 0 y 180"));
      Serial.print(F("  angulo> "));
      return;
    }
    Serial.print(F("Moviendo servo a "));
    Serial.print(a);
    Serial.println(F(" grados..."));
    setServoAngle(a);
    Serial.println(F("OK"));
    waitAngle = false;
    prompt();
    return;
  }

  if (strcmp(cmd, "servo") == 0)  { doServo(); return; }

  if (sw(cmd, "sensor")) {
    const char* a = cmd + 6;
    while (*a == ' ') a++;
    int ms = *a ? toInt(a) : 1000;
    if (ms <= 0) ms = 1000;
    doSensor(ms);
    prompt();
    return;
  }

  if (sw(cmd, "welcome")) {
    const char* a = cmd + 7;
    while (*a == ' ') a++;
    doWelcome(a);
    prompt();
    return;
  }

  if (strcmp(cmd, "help")  == 0) { doHelp();  prompt(); return; }
  if (strcmp(cmd, "clear") == 0) {
    Serial.print(F("\033[2J\033[H"));
    prompt();
    return;
  }

  Serial.print(F("Desconocido: '"));
  Serial.print(cmd);
  Serial.println(F("' -> escriba 'help'"));
  prompt();
}

// ═══════════════════════════════════════════════════════════
void doServo() {
  Serial.print(F("Posicion actual: "));
  Serial.print(currentAngle);
  Serial.println(F(" grados"));
  Serial.println(F("Ingrese angulo (0-180) y presione Env:"));
  Serial.print(F("  angulo> "));
  waitAngle   = true;
  timerActive = false;
}

// ═══════════════════════════════════════════════════════════
void doSensor(int ms) {
  Serial.print(F("Sensor cada "));
  Serial.print(ms);
  Serial.println(F("ms | 'q'+Env para detener"));
  Serial.println(F("  t(ms)    raw   voltaje"));
  Serial.println(F("  ----------------------"));

  unsigned long t0 = millis();

  while (true) {
    if (Serial.available() > 0) {
      char c = (char)Serial.peek();
      if ((uint8_t)c == CTRL_C || c == 'q' || c == 'Q') {
        while (Serial.available()) Serial.read();
        Serial.println(F("\n[sensor detenido]"));
        return;
      }
      Serial.read();
    }

    unsigned long t   = millis() - t0;
    int           raw = analogRead(PIN_SENSOR);
    float         v   = raw * (5.0 / 1023.0);

    Serial.print(F("  "));
    Serial.print(t);
    Serial.print(F("ms\t"));
    Serial.print(raw);
    Serial.print(F("\t"));
    Serial.print(v, 2);
    Serial.println(F("V"));

    unsigned long fin = millis() + ms;
    while (millis() < fin) {
      if (Serial.available() > 0) {
        char c = (char)Serial.peek();
        if ((uint8_t)c == CTRL_C || c == 'q' || c == 'Q') {
          while (Serial.available()) Serial.read();
          Serial.println(F("\n[sensor detenido]"));
          return;
        }
        Serial.read();
      }
    }
  }
}

// ═══════════════════════════════════════════════════════════
void doWelcome(const char* args) {
  if (!args || strlen(args) == 0) {
    printBanner();
    return;
  }
  EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VAL);
  uint8_t i = 0;
  while (args[i] && i < EEPROM_MSG_LEN - 1) {
    EEPROM.write(EEPROM_MSG_ADDR + i, (uint8_t)args[i]);
    i++;
  }
  EEPROM.write(EEPROM_MSG_ADDR + i, 0);
  Serial.print(F("Guardado: \""));
  Serial.print(args);
  Serial.println('"');
}

void printBanner() {
  Serial.println(F("=============================="));
  Serial.println(F("  Arduino CLI - Parte I"));
  Serial.println(F("  Lab. Comunicacion Serial"));
  if (EEPROM.read(EEPROM_MAGIC_ADDR) == EEPROM_MAGIC_VAL) {
    char msg[EEPROM_MSG_LEN];
    for (uint8_t i = 0; i < EEPROM_MSG_LEN; i++) {
      msg[i] = (char)EEPROM.read(EEPROM_MSG_ADDR + i);
      if (!msg[i]) break;
    }
    Serial.print(F("  >> "));
    Serial.println(msg);
  }
  Serial.println(F("  Escriba 'help'"));
  Serial.println(F("=============================="));
}

void doHelp() {
  Serial.println(F("Comandos:"));
  Serial.println(F("  servo             angulo interactivo"));
  Serial.println(F("  sensor <ms>       leer potenciometro"));
  Serial.println(F("                    detener: 'q' + Env"));
  Serial.println(F("  welcome           ver mensaje EEPROM"));
  Serial.println(F("  welcome <texto>   guardar en EEPROM"));
  Serial.println(F("  help              esta ayuda"));
  Serial.println(F("  clear             limpiar pantalla"));
}

void prompt() { Serial.print(F("CLI> ")); }

// ═══════════════════════════════════════════════════════════
int toInt(const char* s) {
  if (!s || !*s) return -1;
  int r = 0;
  bool ok = false;
  for (uint8_t i = 0; s[i]; i++) {
    if (s[i] >= '0' && s[i] <= '9') {
      r = r * 10 + (s[i] - '0');
      ok = true;
    } else return -1;
  }
  return ok ? r : -1;
}

bool sw(const char* s, const char* p) {
  while (*p) { if (*s++ != *p++) return false; }
  return true;
}
