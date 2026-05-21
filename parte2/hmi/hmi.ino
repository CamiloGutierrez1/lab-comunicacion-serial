/*
 * ============================================================
 *  Laboratorio Comunicacion Serial - Parte II
 *  Arduino HMI (abajo)
 *  - LCD 16x2 simple: RS=2, E=3, DB4=4, DB5=5, DB6=6, DB7=7
 *  - Teclado 4x4: filas=8,9,10,11 columnas=12,13,A0,A1
 *  - Envia comandos al Terminal por Serial
 * ============================================================
 *  Teclas:
 *    0-9  escribir numeros
 *    #    Enter - enviar comando
 *    *    Backspace
 *    A    atajo SERVO (escribe "SERVO:" y pide angulo)
 *    B    atajo SENSOR 1000ms
 *    C    atajo WELCOME ver mensaje
 *    D    limpiar pantalla
 * ============================================================
 */

#include <LiquidCrystal.h>
#include <Keypad.h>

// ── LCD ────────────────────────────────────────────────────
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

// ── Teclado ────────────────────────────────────────────────
const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {8,  9,  10, 11};
byte colPins[COLS] = {12, 13, A0, A1};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ── Buffer comando ─────────────────────────────────────────
#define MAX_CMD 16
char    cmdBuf[MAX_CMD + 1];
uint8_t cmdLen = 0;

// ── Buffer respuesta ───────────────────────────────────────
#define MAX_RESP 40
char          respBuf[MAX_RESP + 1];
uint8_t       respLen    = 0;
unsigned long lastRespBy = 0;
bool          respActive = false;

// ── Prototipos ─────────────────────────────────────────────
void handleKey(char key);
void sendCmd(const char* cmd);
void parseResp(const char* resp);
void lcdRow(uint8_t row, const char* text);
void lcdClearRow(uint8_t row);
bool sw(const char* s, const char* p);

// ═══════════════════════════════════════════════════════════
void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcdRow(0, "Arduino HMI");
  lcdRow(1, "Listo!");
  delay(1500);
  lcdRow(0, "CMD>");
  lcdClearRow(1);
}

// ═══════════════════════════════════════════════════════════
void loop() {

  // ── Teclado ───────────────────────────────────────────────
  char key = keypad.getKey();
  if (key) handleKey(key);

  // ── Recibir respuesta del Terminal ────────────────────────
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    lastRespBy = millis();
    respActive = true;

    if (c == '\n' || c == '\r') {
      if (respLen > 0) {
        respBuf[respLen] = '\0';
        parseResp(respBuf);
        respLen    = 0;
        respActive = false;
        memset(respBuf, 0, sizeof(respBuf));
      }
      continue;
    }
    if (respLen < MAX_RESP) respBuf[respLen++] = c;
  }

  // Timeout respuesta
  if (respActive && respLen > 0 &&
      (millis() - lastRespBy) >= 300) {
    respBuf[respLen] = '\0';
    parseResp(respBuf);
    respLen    = 0;
    respActive = false;
    memset(respBuf, 0, sizeof(respBuf));
  }
}

// ═══════════════════════════════════════════════════════════
void handleKey(char key) {

  // # = Enter: enviar
  if (key == '#') {
    if (cmdLen > 0) {
      cmdBuf[cmdLen] = '\0';
      sendCmd(cmdBuf);
      cmdLen = 0;
      memset(cmdBuf, 0, sizeof(cmdBuf));
      lcdRow(0, "Enviando...");
    }
    return;
  }

  // * = Backspace
  if (key == '*') {
    if (cmdLen > 0) {
      cmdLen--;
      cmdBuf[cmdLen] = '\0';
      // Actualizar fila 0
      char line[17];
      snprintf(line, sizeof(line), "CMD>%s", cmdBuf);
      lcdRow(0, line);
    }
    return;
  }

  // A = atajo servo
  if (key == 'A') {
    cmdLen = 0;
    memset(cmdBuf, 0, sizeof(cmdBuf));
    const char* pre = "SERVO:";
    while (*pre && cmdLen < MAX_CMD) cmdBuf[cmdLen++] = *pre++;
    lcdRow(0, "SERVO:_____");
    lcdRow(1, "angulo + #");
    return;
  }

  // B = atajo sensor
  if (key == 'B') {
    sendCmd("SENSOR:1000");
    lcdRow(0, "Sensor 1000ms");
    lcdRow(1, "Leyendo...");
    return;
  }

  // C = atajo welcome ver
  if (key == 'C') {
    sendCmd("WELCOME:");
    lcdRow(0, "EEPROM:");
    lcdRow(1, "Leyendo...");
    return;
  }

  // D = limpiar
  if (key == 'D') {
    cmdLen = 0;
    memset(cmdBuf, 0, sizeof(cmdBuf));
    lcdRow(0, "CMD>");
    lcdClearRow(1);
    return;
  }

  // 0-9 = agregar al buffer
  if (cmdLen < MAX_CMD) {
    cmdBuf[cmdLen++] = key;
    char line[17];
    snprintf(line, sizeof(line), "CMD>%s", cmdBuf);
    lcdRow(0, line);
  }
}

// ═══════════════════════════════════════════════════════════
void sendCmd(const char* cmd) {
  Serial.print(cmd);
  Serial.print('\n');
}

// ═══════════════════════════════════════════════════════════
void parseResp(const char* resp) {

  // OK:servo:90
  if (sw(resp, "OK:servo:")) {
    char line[17];
    snprintf(line, sizeof(line), "Servo:%s gr", resp + 9);
    lcdRow(0, "OK servo");
    lcdRow(1, line);
    return;
  }

  // DATA:t:raw:voltaje
  if (sw(resp, "DATA:")) {
    char buf[MAX_RESP];
    strncpy(buf, resp + 5, sizeof(buf));
    // saltar tiempo
    char* p = buf;
    while (*p && *p != ':') p++;
    if (*p) p++;
    // raw
    char* rawStr = p;
    while (*p && *p != ':') p++;
    if (*p) { *p = '\0'; p++; }
    // voltaje
    char line[17];
    snprintf(line, sizeof(line), "%s  %sV", rawStr, p);
    lcdRow(0, "Sensor:");
    lcdRow(1, line);
    return;
  }

  // DONE:sensor
  if (sw(resp, "DONE:sensor")) {
    lcdRow(0, "Sensor listo");
    lcdClearRow(1);
    return;
  }

  // OK:welcome:texto
  if (sw(resp, "OK:welcome:")) {
    lcdRow(0, "Bienvenida:");
    lcdRow(1, resp + 11);
    return;
  }

  // OK:guardado:texto
  if (sw(resp, "OK:guardado:")) {
    lcdRow(0, "Guardado OK");
    lcdRow(1, resp + 12);
    return;
  }

  // ERR
  if (sw(resp, "ERR:")) {
    lcdRow(0, "Error!");
    lcdRow(1, resp + 4);
    return;
  }

  // Cualquier otro
  lcdRow(1, resp);
}

// ═══════════════════════════════════════════════════════════
//  Helpers LCD
// ═══════════════════════════════════════════════════════════
void lcdRow(uint8_t row, const char* text) {
  lcd.setCursor(0, row);
  uint8_t i = 0;
  while (i < 16 && text[i]) { lcd.print(text[i]); i++; }
  while (i < 16) { lcd.print(' '); i++; }
}

void lcdClearRow(uint8_t row) {
  lcdRow(row, "");
}

bool sw(const char* s, const char* p) {
  while (*p) { if (*s++ != *p++) return false; }
  return true;
}
