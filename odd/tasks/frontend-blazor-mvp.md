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

## Tasks
- [x] T1: Instalar SDK .NET 8 y verificar `dotnet --version`
- [x] T2: Andamiar Blazor WASM en ./ (net8.0) y `dotnet build` verde
- [ ] T3: Modelos DTO + ApiClient con BASE_URL configurable + selector dispositivos
- [ ] T4: UI estado + CRUD alarmas con validacion client-side
- [ ] T5: UI historial eventos con paginacion + manejo errores
- [x] T6: Crear repo privado FrontEndMediClock, commit inicial y push

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

## Next step
- Ejecutar T1-T2-T6 via writer unico, luego T3-T5.

## Route declaration
- Route: delegated direct (mapping trigger ya usado para backend 14 endpoints; writer trigger: 2+ files scaffold). No SDD.
