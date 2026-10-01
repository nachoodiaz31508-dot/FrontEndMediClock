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
#include <time.h>  // hora NTP del ESP32 (getLocalTime/configTime, sin librerías extra)

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
unsigned long ultimaSincNTPms = 0;    // última sincronización NTP exitosa
unsigned long ultimoIntentoNTPms = 0;  // último intento NTP (exitoso o no)
unsigned long ultimaConsultaMs = 0;
unsigned long ultimoReintentoWiFims = 0;
unsigned long ultimaPantallaRelojms = 0;  // último dibujo del reloj en LCD
unsigned long avisoSyncHastaMs = 0;  // muestra "SYNC" hasta este momento

// Estado de la hora (T2): rtcValido dice si el DS1302 responde bien;
// horaValida dice si tiempoCacheado se puede usar (RTC válido o NTP reciente).
bool rtcValido = false;
bool horaValida = false;
bool ntpConfigurado = false;  // configTime() se llama una sola vez en setup()

// ------------------------------------------- T3: estado de alarmas en NVS
// Aviso pendiente que T5 consume para mover el motor y sonar el buzzer.
// verificarAlarmas() lo deja activo; T5 lo atiende y lo pone en false.
struct AvisoAlarma {
  bool activa = false;
  uint8_t dia = 0;     // 0 = domingo, igual que DayOfWeek del RTC
  uint8_t slot = 0;    // 0..2
  uint8_t hora = 0;
  uint8_t minuto = 0;
};
AvisoAlarma alarmaPendiente;

// Marca ya-disparada en memoria: cada celda dispara una sola vez por día.
// Se libera al cambiar de día (ver verificarAlarmas).
bool yaDisparo[7][3] = {{false}};
uint16_t diaMarcaAnio = 0;
uint8_t diaMarcaMes = 0;
uint8_t diaMarcaDia = 0;

// ---------------------------------------------------------- Utilidades
void apagarBuzzer() {
  ledcWrite(BUZZER_LEDC_CHANNEL, 0);
}

// ------------------------------------------ T2: hora (RTC en memoria + NTP)
// Revisa que una fecha del RTC sea posible (pila agotada o primer
// arranque devuelven años absurdos). Solo valida el calendario.
bool esFechaPosible(const RtcDateTime& t) {
  if (t.Year() < 2024 || t.Year() > 2099) return false;
  if (t.Month() == 0 || t.Month() > 12) return false;
  if (t.Day() == 0 || t.Day() > 31) return false;
  return true;
}

// Lee el DS1302 una vez por segundo y lo guarda en tiempoCacheado.
// Así el resto del programa usa el dato en memoria sin frenar el bucle.
// Si el RTC falla (oscilador detenido o error de lectura) no se confía
// en sus registros: se marca horaValida = false para que NTP tome el relevo.
void leerTiempo() {
  unsigned long ahora = millis();
  if (ahora - ultimaLecturaRTCms < RTC_CACHE_INTERVAL_MS) {
    return;
  }
  ultimaLecturaRTCms = ahora;
  RtcDateTime lectura = rtc.GetDateTime();
  if (!rtc.IsDateTimeValid() || rtc.LastError() != 0 || !esFechaPosible(lectura)) {
    rtcValido = false;
    horaValida = false;
    return;
  }
  rtcValido = true;
  horaValida = true;
  tiempoCacheado = lectura;
}

// Trae la hora por NTP cuando hay WiFi y ya pasó el intervalo.
// Con hora válida sincroniza cada NTP_SYNC_INTERVAL_MS; sin hora válida
// reintenta cada NTP_REINTENTO_SIN_HORA_MS hasta lograr la primera.
// Al lograrla ajusta el RTC para seguir funcionando sin WiFi.
void sincronizarNTP() {
  if (!ntpConfigurado || WiFi.status() != WL_CONNECTED) {
    return;
  }
  unsigned long ahora = millis();
  bool toca = horaValida
      ? (ultimaSincNTPms == 0 || ahora - ultimaSincNTPms >= NTP_SYNC_INTERVAL_MS)
      : (ultimoIntentoNTPms == 0 || ahora - ultimoIntentoNTPms >= NTP_REINTENTO_SIN_HORA_MS);
  if (!toca) {
    return;
  }
  ultimoIntentoNTPms = ahora;
  struct tm partes;
  // Espera breve (máximo 1,5 s y solo en su turno): no frena el bucle.
  if (!getLocalTime(&partes, 1500)) {
    return;  // sin respuesta del servidor, se reintenta luego
  }
  RtcDateTime desdeNTP(partes.tm_year + 1900, partes.tm_mon + 1, partes.tm_mday,
                       partes.tm_hour, partes.tm_min, partes.tm_sec);
  rtc.SetDateTime(desdeNTP);  // el RTC guarda la hora como respaldo offline
  tiempoCacheado = desdeNTP;
  ultimaLecturaRTCms = ahora;
  ultimaSincNTPms = ahora;
  rtcValido = true;
  horaValida = true;
  avisoSyncHastaMs = ahora + LCD_SYNC_AVISO_MS;  // aviso breve en pantalla
}

// Nombres cortos de día en español (DayOfWeek: 0 = domingo).
const char* DIAS_ES[7] = {"Dom", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab"};

// Dibuja el reloj en el LCD 16x2, refresco ~1 s sin bloquear.
// Línea 0: "Mie 07:30" · Línea 1: "01/10/2026" (+ " SYNC" al sincronizar).
// Sin hora válida muestra "SIN HORA" hasta que llegue NTP.
void mostrarReloj() {
  unsigned long ahora = millis();
  if (alarmaPendiente.activa) {
    return;  // el aviso "!ALARMA!" queda en pantalla hasta que T5 lo consuma
  }
  if (ahora - ultimaPantallaRelojms < LCD_REFRESH_INTERVAL_MS) {
    return;
  }
  ultimaPantallaRelojms = ahora;
  char texto[17];
  char linea[17];
  if (!horaValida) {
    snprintf(linea, sizeof(linea), "%-16s", "SIN HORA");
    lcd.setCursor(0, 0);
    lcd.print(linea);
    snprintf(linea, sizeof(linea), "%-16s", "Esperando NTP");
    lcd.setCursor(0, 1);
    lcd.print(linea);
    return;
  }
  snprintf(texto, sizeof(texto), "%s %02d:%02d",
           DIAS_ES[tiempoCacheado.DayOfWeek() % 7],
           tiempoCacheado.Hour(), tiempoCacheado.Minute());
  snprintf(linea, sizeof(linea), "%-16s", texto);
  lcd.setCursor(0, 0);
  lcd.print(linea);
  snprintf(texto, sizeof(texto), "%02d/%02d/%04d",
           tiempoCacheado.Day(), tiempoCacheado.Month(), tiempoCacheado.Year());
  if (ahora < avisoSyncHastaMs) {
    char conAviso[17];
    snprintf(conAviso, sizeof(conAviso), "%s SYNC", texto);  // 15 letras, entra en 16
    snprintf(linea, sizeof(linea), "%-16s", conAviso);
  } else {
    snprintf(linea, sizeof(linea), "%-16s", texto);
  }
  lcd.setCursor(0, 1);
  lcd.print(linea);
}

// ------------------------------ T3: planificador de alarmas (por ventana)
// Espacio NVS "mediclock" (ver prefs.begin en setup). Formato de claves:
//   a{d}s{s}h : hora (0-23, UChar) · a{d}s{s}m : minuto (0-59, UChar)
//   a{d}s{s}e : habilitada (bool). d = día 0-6 (0 = domingo, igual que
//   DayOfWeek del RTC), s = slot 0-2. Ejemplo: "a3s1h" = hora del
//   miércoles, slot 1. Clave "alarm_init" (bool): marca de NVS ya iniciado.
// Sin EEPROM: solo Preferences (NVS).

// Arma la clave NVS de una celda ("a3s1h"). buf necesita al menos 7 letras.
void claveAlarma(uint8_t dia, uint8_t slot, char tipo, char* buf, size_t largo) {
  snprintf(buf, largo, "a%us%u%c", dia, slot, tipo);
}

// Guarda una alarma con validación de rangos. Si algún dato está fuera
// de rango devuelve false y no escribe nada.
bool guardarAlarmaLocal(uint8_t dia, uint8_t slot, uint8_t hora, uint8_t minuto, bool habilitada) {
  if (dia > 6 || slot > 2 || hora > 23 || minuto > 59) {
    return false;
  }
  char clave[8];
  claveAlarma(dia, slot, 'h', clave, sizeof(clave));
  prefs.putUChar(clave, hora);
  claveAlarma(dia, slot, 'm', clave, sizeof(clave));
  prefs.putUChar(clave, minuto);
  claveAlarma(dia, slot, 'e', clave, sizeof(clave));
  prefs.putBool(clave, habilitada);
  return true;
}

// Lee una alarma. Devuelve false con día/slot fuera de rango.
// Si la celda nunca se escribió, entrega 0:00 deshabilitada.
bool leerAlarmaLocal(uint8_t dia, uint8_t slot, uint8_t &hora, uint8_t &minuto, bool &habilitada) {
  if (dia > 6 || slot > 2) {
    return false;
  }
  char clave[8];
  claveAlarma(dia, slot, 'h', clave, sizeof(clave));
  hora = prefs.getUChar(clave, 0);
  claveAlarma(dia, slot, 'm', clave, sizeof(clave));
  minuto = prefs.getUChar(clave, 0);
  claveAlarma(dia, slot, 'e', clave, sizeof(clave));
  habilitada = prefs.getBool(clave, false);
  return true;
}

// Primer arranque: si NVS no tiene la marca, crea las 21 celdas
// deshabilitadas (0:00, sin horarios inventados) y deja la marca.
void inicializarNVSsiVacio() {
  if (prefs.isKey("alarm_init")) {
    return;
  }
  for (uint8_t dia = 0; dia < 7; dia++) {
    for (uint8_t slot = 0; slot < 3; slot++) {
      guardarAlarmaLocal(dia, slot, 0, 0, false);
    }
  }
  prefs.putBool("alarm_init", true);
}

// Muestra el aviso como hacía el original ("!ALARMA!").
// No bloquea: solo escribe y vuelve; mostrarReloj() lo respeta
// mientras alarmaPendiente siga activa (la consume T5).
void mostrarAlarmaEnLCD(uint8_t dia, uint8_t slot, uint8_t hora, uint8_t minuto) {
  char linea[17];
  snprintf(linea, sizeof(linea), "%-16s", "!ALARMA!");
  lcd.setCursor(0, 0);
  lcd.print(linea);
  snprintf(linea, sizeof(linea), "D%u S%u %02u:%02u", dia, slot, hora, minuto);
  char linea2[17];
  snprintf(linea2, sizeof(linea2), "%-16s", linea);
  lcd.setCursor(0, 1);
  lcd.print(linea2);
}

// Revisa las 3 alarmas del día actual contra la ventana [hh:mm] del
// minuto en curso. Sin hora válida no dispara nada. Cada celda dispara
// una sola vez por día (marca yaDisparo, liberada al cambiar de día);
// dentro del minuto las pasadas repetidas del loop no re-disparan.
// Al disparar deja alarmaPendiente para T5 y avisa en el LCD.
void verificarAlarmas() {
  if (!horaValida) {
    return;  // RTC caído y sin NTP: no se confía en la hora
  }
  // Cambio de día: se liberan las marcas para disparar de nuevo.
  uint16_t anio = tiempoCacheado.Year();
  uint8_t mes = tiempoCacheado.Month();
  uint8_t hoy = tiempoCacheado.Day();
  if (anio != diaMarcaAnio || mes != diaMarcaMes || hoy != diaMarcaDia) {
    memset(yaDisparo, 0, sizeof(yaDisparo));
    diaMarcaAnio = anio;
    diaMarcaMes = mes;
    diaMarcaDia = hoy;
  }
  uint8_t dia = tiempoCacheado.DayOfWeek() % 7;
  uint8_t hh = tiempoCacheado.Hour();
  uint8_t mm = tiempoCacheado.Minute();
  for (uint8_t slot = 0; slot < 3; slot++) {
    if (yaDisparo[dia][slot]) {
      continue;  // esta celda ya disparó hoy
    }
    uint8_t ah = 0, am = 0;
    bool hab = false;
    if (!leerAlarmaLocal(dia, slot, ah, am, hab) || !hab) {
      continue;
    }
    // Ventana del minuto actual [hh:mm]: dispara en cualquier segundo
    // del minuto, nunca con s == 0 (ese era el error del original).
    if (ah == hh && am == mm) {
      yaDisparo[dia][slot] = true;
      if (!alarmaPendiente.activa) {
        alarmaPendiente.activa = true;
        alarmaPendiente.dia = dia;
        alarmaPendiente.slot = slot;
        alarmaPendiente.hora = ah;
        alarmaPendiente.minuto = am;
      }
      mostrarAlarmaEnLCD(dia, slot, ah, am);
      Serial.printf("[mediclock] alarma d%u s%u %02u:%02u\n", dia, slot, ah, am);
    }
  }
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

  // Fuente principal de hora: se configura una sola vez al arrancar.
  // sincronizarNTP() solo lee cuando toca, sin bloquear el bucle.
  configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  ntpConfigurado = true;

  // Primera lectura con la misma validación que leerTiempo():
  // si el DS1302 perdió la hora no se confía en sus registros
  // y sincronizarNTP() la traerá cuando haya WiFi.
  ultimaLecturaRTCms = millis();
  tiempoCacheado = rtc.GetDateTime();
  if (!rtc.IsDateTimeValid() || rtc.LastError() != 0 || !esFechaPosible(tiempoCacheado)) {
    rtcValido = false;
    horaValida = false;
  } else {
    rtcValido = true;
    horaValida = true;
  }

  stepper.setSpeed(12);  // rpm; el giro real sigue por pasos no bloqueantes en T5
  prefs.begin("mediclock", false);
  inicializarNVSsiVacio();  // T3: primer arranque deja las 21 celdas deshabilitadas

  Serial.println(F("[mediclock] setup done"));
}

// -------------------------------------------------------- Bucle principal
void loop() {
  // Cada función decide sola si ya es su turno; ninguna detiene a las demás.
  leerTiempo();
  sincronizarNTP();
  mostrarReloj();
  verificarAlarmas();
  atenderWiFi();
  actualizarActuadores();
}
