// MediClock ESP32 firmware — scaffold (T1).
// Non-blocking super-loop: every task is polled on a millis() schedule.
// No delay() in loop(). No button-driven Menu(). No EEPROM (NVS via Preferences).

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <LiquidCrystal_I2C.h>
#include <Stepper.h>

#include "config.h"

// ------------------------------------------------------------ Global objects
ThreeWire ds1302Wire(PIN_DS1302_DAT, PIN_DS1302_CLK, PIN_DS1302_RST);
RtcDS1302<ThreeWire> rtc(ds1302Wire);
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);
Stepper stepper(STEPPER_STEPS_PER_REV,
                PIN_STEPPER_IN1, PIN_STEPPER_IN3,
                PIN_STEPPER_IN2, PIN_STEPPER_IN4);
Preferences prefs;  // NVS namespace for cached alarms (T3)

// Cached time, refreshed at most once per second (T2).
RtcDateTime cachedTime;
unsigned long lastRtcReadMs = 0;
unsigned long lastNtpSyncMs = 0;
unsigned long lastFetchMs = 0;
unsigned long lastWifiRetryMs = 0;

// ------------------------------------------------------------------ Helpers
void buzzOff() {
  ledcWrite(BUZZER_LEDC_CHANNEL, 0);
}

// ------------------------------------------------- T2: time (RTC cache + NTP)
// Reads the DS1302 at most once per second into cachedTime and re-syncs
// from NTP every NTP_SYNC_INTERVAL_MS when WiFi is up. Full logic lands in T2.
void leerTiempo() {
  unsigned long now = millis();
  if (now - lastRtcReadMs < RTC_CACHE_INTERVAL_MS) {
    return;
  }
  lastRtcReadMs = now;
  cachedTime = rtc.GetDateTime();
  // TODO(T2): validate cachedTime, fallback handling, LCD clock rendering.
}

// TODO(T2/T4): pull NTP -> RTC when WiFi is connected and interval elapsed.
void sincronizarNTP() {
  unsigned long now = millis();
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  if (lastNtpSyncMs != 0 && now - lastNtpSyncMs < NTP_SYNC_INTERVAL_MS) {
    return;
  }
  // TODO(T2): configTime() once at boot; here update RTC from NTP + set lastNtpSyncMs.
}

// --------------------------------------- T3: alarm scheduler (window match)
// Fires each due alarm once (ya-disparada flag in NVS). Full logic lands in T3.
void verificarAlarmas() {
  // TODO(T3): load alarms from NVS, match [hh:mm] window (never s == 0),
  // set fired flag, call dispense routine in T5.
}

// ------------------------------------------------- T4: WiFi + HTTP backend
// Non-blocking reconnect + periodic GET /alarmas and POST /eventos.
// Full logic lands in T4.
void atenderWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  unsigned long now = millis();
  if (now - lastWifiRetryMs < WIFI_RETRY_INTERVAL_MS) {
    return;
  }
  lastWifiRetryMs = now;
  // TODO(T4): WiFi.begin(WIFI_SSID, WIFI_PASSWORD) without blocking;
  // on connect: GET BACKEND_URL + BACKEND_ALARMS_PATH -> NVS (every
  // ALARMS_FETCH_INTERVAL_MS), POST fired events to BACKEND_EVENT_PATH.
}

// ---------------------------------- T5: actuators (stepper, buzzer, buttons)
// Non-blocking stepper homing on reed endstop, buzzer/LED pattern,
// offline panic button. Full logic lands in T5.
void actualizarActuadores() {
  // TODO(T5): reed-switch homing (PIN_REED_SWITCH, active LOW),
  // single-step non-blocking rotation, LEDC buzzer pattern,
  // panic button (PIN_PANIC_BUTTON) silences/dispenses offline.
}

// -------------------------------------------------------------------- Setup
void setup() {
  Serial.begin(115200);

  pinMode(PIN_REED_SWITCH, INPUT_PULLUP);   // needs external 10 k pull-up (GPIO34)
  pinMode(PIN_PANIC_BUTTON, INPUT_PULLUP);  // needs external 10 k pull-up (GPIO35)
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  ledcSetup(BUZZER_LEDC_CHANNEL, BUZZER_LEDC_FREQ_HZ, BUZZER_LEDC_RES_BITS);
  ledcAttachPin(PIN_BUZZER, BUZZER_LEDC_CHANNEL);
  buzzOff();

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("MediClock boot");

  rtc.Begin();
  // TODO(T2): if RTC lost confidence (!IsDateTimeValid / LastError),
  // flag for NTP sync instead of trusting the registers.
  cachedTime = rtc.GetDateTime();

  stepper.setSpeed(12);  // rpm; actual stepping stays non-blocking in T5
  prefs.begin("mediclock", false);

  Serial.println(F("[mediclock] setup done"));
}

// --------------------------------------------------------------------- Loop
void loop() {
  leerTiempo();
  sincronizarNTP();
  verificarAlarmas();
  atenderWiFi();
  actualizarActuadores();
}
