# MediClock — Firmware ESP32 (`nuevo_mediclock_ino`)

Firmware del dispensador de medicamentos MediClock para ESP32. Lee la hora de
un RTC DS1302 (con NTP como fuente principal cuando hay WiFi), guarda las
alarmas en memoria local, mueve el dispensador con un motor paso a paso y avisa
con buzzer, LED y pantalla LCD. Funciona sin internet gracias al respaldo local.

## Qué hace el ESP32 (su rol en el sistema)

El ESP32 es el único que toca el hardware. El servidor (back) solo guarda la
lista de alarmas que carga el familiar desde la app. El ciclo es siempre este:

1. **GET de alarmas** — cada 5 minutos pide `GET <BACKEND_URL>/alarmas` (T4).
2. **Guarda local** — copia lo válido en NVS (`Preferences`, espacio
   `mediclock`, 7 días x 3 turnos). Lo que falla o falta no borra nada local,
   así el equipo sigue con lo último bueno (T3).
3. **Dispara solo** — compara la hora del RTC con las alarmas guardadas por
   ventana de minuto (nunca con `segundo == 0`) y con marca ya-disparada, para
   que cada alarma se ejecute una sola vez por día aunque el loop pase muchas
   veces dentro del mismo minuto (T3).
4. **Mueve y avisa** — busca el origen con el reed, gira la dosis, suena el
   buzzer intermitente con el LED a la par y muestra `!ALARMA!` / `Dosis
   entregada` en el LCD (T5).
5. **POST de evento** — avisa cada disparo con `POST <BACKEND_URL>/eventos`.
   Si no hay WiFi, el evento queda en cola (de a uno, el nuevo reemplaza al
   anterior) y se reintenta en el próximo turno sin bloquear nada (T4).

Prioridad de hora: **NTP cuando hay WiFi, RTC DS1302 como respaldo**.
El RTC se lee como máximo una vez por segundo (dato en memoria, T2).

## Tabla de cableado final

| Parte | Pin ESP32 | Notas |
| --- | --- | --- |
| LCD 16x2 I2C (PCF8574) SDA | GPIO21 | Bus I2C por hardware, dir. `0x27` |
| LCD 16x2 I2C (PCF8574) SCL | GPIO22 | Bus I2C por hardware |
| DS1302 DAT | GPIO19 | Lógica 3,3 V; Vcc del módulo según su hoja de datos, GND común |
| DS1302 CLK | GPIO18 | — |
| DS1302 RST (CE) | GPIO5 | Pin de arranque: debe leer HIGH al encender. La línea queda en LOW y solo sube durante cada lectura; el pull-up de placa la mantiene en HIGH al arrancar. Nunca a GND |
| Motor IN1 (ULN2003) | GPIO13 | Vía ULN2003, ver alimentación |
| Motor IN2 (ULN2003) | GPIO12 | Pin de arranque (MTDI): debe leer LOW al encender. La entrada del ULN2003 queda en LOW al arrancar — NO agregar pull-up externo aquí |
| Motor IN3 (ULN2003) | GPIO14 | — |
| Motor IN4 (ULN2003) | GPIO27 | — |
| Reed (final de carrera) | GPIO34 | Solo entrada, sin pull-up interno: **pull-up externo de 10 k a 3V3 sí o sí**. El contacto cierra a GND (activo en LOW) |
| Botón de pánico | GPIO35 | Igual que el reed: solo entrada, **pull-up externo de 10 k a 3V3**, activo en LOW |
| LED de estado | GPIO23 | Activo en HIGH (usar serie de 220 Ω) |
| Buzzer pasivo | GPIO26 | Por PWM LEDC canal 0 a 2 kHz |

No usados: GPIO6–GPIO11 (flash SPI interna), GPIO0/GPIO2/GPIO15 (arranque, se
dejan libres para un encendido confiable).

## Alimentación

- **Fuentes de 5 V separadas con GND común:** una solo para el motor (riel del
  motor / pin `COM` del ULN2003) y otra para la lógica (USB de la placa o pin
  5 V). Unir los GND de ambas fuentes en un solo punto.
- El 28BYJ-48 consume más de lo que el regulador de la placa entrega con carga:
  **nunca alimentar el motor desde el regulador de la placa**.
- El DS1302 mantiene la hora con su pila botón cuando se corta la energía.
- El ESP32 trabaja en 3,3 V; todas las señales de control son de 3V3. No
  conectar 5 V directo a ningún GPIO.

## Contrato JSON con el servidor

Los valores reales de URL y clave WiFi **nunca** se guardan en el repo: en
`config.h` solo hay marcadores (`TU_SSID`, `TU_PASSWORD`, `https://tu-back/api`).
Cada placa se configura antes de cargarla, sin commitear.

**GET** `BACKEND_URL + BACKEND_ALARMS_PATH` → `200` con lista JSON.
Formato real del back (.NET, ver `Modelos/Alarma.cs` del front):

```json
[
  {"alarmaId": 2, "diaSemana": 2, "numeroAlarma": 1, "hora": "08:00:00"}
]
```

- `diaSemana` 1–7 (1 = lunes, 7 = domingo). Conversión interna:
  `dia = diaSemana % 7` (el domingo 7 pasa a 0, igual que `DayOfWeek` del RTC).
- `numeroAlarma` 1–3. Conversión interna: `slot = numeroAlarma - 1`.
- Toda alarma listada se toma como habilitada (el back no trae bandera).
- También se acepta el formato simple `{dia 0-6, slot 0-2, hora 0-23,
  minuto 0-59, habilitada 0/1}`; si falta `habilitada` se asume 1.

**POST** `BACKEND_URL + BACKEND_EVENT_PATH` con:

```json
{"dia": 3, "slot": 1, "hora": 7, "minuto": 30, "fecha": "01/10/2026"}
```

`fecha` en formato `DD/MM/AAAA` tomado del RTC al momento del disparo
(`00/00/0000` si no había hora válida). Se acepta `200`, `201` o `202` como
éxito; otro código deja el evento en cola para el próximo turno.

## Calibración

- **Pasos de dosis (`DISPENSAR_PASOS`, valor inicial 273):** es el giro del
  motor después de encontrar el origen. Poner una pastilla de prueba, forzar
  una alarma cercana o tocar pánico con alarma pendiente y mirar si cae una
  sola dosis. Si cae de más, bajar el número; si no cae, subirlo. Cambiar solo
  ese define y volver a cargar.
- **Reed (origen):** pegar el imán en el aspa de modo que cierre el reed una
  sola vez por vuelta. Con el equipo encendido, girar el aspa a mano y mirar el
  Serial: al pasar por el origen el homing termina rápido. Si el Serial dice
  `reed sin marcar` siempre, revisar distancia imán–reed (acercar a 3–5 mm),
  continuidad del cable y el pull-up de 10 k.
- **Buzzer (`BUZZER_DUTY`, inicial 128 de 0–255):** subir para más volumen,
  bajar si molesta. El patrón intermitente es fijo de 500 ms on/off con el LED
  a la par (`BUZZER_PARPADEO_MS`).

## Puesta en marcha paso a paso

1. Cablear todo **sin energía**: motor a su fuente de 5 V, lógica por USB,
   GND común, pull-ups de 10 k en GPIO34 y GPIO35 a 3V3.
2. En `config.h` poner el SSID/clave WiFi y la URL del back de **esta**
   instalación (sin commitear).
3. Compilar y cargar (ver abajo). Abrir el monitor serial a 115200.
4. Debe verse `MediClock boot` en el LCD y `[mediclock] setup done` en Serial.
5. Sin hora inicial el LCD muestra `SIN HORA` / `Esperando NTP`: conectar el
   WiFi y esperar el primer sync (el LCD agrega `SYNC` 2 s al sincronizar).
6. Cargar una alarma próxima desde la app y esperar el GET (o reiniciar para
   forzar la primera descarga). El Serial informa `alarmas del back: N
   guardadas`.
7. Al llegar la hora: `!ALARMA!` en LCD, homing con reed, giro de dosis,
   buzzer/LED intermitentes, `Dosis entregada` 1 s y luego el reloj retoma.
8. Probar el botón de pánico: con alarma sonando la confirma y calla; sin
   alarma solo muestra `MediClock listo` + hora 2 s, sin mover el motor.

## Solución de problemas

| Síntoma | Causa probable | Qué hacer |
| --- | --- | --- |
| LCD fijo en `SIN HORA` | Sin WiFi o NTP bloqueado; RTC con pila agotada | Revisar SSID/clave y que el router tenga internet; confirmar `pool.ntp.org` accesible; cambiar la pila del DS1302 y esperar el reintento (cada 30 s sin hora) |
| `reed sin marcar` en Serial siempre | Imán lejos, cable cortado o sin pull-up | Acercar imán a 3–5 mm del reed; medir continuidad; verificar pull-up 10 k a 3V3 en GPIO34; el equipo dispensa igual para no trabarse, pero sin origen exacto |
| WiFi caído (`WiFi conectando...` en loop) | Router apagado o clave mal cargada | No tocar nada: el equipo sigue con NVS + RTC y reintenta solo cada 10 s; el evento queda en cola y se envía al volver la red |
| Alarma no dispara a su hora | Hora inválida, celda deshabilitada o día distinto | Ver que el LCD dé la hora real (no `SIN HORA`); revisar en Serial el `d/m/hh:mm` del disparo; confirmar en la app que la alarma sea para hoy |
| Motor vibra pero no gira | Fuente débil o secuencia IN1–IN4 mal conectada | Usar la fuente de 5 V separada solo para el motor; revisar orden IN1/IN2/IN3/IN4 según tabla; GND común entre fuentes |
| Buzzer no suena / LED no prende | Pin o volumen | Revisar GPIO26/23; subir `BUZZER_DUTY`; el patrón solo suena con alarma pendiente (ver `!ALARMA!` en LCD) |

## Cómo compilar

Requiere [arduino-cli](https://arduino.github.io/arduino-cli/) con el núcleo
ESP32 y estas bibliotecas (las del proyecto original más las propias del ESP32,
sin dependencias nuevas):

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

> Nota de entorno: `arduino-cli` no está instalado en este host (verificado
> el 2026-10-01 con `which arduino-cli`), por eso T1–T6 se verificaron por
> lectura estructural (includes, setup/loop, sin `delay()` / `Menu()` /
> `EEPROM` reales, marcadores intactos) y no por compilación. Correr el comando
> de arriba en un equipo preparado antes de cargar a placa. Queda como
> pendiente explícito.
