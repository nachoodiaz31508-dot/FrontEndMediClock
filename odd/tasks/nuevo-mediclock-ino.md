# Feature: nuevo-mediclock-ino — Firmware ESP32 desde cero

## Objetivo
Rehacer el firmware Mediclock para ESP32 con WiFi, sin botones de configuración, con botón físico de pánico y reed switch como final de carrera.

## Problema
El `.ino` actual es de Arduino UNO (pines 2-13, EEPROM, DS1302 bit-bang, 3 botones, `Menu()` local). No compila ni funciona en ESP32 y no habla con el back/front.

## Por qué desde cero
Son muchos cambios transversales (pines, NVS, NTP+RTC, WiFi HTTP, scheduler sin `s==0`, stepper no bloqueante con reed). Reescribir evita arrastrar defines de UNO y deja el loop no bloqueante limpio. Se preserva la lógica probada: cache RTC 1s, `TiempoCumplido()`, sistema de mensajes, formato EEPROM 7x3.

## Alcance autorizado
- Crear carpeta `nuevo_mediclock_ino/` con `.ino` + `config.h` + `README.md` + `api.h/.cpp` si hace falta.
- NO tocar `MediclockIno` original remoto ni el front Blazor en este feature.
- NO commitear credenciales WiFi ni URLs con tokens. Usar placeholders.

## Restricciones
- Alimentación 5V separada motor/lógica. ESP32 a 3.3V.
- Mantener DS1302 (no migrar a DS3231).
- RTC como respaldo, NTP como fuente principal cuando hay WiFi.
- ESP32: pide alarmas al back (deploy), las guarda local, dispara componentes, reporta evento de alarma.
- Reed switch = final de carrera para posición exacta de aspas.
- Botón pánico físico = silenciar/entregar aunque no haya WiFi.

## Checklist
- [x] T1 — Scaffold `nuevo_mediclock_ino/`: `.ino` base que compila, `config.h` con pines ESP32 seguros, README cableado. (ruta: delegada, trigger: writer 2+ files)
- [x] T2 — Tiempo: cache RTC 1s + sync NTP periódico + fallback + LCD reloj.
- [x] T3 — Alarmas NVS + scheduler con ventana (no `s==0`) + flag ya-disparada.
- [x] T4 — WiFi + HTTP: GET alarmas back, POST evento alarma, reconexión no bloqueante.
- [ ] T5 — Actuadores: stepper no bloqueante + reed endstop + buzzer/LED patrón + botón pánico.
- [ ] T6 — Integración loop final + verificación + docs.

## Criterios de aceptación
- Compila para ESP32 (o documenta por qué no se pudo compilar en este host).
- Sin `delay()` bloqueantes en loop; sin `Menu()` por botones; sin `EEPROM.read/write`.
- Pines ESP32 válidos (evitar 6-11 flash, strapping 0/2/15) y documentados.
- Funciona offline con NVS+RTC si se cae WiFi.

## Checks aplicables
- `arduino-cli compile --fqbn esp32:esp32:esp32 nuevo_mediclock_ino` si toolchain disponible, sino readback estructural.
- `git diff --stat` por work-unit commit.

## TDD
- Modo: OFF para firmware (sin test runner host en este repo Blazor). Fuente: sin config de tests embebidos + decisión sesión. Runner: N/A. Checks ordinarios: compilación + readback.

## Delivery
- Estrategia: `ask-on-risk` (default). Forecast: ~550-650 líneas autoradas (scaffold ~150, tiempo ~80, NVS+scheduler ~100, WiFi+HTTP ~150, actuadores ~120). Supera heurística 400 → se avisará antes de PR; por ahora work-unit commits en rama feature, sin PR.
- Running: ~860 líneas (T1 361 + T2 ~120 + T3 ~140 + T4 ~245). Chain strategy: pendiente (solo si se pide PR).

## Progreso
- 2026-09-30: T1 done en `814359b` — scaffold 3 archivos, 361 inserciones, sin delay/Menu/EEPROM reales, arduino-cli pendiente (no instalado en host). Next: T2 tiempo.
- 2026-09-30: T1-ES pasada a español didáctico (renombres tiempoCacheado/apagarBuzzer, comentarios y README en español neutro, pines e intervalos intactos, solo placeholders).
- 2026-09-30: T2 done sin commit — leerTiempo() con cache 1s + validación IsDateTimeValid/LastError/rango (rtcValido/horaValida), sincronizarNTP() con configTime() una vez en setup + getLocalTime con reintento 30 s sin hora / 6 h con hora + SetDateTime al RTC, mostrarReloj() en LCD ("Mie 07:30"/fecha, "SIN HORA", "SYNC" 2 s), config.h +3 defines sin tocar pines, solo time.h estándar. Verificado por readback (sin delay/Menu/EEPROM reales, placeholders intactos); arduino-cli ausente, compilación pendiente en equipo preparado. Next: T3 NVS+scheduler.
- 2026-09-30: T3 done sin commit — NVS `mediclock` 7x3 (`a{d}s{s}h/m/e` + marca `alarm_init`), `guardar/leerAlarmaLocal` con validación, `inicializarNVSsiVacio()` (21 celdas deshabilitadas), `verificarAlarmas()` por ventana [hh:mm] con `horaValida`, marca diaria en RAM liberada al cambiar de día, `alarmaPendiente` para T5 + `!ALARMA!` en LCD (mostrarReloj lo respeta). Verificado por readback (sin delay/EEPROM/Menu reales); compilación pendiente. Next: T4 WiFi+HTTP.
- 2026-10-01: T4 done sin commit — `atenderWiFi()` reconexión no bloqueante (`WiFi.begin()` solo en su turno, primer intento inmediato, `WiFi.mode(WIFI_STA)` en setup), GET cada 5 min con timeout 5 s a `BACKEND_URL + BACKEND_ALARMS_PATH` con parseo manual defensivo (formato real back `{diaSemana 1-7, numeroAlarma 1-3, hora "HH:mm:ss"}` + formato simple `{dia, slot, hora, minuto, habilitada}`; conversión dia = diaSemana % 7, slot = numero - 1), lo válido a NVS vía `guardarAlarmaLocal()`, errores HTTP/JSON por Serial sin borrar NVS; POST evento `{dia, slot, hora, minuto, fecha}` con reintento no bloqueante + `EventoPendiente` (`encolarEventoParaEnvio` / `hayEventoPendiente` / `marcarEventoEnviado`) para T5; `+HTTP_TIMEOUT_MS` en config.h, pines y loop intactos. Verificado por readback (sin delay/EEPROM/Menu reales, placeholders intactos); compilación pendiente (sin arduino-cli en host). Next: T5 actuadores.

## Decisiones
- Reescritura desde cero aceptada: preserva ideas probadas, descarta defines UNO.
- Convención firmware: igual que front — español neutro/profesional en dominio (nombres, declaraciones, comentarios), ordenado y didáctico simple. T1 quedó en inglés y se adapta desde T2.
