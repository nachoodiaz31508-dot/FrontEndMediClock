# MediClock — ESP32 Firmware (`nuevo_mediclock_ino`)

Scaffold (T1) of the rewritten MediClock firmware for ESP32. Non-blocking
super-loop, WiFi-connected, no configuration buttons, no EEPROM.

## Role of the ESP32

1. **GET alarms** — polls `GET <BACKEND_URL>/alarmas` every 5 minutes (T4).
2. **Cache locally** — stores the alarm list in NVS (`Preferences`, namespace
   `mediclock`) so the device keeps working offline (T3).
3. **Fire** — matches the RTC time against the cached alarms with a time
   window (never `second == 0`) and a fired-flag so each alarm triggers
   exactly once (T3 + T5).
4. **POST event** — reports each fired alarm to `POST <BACKEND_URL>/eventos`
   when WiFi is up; events are retried later if offline (T4).

Time source priority: **NTP when WiFi is available, DS1302 RTC as fallback**
(T2). The RTC is re-read at most once per second (cached).

## Wiring

| Part | ESP32 pin | Notes |
| --- | --- | --- |
| LCD 16x2 I2C (PCF8574) SDA | GPIO21 | Default I2C bus |
| LCD 16x2 I2C (PCF8574) SCL | GPIO22 | Default I2C bus, addr `0x27` |
| DS1302 DAT | GPIO19 | 3.3 V logic; module Vcc to 3V3 or 5 V per module spec, common GND |
| DS1302 CLK | GPIO18 | — |
| DS1302 RST (CE) | GPIO5 | Strapping pin, must boot HIGH: board pull-up keeps it HIGH; never tie to GND |
| Stepper IN1 (ULN2003) | GPIO13 | Via ULN2003, see power note |
| Stepper IN2 (ULN2003) | GPIO12 | Strapping pin (MTDI), must boot LOW: ULN2003 input floats LOW at boot — do NOT add a pull-up here |
| Stepper IN3 (ULN2003) | GPIO14 | — |
| Stepper IN4 (ULN2003) | GPIO27 | — |
| Reed switch (endstop) | GPIO34 | Input-only, active LOW to GND + **external 10 k pull-up to 3V3** (no internal pull-up on GPIO34–39) |
| Panic button | GPIO35 | Same as reed: input-only, active LOW + **external 10 k pull-up to 3V3** |
| Status LED | GPIO23 | Active HIGH (use series 220 Ω resistor) |
| Buzzer (passive) | GPIO26 | Driven by LEDC PWM channel 0 @ 2 kHz |

Avoided: GPIO6–GPIO11 (SPI flash), strapping GPIO0/GPIO2/GPIO15 (left
untouched for reliable boot).

## Power

- **Separate 5 V supplies (common GND):** one for the stepper motor
  (ULN2003 `COM`/motor rail), one for logic (ESP32 devkit USB or 5 V pin).
- The 28BYJ-48 draws more than the ESP32 5 V pin can source reliably under
  load — **never power the motor from the devkit regulator**.
- DS1302 keeps time on its backup battery when main power is off.
- ESP32 runs at 3.3 V logic; all control signals above are 3V3-safe.

## Reed switch as endstop

The reed contact is mounted so the dispenser blade magnet closes it once per
revolution (home position). Homing (T5) rotates the stepper one step at a
time (non-blocking) until the reed reads LOW, then stops — giving an exact,
repeatable blade position without counting steps open-loop.

## Offline panic button

The panic button (GPIO35, active LOW) is serviced locally in `loop()` and
does **not** require WiFi: it silences the buzzer and forces a dispense cycle
so the patient gets the dose even with the backend unreachable. The event is
queued and POSTed once connectivity returns.

## How to build

Requires [arduino-cli](https://arduino.github.io/arduino-cli/) with the ESP32
core and these libraries (no new dependencies beyond the original project
plus ESP32 built-ins):

- `WiFi`, `HTTPClient`, `Preferences` (ESP32 core built-ins)
- `RtcDS1302` / `ThreeWire` — `makuna/Rtc`
- `LiquidCrystal_I2C` — `frank-de-brabander/LiquidCrystal_I2C`
- `Stepper` (Arduino built-in)

```bash
# one-time setup
arduino-cli core update-index
arduino-cli core install esp32:esp32
arduino-cli lib install "Rtc by Makuna" "LiquidCrystal I2C"

# compile
arduino-cli compile --fqbn esp32:esp32:esp32 nuevo_mediclock_ino

# flash (adjust port)
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 nuevo_mediclock_ino
```

> Host note: `arduino-cli` is not installed in this environment, so T1 was
> verified by structural readback (includes, setup/loop, no `delay()` /
> `Menu()` / `EEPROM`) rather than a toolchain build. Run the compile command
> above on a provisioned host.

## Roadmap

- T2 — cached RTC read + periodic NTP sync + LCD clock.
- T3 — NVS alarm store + window scheduler + fired flag.
- T4 — non-blocking WiFi reconnect + GET alarms + POST events.
- T5 — non-blocking stepper + reed homing + buzzer/LED + panic button.
- T6 — final loop integration + verification + docs.
