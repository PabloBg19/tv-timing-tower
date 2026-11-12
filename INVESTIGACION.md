# Investigación previa — TV Timing Tower

Fecha: 2026-11-12. Objetivo: F1 25 Steam build 25251167 + 2026 Season Pack + Swofty Launcher 1.9.x
(instalado en este PC: **1.9.14**, `swofty --version`).

## 1. Swofty Launcher (docs f1.swofty.net y repo Swofty-Developments/SwoftyLauncher)

Fuentes leídas: el repo clonado en el tag `v1.9.14` (mismo que el launcher instalado) y su carpeta `docs/`
(que es exactamente el contenido de https://f1.swofty.net). Lo relevante:

### Addons y manifiesto (`docs/reference/addons/manifest.md`, `first-addon.md`, `publishing.md`)
- Un addon es una carpeta (o `.zip` / `.swoftyaddon`) con `swofty.json`. Campos: `schema_version`, `id`,
  `name`, `version`, `author`, `description`, `license`, `category`, `kind` (`ui`, `native`, …),
  `compat` (`game`, `game_builds`, `launcher`), `requires` (`swofty.core`), `experimental_features`,
  `options` (ajustes), `operations` (`database`, `hud`, `native`, …).
- Se instala arrastrando el zip a la página **Addons** o desde el **Addon Browser** (publicación vía PR al
  registro, `publishing.md`).
- Las "etiquetas" que pide el encargo (*Draws over the game*, *Code*) son las que el Addon Browser muestra a
  partir de `kind`/`experimental_features` (`native` → código, `overlay` → dibuja sobre el juego). Se reflejan
  también en `tags` del manifiesto propio (ver DECISIONES.md).

### Ajustes (ADDON SETTINGS) (`settings.md`)
- `options[]` con `type` `toggle` | `choice` | `slider`, `binds: "native"` (el DLL los lee en caliente con
  `SWOFTY_SETTING(tipo, id, defecto)` y `SWOFTY_ON_CHANGE`) o `binds: "db"` (solo en build, op files).
- Aparecen en el juego en **Carrera → ADDON SETTINGS** y en la fila del addon en el launcher.
- Los ficheros `hud` (registros `hud.element`) **no** admiten placeholders `{{id}}` (comprobado en
  `packages/core/src/hud/*.rs`): lo que cambie en caliente tiene que ir por hojas (leaves) del DLL.

### Plugins nativos / Advanced Mod Mixin System (`native-plugins.md`, `hooking.md`, `std/mixin.md`)
- DLL x64 compilada con `zig c++ -target x86_64-windows-gnu -std=c++23` (plantillas `templates/*/native/build.sh`),
  cabeceras MIT del SDK en `packages/sdk/include`.
- `SWOFTY_ON_LOAD`, `SWOFTY_ADDON()`, `swofty::events::on_frame(cb)` (hilo de flujo del juego), `swofty::log`.
- Hooks por nombre de la base de conocimiento (`SWOFTY_HOOK("Nombre_Funcion")`) con prioridades y
  encadenado (`chain`), resueltos contra `bindings.json` del core: si la build no coincide el hook no se
  instala y se registra en el log. Para este addon **no hace falta ningún hook**: todo lo que necesitamos lo
  da la API `live` (lectura) y el sistema de hojas del HUD (escritura).
- Launcher plugins (`launcher-api.md`): procesos aparte para páginas del launcher. No se necesitan.

### Estado en vivo (`std/live.md`) — fuente de datos elegida
- `swofty/live.hpp` lee la sesión **desde la memoria del juego**, sin UDP: orden de clasificación, posición,
  vuelta (`lap`, `lap_m`, `total_m`), estado de resultado (2 corriendo, 3 terminado, 4 DNF, 5 DSQ, 6 NC,
  7 retirado), `pit_state`/`pit_status`, neumático (`tyre_letter`), penalizaciones (`time_penalty_s`,
  `drive_through`, `stop_go`, `track_limit_warnings`), `gap_leader_ms`/`gap_ahead_ms` (solo en carrera), y
  de sesión: `type` (1–4 libres, 5–9 qualy, 10–14 sprint shootout, 15–17 carrera, 18 TT), `safety_car`
  (0/1 SC/2 VSC), `red_flag`, `yellow_sectors`, `chequered`, `order[]`, `road_order[]`.
- `live::team_badge(vehicle)` → clave de escudo del juego (p. ej. `ferrari_2026`).
- Requiere `"experimental_features": ["telemetry"]`. Cada grupo solo se activa si la build está verificada;
  si no, las llamadas devuelven vacío (fallo seguro).
- **Comprobado en los logs de este PC** (`%LOCALAPPDATA%\Swofty\sessions\*\game-logs\loader.log`):
  `[live] on (Steam build): groups car systems, ers, world, aero, progress, classification, timing, pit,
  penalties, tyres, flags, controllers, spectator, mode, badges, track, pit lane, track name, spectator gap,
  bus events` → todos los grupos que necesitamos están activos en tu instalación.
- **No disponible** en `live` 1.9.14: nombres de piloto, color de equipo, reloj de sesión, total de vueltas,
  tiempos de vuelta/sector. Los tres primeros se resuelven enlazando valores del propio view model del juego
  desde el HUD (abajo). Los tiempos de vuelta los calcula el addon (DECISIONES.md D5).

### HUD del juego: `hud.element` (`make/hud-element.md`, `ops/hud.md`, `ops/hud-screens.md`, `ops/hud-textures.md`, `ops/hud-motion.md`)
- Un registro `hud.element` con `"screen": "standings"` se compone **dentro del propio tablero de posiciones
  del juego** (Track Position): mientras la hoja `show` es verdadera el tablero nativo queda enmascarado y se
  ve el nuestro; si la hoja es falsa o el DLL no carga, vuelve el tablero original → **fallo seguro**.
- `also.replay` / `also.spectator` ponen la misma torre en repeticiones y modo espectador.
- `stock_nodes` oculta partes nativas (contador de vueltas, caja de bandera) mientras se ve la torre.
- Elementos: `sprite` (PNG, tamaños múltiplo de 4, 3 px por unidad HUD), `text` (estilos de fuente del juego:
  `OSD_24_bold`, `OSD_19_regular`, …), `group` con `repeat` (filas), `stack` (apilador nativo), `centre`,
  `unit` (`team_logo`, `series_logo`: logos del propio juego), bindings `view_model` (hojas propias),
  `session_car` (`driver.name_code`, `driver.second_name`, `nub_colour`, `position` por coche),
  `session: time_remaining_str` (reloj de sesión con el formateador nativo), `path: #session.number_of_laps`.
- Como forma parte del HUD nativo, el **editor de HUD del juego** permite mover, escalar y cambiar la opacidad
  del elemento, y se oculta cuando el juego oculta su HUD.
- `swofty hud check` y `swofty hud compose` validan todo contra los ficheros originales del juego.
  **Ejecutado en este PC: 90 ok, 0 failed**, y la plantilla `templates/hud-tower` compone sin errores.
- Límite: 1024 copias por registro (repeticiones).
- `motion` (animaciones con nodos nativos) existe pero la doc avisa: *"not yet seen in game"*.

### Hojas del HUD (`std/hud-leaves.md`, `hud-frame.md`, `hud-format.md`, `timing.md`)
- `hud::Leaves` declara hojas tipadas (bool, int, texto ≤ 200 B) bajo `view_model_data.hud_*`; el core solo
  envía cambios (≤ 256/tick), agrupa filas atómicas y re-envía tras reconstrucciones del view model.
- `fake_hud.hpp` / `fake_live.hpp` permiten tests en el host sin el juego.
- `swofty::timing` es un modelo de torre (portado de Project Halo) con contrato de hojas `hud_tt_*`.

### Overlay (`std/overlay.md`)
- API de dibujo 2D inmediata sobre el swapchain (rects, texto con Titillium Web SDF, PNG con `load_image`,
  transformaciones, opacidad), experimental `overlay`. Funciona en este PC con frame generation
  (logs: `[overlay] frame generation owns swapchain … draws into the game's back buffer`).
- No puede leer el view model del juego → no tiene nombres de piloto. Por eso **no** es el motor de la torre;
  se usa solo para el logo de la cadena (arriba a la derecha), que no tiene que ver con datos.

### PR #218 (Swofty-Developments/SwoftyLauncher)
- Filas de la base de conocimiento (L1, estáticas) sobre pit lane, safety car (lógica de despliegue,
  tether, red flag, ToRestart), marshals y track limits. Sin cambios de comportamiento en el core. Útil como
  referencia de que el estado "SC ending"/"in this lap" existe en la lógica del SC pero **no está expuesto**
  por `live`; por eso SC/VSC ENDING se infieren (DECISIONES.md D8).

## 2. Addons de referencia

| Addon | Cómo dibuja | Cómo obtiene datos | Qué aprendimos |
|---|---|---|---|
| **Race Broadcast** (apoc101) `gfx.c`, `hud_fix.c` | Overlay de Swofty para su torre/captions; `hud_fix.c` escanea patrones para mantener el HUD de espectador | **UDP** del propio juego en 127.0.0.1 | Patrón de opciones con tecla, overlay; el UDP está prohibido aquí |
| **Terminal Damage** (nyannoying) | Overlay + leaves del HUD | estado del jugador por memoria (`docs/player-state-api.md`) | Lecturas con fallo seguro y logging por build |
| **Spool-Up / grid start** (nyannoying) | Registro ERP propio + hojas `view_model_data.hud_spool_*` | memoria | Hojas del HUD; ya instalado aquí y funcionando (`[hud] set applied by nick.grid-start-2026`) |
| **Project Halo** (nyannoying, el de OverTake: halo 2026 + torre de retransmisión *con el propio HUD de F1 25*) | Escenas `.idf` y plantillas `.ui` del juego compuestas por sus herramientas; hoy Swofty integra ese mismo compositor (`hud.element`, `timing.hpp`, `hud_leaves.hpp` vienen de Halo con permiso) | Lector propio de memoria por RVAs de la build 25251167 (incl. tiempos de vuelta) | Contrato de hojas de la torre (`rNN_cVV` para enlazar nombres por coche), zona de eliminación Q1/Q2 = 10+(n−10)/2, "Interval/Leader", P de pit, animaciones probadas en juego. Licencia de atribución (uso privado sin obligación; publicación: "Uses work from Project Halo by nyannoying") |

Conclusión técnica: la vía soportada y probada para "una torre con el propio HUD del juego" es
`hud.element` (pantalla `standings`) + DLL con `swofty::live` escribiendo hojas.

## 3. Telemetría UDP 2026 de EA (solo como referencia semántica)
- Session: `m_sessionType` (mismo mapeo 1–18 que `live`), `m_safetyCarStatus` (0 ninguno, 1 SC, 2 VSC,
  3 vuelta de formación), `m_totalLaps`, `m_sessionTimeLeft`.
- Lap Data: `m_carPosition`, `m_currentLapNum`, `m_pitStatus` (0/1/2), `m_resultStatus` (0 inválido,
  1 inactivo, 2 activo, 3 terminado, 4 DNF, 5 DSQ, 6 NC, 7 retirado), `m_penalties`, `m_driverStatus`
  (garaje, vuelta lanzada, in/out lap).
- Event: `SCAR` con `eventType` 0 desplegado, 1 retirándose ("ending"), 2 retirado, 3 reanudación; `FTLP`
  vuelta rápida; `CHQF`; `RDFL`.
- Car Status: `m_visualTyreCompound` (16 S, 17 M, 18 H, 7 I, 8 W), `m_tyresAgeLaps`.
Usado para nombrar estados y textos (DNF, DSQ, NC, OUT) y para saber qué no da la memoria.

## 4. Diseño de la torre real (F1 TV / DAZN 2025–2026)
Referencias: la captura aportada (Hungría 2025, FP1), coberturas públicas
([GPFans 2026](https://www.gpfans.com/en/f1-news/1078428/f1-2026-tv-broadcast-new-timing-tower-graphic/),
[SI 2025](https://www.si.com/onsi/f1/news/f1-fans-rip-apart-2025-graphics-at-pre-season-testing-such-a-shame-01jn19jk806c)).
- Columna vertical a la izquierda, fondo azul marino muy oscuro (≈ `#0B0E1E`–`#15182E`) con degradado vertical
  suave, sin separadores marcados entre filas.
- Cabecera: logo F1 blanco centrado; debajo la sesión y el dato: `P1 33:56`, `Q2 08:12`, `LAP 23/57`.
- Fila: número de posición blanco en negrita (P1 en caja roja `#E10600`), logo de equipo blanco pequeño,
  apellido en mayúsculas en Formula1 Display Bold, dato a la derecha (intervalo / gap / tiempo).
- Indicador de boxes: cuadrado blanco con "P" oscuro pegado por fuera a la derecha de la columna.
- 2026: gaps con **un solo decimal truncado** (`+0.9`, `+12.3`); "Interval"/"Leader" en la fila del líder.
- Estados: tarjeta amarilla `SAFETY CAR` / `VIRTUAL SAFETY CAR` con texto negro, `VSC ENDING` /
  `SAFETY CAR IN THIS LAP`, tarjeta roja `RED FLAG`, `YELLOW FLAG` + sector, bandera a cuadros al final,
  verde `TRACK CLEAR` breve al terminar la neutralización. Vuelta rápida en morado `#A020F0`.
- Qualy: zona de eliminación con las posiciones en rojo y línea roja por encima; eliminados atenuados.
- Tipografía: Formula1 Display (propietaria). El HUD de F1 25 ya usa la familia Formula1 en sus estilos
  `OSD_*`, así que dentro del juego la torre la usa sin distribuir la fuente. No está instalada en este PC;
  el visor de diseño usa Titillium Web (OFL) o la fuente que se ponga en `assets/private/fonts`.

## 5. Segunda ronda (0.2.0): torre 2026 real y estados de bandera

- **Captura real de la torre 2026** (comparativa 2025/2026 de la señal internacional, Australia 2026 P1): cabecera
  con logo F1 y `P1 29:40`; filas `posición · logo de equipo en color · código (LEC) · tiempo/diferencia · letra del
  neumático en su color (M amarilla, H blanca) · P blanca fuera de la columna`. Esquinas rectas, fondo azul muy
  oscuro. El cambio 2025→2026 es mínimo: dos filas más y gaps con un decimal
  ([GPFans](https://www.gpfans.com/en/f1-news/1078428/f1-2026-tv-broadcast-new-timing-tower-graphic/)).
- **Estados de bandera**: en la torre FOM la tarjeta de color (SAFETY CAR, VIRTUAL SAFETY CAR, … ENDING, RED FLAG,
  YELLOW FLAG + sectores, GREEN FLAG/TRACK CLEAR) va pegada a la cabecera y la torre entera queda enmarcada en ese
  color. No hay en la web una descripción oficial; se ha deducido de capturas de emisiones y de un rediseño público
  que replica los estados. Project Halo (que reproduce la señal 2026) también los pone en la cabecera.
- **Logos del juego**: Swofty documenta que `Team_Build_SessionCars` construye
  `#overrides.osd.timing_tower.team_badges_broadcast.<clave>` y el HUD `…team_badges_hud.<clave>` con la misma clave
  (`timing_tower_texture_path`). Claves 2026 vistas en logs del juego: `mclaren_26`, `mercedes_26`, `red_bull_26`,
  `ferrari_26`, `williams_26`, `alphatauri_26`, `aston_martin_26`, `haas_26`, `alfa_romeo_26`, `alpine_26`,
  `gm_cadillac_26`.
- **Reloj**: el formateador clonado (`clock_fmt`) se dispara con el flanco 0→1 de su hoja `start`.
- **Visibilidad**: el log de la prueba no mostró escrituras perdidas (las hojas de Swofty no descartan nada), así
  que los dos apellidos en una fila se atribuyen al motor de UI (cambio bajo un nodo oculto, D27).

## 6. Medidas de la torre 2026 (0.4.0)

Fuente: cinco capturas de la emisión 2026 aportadas por Pablo (vueltas 8/71, 26/71 con VSC, 4/55, 10/55 con SC y
sprint 2/24 con GAINED / LOST). Método: ampliación 4x con rejilla cada 10 px, perfiles de luminancia por columna y
detección de color de la caja de P1 y de los bloques amarillos. Los valores están en `REF` (`tools/hudgen.py`) y su
justificación en DECISIONES.md D33.

Hallazgos que cambian el diseño:
- Las filas se comprimen cuando hay un bloque de bandera (21,5 px → 19,2 px).
- En carrera, un coche en boxes muestra `IN PIT` en rojo.
- En el sprint, la cabecera lleva el logotipo SPRINT en lugar del lockup de la FIA, y `GAINED / LOST` va dentro de
  la banda oscura de `LAP`.
- Colores de los iconos de equipo en la emisión: Mercedes, Aston Martin, Williams, Racing Bulls y Cadillac en
  blanco; Audi en rojo; el resto, en su color.

Recursos descargados (solo PRIVATE, en `assets/private/logos`, fuera de git):
- Iconos `2026<equipo>logo.webp` y `…logowhite.webp` de formula1.com.
- `FIA logo (black).png` y `FIA logo (white).png` de Wikimedia Commons.

## 7. Grafismos de la emisión que faltaban (0.6.0)

La torre de la emisión cambia la columna de datos según lo que quiera contar la realización: intervalos, gap al
líder, posiciones ganadas/perdidas, paradas en boxes, vuelta rápida y estado de boxes (`IN PIT` / `OUT`).
Fuentes: [Matt Birkett, F1 TV graphics](http://mattbirkett.co.uk/wp/project/f1-tv-graphics/),
[The Field F1, cómo leer la torre](https://www.thefieldf1.com/charts/how-to-read-timing-screen) y el vídeo de la
emisión 2026 aportado por Pablo (adelantamientos e `IN PIT`).

| Elemento | Estado |
|---|---|
| Intervalo / Leader | Hecho (D7) |
| GAINED / LOST | Hecho (D31) |
| PIT STOPS | **Nuevo en 0.6** (contador de paradas del juego) |
| IN PIT (menta) | Hecho (D40) |
| FORMATION LAP | **Nuevo en 0.6** |
| Diferencias de clasificación por zona | **Nuevo en 0.6** (D45) |
| FINAL LAP | No: el DLL no conoce las vueltas totales |
| Deslizamiento de filas | No: el `motion` del compositor se reproduce al revés al reiniciarse (D41); se usa un fundido |
| Rendimiento del neumático, estrategia, batallas, sectores | Fuera de la torre o sin datos en `live` |
