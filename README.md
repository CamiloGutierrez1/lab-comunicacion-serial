# Laboratorio Comunicacion Serial

[![Compilar Sketches Arduino](https://github.com/CamiloGutierrez1/lab-comunicacion-serial/actions/workflows/compile.yml/badge.svg?branch=main)](https://github.com/CamiloGutierrez1/lab-comunicacion-serial/actions/workflows/compile.yml)

Laboratorio de comunicacion serial con Arduino Uno. Implementacion de terminal CLI interactiva e interfaz HMI con teclado matricial y LCD.

## Integrantes

| # | Nombre |
|---|---|
| 1 | Camilo Andres Gutierrez Barriga |
| 2 | Daniel Felipe Sanabria Solano |
| 3 | Jorge Andres Rodriguez Huertas |
| 4 | Santiago Pulido Herrera |

---

## Estructura del repositorio

```
lab-comunicacion-serial/
├── parte1/
│   └── terminal_cli/
│       └── terminal_cli.ino   # Terminal CLI interactiva
├── parte2/
│   ├── terminal/
│   │   └── terminal.ino       # Arduino Terminal (servo + sensor)
│   └── hmi/
│       └── hmi.ino            # Arduino HMI (teclado + LCD)
└── .github/
    └── workflows/
        └── compile.yml        # CI: compila automaticamente en cada push
```

---

## Parte I — Terminal CLI Interactiva

**Simulacion Tinkercad:** https://www.tinkercad.com/things/g74mvMYleHn-incredible-allis/editel?returnTo=https%3A%2F%2Fwww.tinkercad.com%2Fdashboard&sharecode=VnFO2N1BSzmU1Nnm8N9X8Z8oMjiZI-ZkyWMpuKhRFq0

### Componentes
- Arduino Uno R3
- Micro Servo (pin 9)
- Potenciometro (A0)

### Comandos disponibles

| Comando | Descripcion |
|---|---|
| `servo` | Mueve el servomotor (pide angulo interactivo 0-180) |
| `sensor <ms>` | Lee el potenciometro cada `<ms>` milisegundos con timestamp |
| `welcome` | Muestra el mensaje de bienvenida guardado en EEPROM |
| `welcome <texto>` | Guarda un nuevo mensaje persistente en EEPROM |
| `help` | Lista de comandos disponibles |
| `clear` | Limpia la pantalla del terminal |

### Uso
Abrir el Serial Monitor a 9600 baudios. Escribir el comando y presionar Enter.
Para detener el comando `sensor` escribir `q` y presionar Enter.

---

## Parte II — HMI con Teclado y LCD

**Simulacion Tinkercad:** https://www.tinkercad.com/things/iRZ0GTj8bti/editel?returnTo=%2Fdashboard&sharecode=Sv-C9tNe6x_oZTMbKVGyDEd3JaCZ-UyHFLYh2C28Suw

### Componentes
**Arduino Terminal (arriba)**
- Arduino Uno R3
- Micro Servo (pin 9)
- Potenciometro (A0)

**Arduino HMI (abajo)**
- Arduino Uno R3
- Teclado matricial 4x4 (pines 8-13, A0, A1)
- LCD 16x2 (RS=2, E=3, DB4=4, DB5=5, DB6=6, DB7=7)

### Conexion entre Arduinos
| Arduino Terminal | Arduino HMI |
|---|---|
| TX (pin 1) | RX (pin 0) |
| RX (pin 0) | TX (pin 1) |
| GND | GND |

### Teclas del HMI

| Tecla | Funcion |
|---|---|
| `0-9` | Escribir numeros |
| `#` | Enter — enviar comando |
| `*` | Backspace |
| `A` | Atajo servo (escribe SERVO:, luego angulo + #) |
| `B` | Atajo sensor 1000ms |
| `C` solo | Ver mensaje EEPROM |
| `numeros` + `C` | Guardar texto como mensaje EEPROM |
| `D` | Limpiar pantalla |

---

## Parte III — Factibilidad a 20 metros

Investigacion sobre la viabilidad de la comunicacion serial a 20 metros con cable AWG 24. Ver documento adjunto en el repositorio.

---

## CI/CD — GitHub Actions

El repositorio incluye un workflow que compila automaticamente los tres sketches en cada `push` que modifique archivos `.ino`.

- `parte1/terminal_cli/terminal_cli.ino` — compilado para Arduino Uno
- `parte2/terminal/terminal.ino` — compilado para Arduino Uno
- `parte2/hmi/hmi.ino` — compilado para Arduino Uno (con librerias Keypad y LiquidCrystal)