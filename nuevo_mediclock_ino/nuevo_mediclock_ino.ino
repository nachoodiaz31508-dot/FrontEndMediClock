// Firmware MediClock para ESP32 — base (T1).
// Bucle principal no bloqueante: cada tarea se atiende por turnos según millis().
// Sin delay() en loop(). Sin menú por botones. Sin EEPROM (se usa NVS con Preferences).

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <LiquidCrystal_I2C.h>
#include <Stepper.h>

#include "config.h"

// ------------------------------------------------------ Objetos globales
ThreeWire ds1302Wire(PIN_DS1302_DAT, PIN_DS1302_CLK, PIN_DS1302_RST);
RtcDS1302<ThreeWire> rtc(ds1302Wire);
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);
Stepper stepper(STEPPER_STEPS_PER_REV,
                PIN_STEPPER_IN1, PIN_STEPPER_IN3,
                PIN_STEPPER_IN2, PIN_STEPPER_IN4);
Preferences prefs;  // Espacio NVS donde se guardan las alarmas (T3)

// Hora en memoria: se lee del RTC como máximo una vez por segundo (T2).
RtcDateTime tiempoCacheado;
unsigned long ultimaLecturaRTCms = 0;
unsigned long ultimaSincNTPms = 0;
unsigned long ultimaConsultaMs = 0;
unsigned long ultimoReintentoWiFims = 0;

// ---------------------------------------------------------- Utilidades
void apagarBuzzer() {
  ledcWrite(BUZZER_LEDC_CHANNEL, 0);
}

// ------------------------------------------ T2: hora (RTC en memoria + NTP)
// Lee el DS1302 una vez por segundo y lo guarda en tiempoCacheado.
// Así el resto del programa usa el dato en memoria sin frenar el bucle.
// La sincronización con NTP ajusta el RTC cuando hay WiFi. Lógica completa en T2.
void leerTiempo() {
  unsigned long ahora = millis();
  if (ahora - ultimaLecturaRTCms < RTC_CACHE_INTERVAL_MS) {
    return;
  }
  ultimaLecturaRTCms = ahora;
  tiempoCacheado = rtc.GetDateTime();
  // TODO(T2): validar tiempoCacheado, definir respaldo si el RTC falla y mostrar reloj en LCD.
}

// TODO(T2/T4): traer la hora por NTP cuando haya WiFi y ya pasó el intervalo.
void sincronizarNTP() {
  unsigned long ahora = millis();
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }
  if (ultimaSincNTPms != 0 && ahora - ultimaSincNTPms < NTP_SYNC_INTERVAL_MS) {
    return;
  }
  // TODO(T2): configTime() una vez al arrancar; aquí actualizar el RTC desde NTP y guardar ultimaSincNTPms.
}

// ------------------------------ T3: planificador de alarmas (por ventana)
// Cada alarma dispara una sola vez (marca ya-disparada en NVS). Lógica completa en T3.
void verificarAlarmas() {
  // TODO(T3): leer alarmas desde NVS, comparar ventana [hh:mm] (nunca con s == 0),
  // marcar disparada y llamar a la rutina de dispenser en T5.
}

// ------------------------------------------------- T4: WiFi + servidor
// Reconexión no bloqueante + GET /alarmas periódico y POST /eventos.
// Si no hay WiFi, el equipo sigue con lo guardado en NVS. Lógica completa en T4.
void atenderWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  unsigned long ahora = millis();
  if (ahora - ultimoReintentoWiFims < WIFI_RETRY_INTERVAL_MS) {
    return;
  }
  ultimoReintentoWiFims = ahora;
  // TODO(T4): WiFi.begin(WIFI_SSID, WIFI_PASSWORD) sin bloquear;
  // al conectar: GET BACKEND_URL + BACKEND_ALARMS_PATH -> NVS (cada
  // ALARMS_FETCH_INTERVAL_MS) y POST de eventos a BACKEND_EVENT_PATH.
}

// --------------------------- T5: actuadores (motor, buzzer, botones)
// Motor con reed como final de carrera, patrón de buzzer/LED y botón de pánico.
// Todo avanza por pasos cortos sin bloquear. Lógica completa en T5.
void actualizarActuadores() {
  // TODO(T5): llevar a origen con reed (PIN_REED_SWITCH, activo en LOW),
  // giro por pasos sin bloquear, patrón de buzzer por LEDC y
  // botón de pánico (PIN_PANIC_BUTTON) que dispensa sin WiFi.
}

// --------------------------------------------------------------- Arranque
void setup() {
  Serial.begin(115200);

  // GPIO34/35 son solo entrada y no tienen pull-up interno:
  // llevan pull-up externo de 10 k a 3,3 V y activan en LOW (a GND).
  pinMode(PIN_REED_SWITCH, INPUT_PULLUP);
  pinMode(PIN_PANIC_BUTTON, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  ledcSetup(BUZZER_LEDC_CHANNEL, BUZZER_LEDC_FREQ_HZ, BUZZER_LEDC_RES_BITS);
  ledcAttachPin(PIN_BUZZER, BUZZER_LEDC_CHANNEL);
  apagarBuzzer();

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("MediClock boot");

  rtc.Begin();
  // TODO(T2): si el RTC perdió validez (!IsDateTimeValid / LastError),
  // marcar para sincronizar por NTP en vez de confiar en sus registros.
  tiempoCacheado = rtc.GetDateTime();

  stepper.setSpeed(12);  // rpm; el giro real sigue por pasos no bloqueantes en T5
  prefs.begin("mediclock", false);

  Serial.println(F("[mediclock] setup done"));
}

// -------------------------------------------------------- Bucle principal
void loop() {
  // Cada función decide sola si ya es su turno; ninguna detiene a las demás.
  leerTiempo();
  sincronizarNTP();
  verificarAlarmas();
  atenderWiFi();
  actualizarActuadores();
}
