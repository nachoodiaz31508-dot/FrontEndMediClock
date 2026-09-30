#ifndef MEDICLOCK_CONFIG_H
#define MEDICLOCK_CONFIG_H

// Central hardware / network configuration for the MediClock ESP32 firmware.
// All pins below were chosen to avoid the SPI-flash bus (GPIO6-GPIO11) and
// the strapping pins GPIO0 / GPIO2 / GPIO15. See README.md wiring table.

// ---------------------------------------------------------------- I2C bus
// ESP32 default hardware I2C bus. Shared by the LCD only (DS1302 is 3-wire).
#define PIN_I2C_SDA        21
#define PIN_I2C_SCL        22
#define LCD_I2C_ADDR       0x27
#define LCD_COLS           16
#define LCD_ROWS           2

// ------------------------------------------------------- RTC DS1302 (3-wire)
// NOTE on GPIO5: it is a strapping pin that must read HIGH at reset.
// The DS1302 RST line idles LOW and is driven HIGH only during a transfer,
// plus the board-level pull-up keeps boot state HIGH, so this use is safe.
// Never tie GPIO5 to GND.
#define PIN_DS1302_DAT     19   // DAT (IO)
#define PIN_DS1302_CLK     18   // CLK (SCLK)
#define PIN_DS1302_RST     5    // RST (CE)

// --------------------------------------- Stepper 28BYJ-48 + ULN2003 driver
// Driven through a ULN2003 with a SEPARATE 5 V supply (see README).
// NOTE on GPIO12: it is a strapping pin (MTDI) that must read LOW at reset
// to select the correct flash voltage. The ULN2003 input stage does not
// pull the line HIGH at boot, so IN2 on GPIO12 is safe — but do NOT add an
// external pull-up on this line.
#define PIN_STEPPER_IN1    13
#define PIN_STEPPER_IN2    12   // strapping pin: keep free of pull-ups
#define PIN_STEPPER_IN3    14
#define PIN_STEPPER_IN4    27
#define STEPPER_STEPS_PER_REV 2048  // 28BYJ-48 in half-step mode

// ------------------------------------------------------- Reed switch (endstop)
// GPIO34 is input-only (no output driver, no internal pull-up/down).
// An EXTERNAL 10 k pull-up to 3.3 V is mandatory; the reed contact closes to GND.
// INPUT_PULLUP is requested for intent but has no effect on GPIO34-39.
#define PIN_REED_SWITCH    34

// ------------------------------------------------------- Panic button (offline)
// Same electrical constraints as the reed switch: GPIO35 is input-only with
// no internal pull-up, so an EXTERNAL 10 k pull-up to 3.3 V is mandatory.
// Active LOW. Works offline (no WiFi required) — handled in T5.
#define PIN_PANIC_BUTTON   35

// ------------------------------------------------------------------ Outputs
#define PIN_LED            23   // status LED, active HIGH
#define PIN_BUZZER         26   // passive buzzer driven via LEDC PWM
#define BUZZER_LEDC_CHANNEL 0
#define BUZZER_LEDC_FREQ_HZ 2000
#define BUZZER_LEDC_RES_BITS 8

// ------------------------------------------------------------------ Network
// PLACEHOLDERS ONLY — never commit real credentials (see repo policy).
#define WIFI_SSID          "TU_SSID"
#define WIFI_PASSWORD      "TU_PASSWORD"
#define BACKEND_URL        "https://tu-back/api"
#define BACKEND_ALARMS_PATH "/alarmas"
#define BACKEND_EVENT_PATH  "/eventos"

// ---------------------------------------------------------------------- NTP
// America/Argentina/Buenos_Aires (UTC-3, no DST).
#define NTP_SERVER         "pool.ntp.org"
#define NTP_GMT_OFFSET_SEC (-3 * 3600)
#define NTP_DAYLIGHT_OFFSET_SEC 0
#define NTP_SYNC_INTERVAL_MS (6UL * 3600UL * 1000UL)  // re-sync every 6 h

// ---------------------------------------------------------- Scheduler timing
#define RTC_CACHE_INTERVAL_MS   1000   // T2: RTC re-read at most once per second
#define WIFI_RETRY_INTERVAL_MS  10000  // T4: non-blocking reconnect window
#define ALARMS_FETCH_INTERVAL_MS (5UL * 60UL * 1000UL) // T4: GET /alarmas every 5 min

#endif // MEDICLOCK_CONFIG_H
