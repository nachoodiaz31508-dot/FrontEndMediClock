# MediClock — Dispensador automático de medicamentos

Proyecto académico: un equipo basado en ESP32 que entrega la medicación a su hora,
avisa con luz y sonido, y se configura desde una aplicación web. Funciona aunque se
corte internet, gracias a su memoria local y reloj de respaldo.

## Contenido del repo

| Carpeta | Qué es |
| --- | --- |
| `FrontMediclock/` | Aplicación web Blazor (la pantalla que usa la familia para cargar horarios y ver eventos). |
| `nuevo_mediclock_ino/` | Firmware del ESP32 en Arduino (`.ino`): pide los horarios al servidor, mueve el dispensador y reporta cada dosis. Tiene su propio `README.md` con el cableado paso a paso. |
| `odd/` | Documentos de trabajo del desarrollo (no necesarios para usar el proyecto). |

## Cómo se conectan las partes

```
App web (FrontMediclock) → carga horarios → Servidor (back-end en deploy)
Servidor → ESP32 pide horarios (GET) → los guarda en memoria
ESP32 → a la hora indicada: gira el motor, suena, prende el LED
ESP32 → avisa al servidor cada dosis entregada (POST)
```

## Puesta en marcha rápida

- **Probar la app web:** ver `FrontMediclock/` (requiere .NET 8).
- **Cargar el firmware:** abrir `nuevo_mediclock_ino/nuevo_mediclock_ino.ino` en Arduino IDE,
  completar WiFi y URL en `config.h` y subir a un ESP32 DEVKIT V1. El detalle de
  cables, librerías y calibración está en `nuevo_mediclock_ino/README.md`.

## Estado

Prototipo funcional en pruebas físicas: conecta a WiFi, descarga horarios, dispensa
con final de carrera (sensor reed) y reporta eventos. Mejoras previstas: portal
cautivo para cargar el WiFi desde el celular y botón de pánico con pulsación larga.
