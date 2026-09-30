# MediClock — Firmware ESP32 (`nuevo_mediclock_ino`)

Base (T1) del firmware MediClock para ESP32. Bucle principal no bloqueante,
con WiFi, sin botones de configuración y sin EEPROM.

## Qué hace el ESP32

1. **GET de alarmas** — pide `GET <BACKEND_URL>/alarmas` cada 5 minutos (T4).
2. **Guarda local** — copia la lista en NVS (`Preferences`, espacio
   `mediclock`) para seguir funcionando sin internet (T3).
3. **Dispara** — compara la hora del RTC con las alarmas guardadas por ventana
   de tiempo (nunca con `segundo == 0`) y con marca ya-disparada, para que cada
   alarma se ejecute una sola vez (T3 + T5).
4. **POST de evento** — avisa cada disparo con `POST <BACKEND_URL>/eventos`
   cuando hay WiFi; si no hay conexión, lo reintenta más tarde (T4).

Prioridad de hora: **NTP cuando hay WiFi, RTC DS1302 como respaldo**
(T2). El RTC se lee como máximo una vez por segundo (dato en memoria).

## Cableado

| Parte | Pin ESP32 | Notas |
| --- | --- | --- |
| LCD 16x2 I2C (PCF8574) SDA | GPIO21 | Bus I2C por defecto |
| LCD 16x2 I2C (PCF8574) SCL | GPIO22 | Bus I2C por defecto, dir. `0x27` |
| DS1302 DAT | GPIO19 | Lógica 3,3 V; Vcc del módulo a 3V3 o 5 V según su hoja de datos, GND común |
| DS1302 CLK | GPIO18 | — |
| DS1302 RST (CE) | GPIO5 | Pin de arranque, debe iniciar en HIGH: el pull-up de placa lo mantiene en HIGH; nunca a GND |
| Motor IN1 (ULN2003) | GPIO13 | Vía ULN2003, ver nota de alimentación |
| Motor IN2 (ULN2003) | GPIO12 | Pin de arranque (MTDI), debe iniciar en LOW: la entrada del ULN2003 queda en LOW al arrancar — NO agregar pull-up |
| Motor IN3 (ULN2003) | GPIO14 | — |
| Motor IN4 (ULN2003) | GPIO27 | — |
| Reed (final de carrera) | GPIO34 | Solo entrada, activo en LOW a GND + **pull-up externo de 10 k a 3V3** (GPIO34–39 sin pull-up interno) |
| Botón de pánico | GPIO35 | Igual que el reed: solo entrada, activo en LOW + **pull-up externo de 10 k a 3V3** |
| LED de estado | GPIO23 | Activo en HIGH (usar serie de 220 Ω) |
| Buzzer (pasivo) | GPIO26 | Por PWM LEDC canal 0 a 2 kHz |

No usados: GPIO6–GPIO11 (flash SPI), pines de arranque GPIO0/GPIO2/GPIO15
(se dejan libres para un arranque confiable).

## Alimentación

- **Fuentes de 5 V separadas (GND común):** una para el motor
  (riel del motor / `COM` del ULN2003) y otra para la lógica (USB de la placa o pin 5 V).
- El 28BYJ-48 consume más de lo que el regulador de la placa entrega con carga:
  **nunca alimentar el motor desde el regulador de la placa**.
- El DS1302 mantiene la hora con su pila cuando se corta la energía.
- El ESP32 trabaja en 3,3 V; todas las señales de control son de 3V3.

## Reed como final de carrera

El contacto reed se monta para que el imán de la paleta lo cierre una vez por
vuelta (posición de origen). Al buscar origen (T5), el motor avanza de a un
paso por vez (sin bloquear) hasta que el reed lee LOW y se detiene. Así la
paleta queda siempre en la misma posición, sin contar pasos a ciegas.

## Botón de pánico sin internet

El botón de pánico (GPIO35, activo en LOW) se atiende en `loop()` y
**no** necesita WiFi: silencia el buzzer y fuerza un ciclo de dispenser
para que el paciente reciba la dosis aunque el servidor no responda. El evento
queda en cola y se envía (POST) cuando vuelve la conexión.

## Cómo compilar

Requiere [arduino-cli](https://arduino.github.io/arduino-cli/) con el núcleo
ESP32 y estas bibliotecas (sin dependencias nuevas salvo las del proyecto
original más las propias del ESP32):

- `WiFi`, `HTTPClient`, `Preferences` (incluidas en el núcleo ESP32)
- `RtcDS1302` / `ThreeWire` — `makuna/Rtc`
- `LiquidCrystal_I2C` — `frank-de-brabander/LiquidCrystal_I2C`
- `Stepper` (incluida en Arduino)

```bash
# instalación única
arduino-cli core update-index
arduino-cli core install esp32:esp32
arduino-cli lib install "Rtc by Makuna" "LiquidCrystal I2C"

# compilar
arduino-cli compile --fqbn esp32:esp32:esp32 nuevo_mediclock_ino

# cargar (ajustar puerto)
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 nuevo_mediclock_ino
```

> Nota de entorno: `arduino-cli` no está instalado aquí, por eso T1 se
> verificó por lectura estructural (includes, setup/loop, sin `delay()` /
> `Menu()` / `EEPROM`) y no por compilación. Correr el comando de arriba en
> un equipo preparado.

## Plan

- T2 — lectura RTC en memoria + sincronización NTP + reloj en LCD.
- T3 — guardado de alarmas en NVS + planificador por ventana + marca disparada.
- T4 — reconexión WiFi sin bloquear + GET alarmas + POST eventos.
- T5 — motor sin bloquear + origen con reed + buzzer/LED + botón de pánico.
- T6 — integración final del bucle + verificación + docs.
