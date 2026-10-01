# Frontend Blazor MVP — MediClock

## Objective
Crear frontend Blazor WASM .NET 8 separado que consume backend desplegado para programar alarmas y visualizar eventos del pastillero.

## Problem
Sin UI, el paciente/cuidador no puede configurar horarios ni ver reportes; ESP y backend ya existen pero falta la pata web.

## Why
Rol frontend del equipo de 4, stack convenio C#/.NET, deploy separado permitido por instituto.

## Scope
- Selector dinámico dispositivos (default id=1 ESP_32_Prueba)
- Ver estado + configuracion (GET configuracion)
- CRUD alarmas (dia 1-7, numero 1-3, hora HH:mm:ss, unique -> 409)
- Historial eventos paginado (pagina/tamanioPagina, TipoEvento exacta, descripcion 10-300)
- Manejo errores string[], 404/409/400, 204 sin contenido
- BASE_URL configurable (prod https://mediclockbackend.runasp.net, dev localhost)

Out of scope: auth, multi-usuario, ESP firmware, cambios backend.

## Constraints
- No romper contrato backend, backend fuente de verdad
- CORS * sin auth, JSON camelCase
- fechaHora la pone el server, no enviar
- Linux Mint 22.3, git ok, dotnet SDK 8 requerido
- Convencion instituto: codigo dominio en español neutro (nombres, declaraciones, comentarios) como el back, ordenado y facil de entender. Explicacion didactica simple en cada paso.

## Tasks
- [x] T1: Instalar SDK .NET 8 y verificar `dotnet --version`
- [x] T2: Andamiar Blazor WASM en ./ (net8.0) y `dotnet build` verde
- [x] T3: Modelos DTO + ApiClient con BASE_URL configurable + selector dispositivos
- [x] T4: UI estado + CRUD alarmas con validacion client-side
- [x] T5: UI historial eventos con paginacion + manejo errores
- [x] T5b: Fix dia 1=lunes..7=domingo + logo en header/nav/favicon
- [x] T6: Crear repo privado FrontEndMediClock, commit inicial y push
- [x] T7: Puntos front-only 4-7 (titulo con paciente, alias local, rediseno 2 columnas, formulario dia/hora amable)

## Authorized scope
Carpeta /home/joacoynacho/proyectos/MediClock para scaffold; instalacion SDK sistema; crear repo privado GitHub FrontEndMediClock via gh default auth. Push y PRs futuros a decision del usuario.

## Acceptance criteria
- `dotnet build` pasa
- App levanta, lista dispositivos, muestra configuracion id=1 real
- Crear alarma valida persiste y aparece; duplicada muestra 409 amable
- Eventos cargan paginados

## Applicable checks
- `dotnet build`
- `dotnet run` smoke + GET configuracion real
- Structural readback (trivial docs excluidos)

## Progress
- 2026-09-30: doc creado, 6 tasks. Env: Mint 22.3, git 2.43, dotnet missing -> instalar.
- 2026-09-30: T1+T2+T6 done. Commit 09f74a3 `feat: bootstrap blazor wasm frontend` pushed to main. Repo: https://github.com/nachoodiaz31508-dot/FrontEndMediClock (private). Install note: sudo interactivo no disponible en sesion -> SDK via script oficial Microsoft dotnet-install.sh --channel 8.0 a ~/.dotnet (desvio de apt-path, misma fuente). dev-certs --trust pendiente (requiere sudo/store, solo afecta run https local).

## Verification evidence
- `dotnet --version`: 8.0.425
- `dotnet --list-sdks`: 8.0.425 [/home/joacoynacho/.dotnet/sdk]
- `dotnet build`: Compilacion correcta, 0 Advertencia(s), 0 Errores (16.93s)
- `git log --oneline -1`: 09f74a3 feat: bootstrap blazor wasm frontend
- `gh repo view --json name,visibility,url`: pendiente re-verificacion (repo creado via `gh repo create FrontEndMediClock --private --source=. --push` -> https://github.com/nachoodiaz31508-dot/FrontEndMediClock)
- T3 (2026-09-30): `dotnet build` verde (0 advertencias, 0 errores, 13.48s). Smoke: `dotnet run` sirve index 200 y appsettings.json con BaseUrl; GET real `api/dispositivos/1/configuracion` devuelve ESP_32_Prueba con 1 alarma. Commit `feat: modelos y cliente api con selector` pusheado a main.
- T4 (2026-09-30): `dotnet build` verde (0 advertencias, 0 errores, 15.23s). Rutas CRUD verificadas contra prod: POST `api/dispositivos/1/alarmas` -> 201, PUT `.../alarmas/2` -> 200, GET una -> 200, DELETE -> 204, lista final intacta (1 alarma). Duplicado -> 409 string[] amable; dia invalido -> 400 detalle de problema (parseado en cliente). Smoke `dotnet run` en :5199: index 200 con `<title>MediClock</title>` + `css/medclock.css`, appsettings.json con BaseUrl prod. UI: selector arriba, 3 tarjetas (dispositivo/cantidad/proxima), lista ordenada dia/numero, formulario crear/editar con validacion client-side (rango + duplicado previo), 409 duplicado y 409 borrado-con-eventos con mensajes amables, confirmacion en dos pasos antes de borrar. Paleta logo (marino #0f2f5b, celeste #29b6d1, teal #14a3a3, fondo #f4f7fb); logo archivo pendiente de subida por el usuario (placeholder texto + cruz). Commit `feat: ui estado y crud alarmas` pusheado a main.
- T5 (2026-09-30): `dotnet build` verde (0 advertencias, 0 errores, 15.08s). Seccion Historial en Home: tabla fechaHora/tipo/descripcion/alarma, tamanio 20, boton Cargar mas, error string[] en español, vacio amable, recarga al cambiar dispositivo. Smoke prod: GET `api/dispositivos/1/configuracion` -> ESP_32_Prueba (1 alarma dia 2 = martes); GET `api/dispositivos/1/eventos?pagina=1&tamanioPagina=20` -> 2 eventos (AlarmaActivada, DosisEntregada).
- Fix dia (2026-09-30): backend valida "entre 1 y 7 (Lunes a Domingo)" (400 probado con dia 8 y 0). Comentario Alarma.cs 1=lunes..7=domingo; Proxima usa ((DayOfWeek+6)%7)+1; nombres Lunes..Domingo en tarjeta/lista/borrado/formulario.
- Logo (2026-09-30): `wwwroot/img/mediclock-logo.png` en header MainLayout + NavMenu (img con alt "Logotipo de MediClock"); favicon apunta al logo (favicon.png queda como respaldo en wwwroot).
- T7 (2026-10-01, puntos 4-7 solo-front, sin backend/firmware): `dotnet build` verde (0 advertencias, 0 errores). Titulo "Alarmas de pastillero NOMBRE" con paciente editable en localStorage (`mediclock.nombrePaciente`, reserva comentada para perfil real del back). Alias por dispositivo en localStorage (`mediclock.alias.{id}`) sin PUT; nombre real del servidor como secundario si difiere. Rediseno: boton celeste #29b6d1 despliega formulario compartido crear/editar a la izquierda, lista compacta a la derecha, apilado en mobile (<=768px). Formulario: dia por nombre Lunes..Domingo (1-7), hora type=time convertida a HH:mm:ss; validaciones y 409 amable intactos. Commits `feat: nombre de paciente y alias...` + `feat: rediseno...` en main. Smoke prod: GET `api/dispositivos/1/configuracion` -> ESP_32_Prueba (2 alarmas: dia 2 nro 1 08:00:00, dia 3 nro 2 15:00:00); GET eventos pagina 1 -> 200.

## Next step
- T5 done. MVP frontend completo (T1-T6 + fix dia + logo). Sigue deploy o mejoras a decision del usuario.

## Route declaration
- Route: delegated direct (mapping trigger ya usado para backend 14 endpoints; writer trigger: 2+ files scaffold). No SDD.
