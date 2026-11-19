# TV Timing Tower

*Español · [English below](#english)*

Addon para **Swofty Launcher** (F1 25, PC) que sustituye la torre de posiciones del juego por la torre de tiempos de
la retransmisión de F1 2026. **La dibuja el propio HUD del juego**: no es una app externa ni un overlay (Electron,
OBS o SimHub), y no usa telemetría UDP.

## Qué hace

- **Cabecera** como la de la TV: logo F1, `FIA Formula 1 World Championship` (o `SPRINT`), y debajo `LAP 23 / 57`,
  `FORMATION LAP`, `Q2 08:12` o `P1 33:56`.
- **Una fila por coche (hasta 24, Mi Equipo incluido):**
  - posición, con P1 en caja roja;
  - logo del equipo: el escudo del propio juego (sigue a los mods de logos), al tamaño de los iconos de la tele;
    Mi Equipo con su propio escudo;
  - abreviatura (`RUS`) cuando hay datos al lado y apellido completo cuando no;
  - el dato de la derecha y la letra del neumático en su color.
- **Libres:** mejor vuelta de P1 y diferencias con P1, y la `P` blanca en boxes o en el garaje.
- **Clasificación y clasificación sprint:**
  - tiempo restante de la sesión, mejor vuelta de P1 y diferencias; en la zona de eliminación, la diferencia con el
    último que pasa, en rojo y con la línea del corte;
  - `Out Lap` para un coche en su vuelta de salida y `-` para uno sin tiempo;
  - los eliminados de la ronda anterior salen de la torre;
  - **tarjeta de tu vuelta como la de la TV de 2026**, al lado de la torre: el tiempo corriendo (gris hasta el
    primer parcial), en cada parcial la diferencia en grande (morado, verde o amarillo) con `SECTOR n`, el tiempo
    del rival (P1, o el corte si estás en la zona de eliminación) hasta el final del sector, y las barras S1 S2 S3.
- **Carrera y sprint:**
  - intervalos con un decimal, como en 2026 (o milésimas, ajuste); la diferencia con el líder en las vueltas 5, 15,
    25…;
  - en parrilla `STARTING TYRES`; en modo carrera, en la vuelta de formación, el campeonato de pilotos y después el
    de constructores; `GAINED / LOST` al empezar la vuelta 2, `TYRES` en las vueltas 7, 17…, `PIT STOPS` en las 10,
    20… y, al acabar, las diferencias en meta con la bandera a cuadros en cada coche;
  - `IN PIT` en verde menta, `+1 Lap`, `Out`/`DSQ`/`NC`; sanciones con el cuadrado rojo «!»; triángulos de posición;
  - en los adelantamientos las filas se deslizan a su nuevo puesto;
  - franjas arriba en el centro: `NEW RACE LEADER` y `FASTEST LAP` (piloto y tiempo), con el reloj morado en la
    fila del autor de la vuelta rápida.
- **Banderas como en la señal de FOM:** bloque de color con el logo FIA para `SAFETY CAR`, `VIRTUAL SAFETY CAR`,
  `SC ENDING`, `VSC ENDING`, `YELLOW FLAG` + sector, `RED FLAG` y `TRACK CLEAR`. Con Safety Car o bandera roja la
  torre muestra solo los nombres.
- **Todo sale del juego:** pilotos, nombres, equipos, colores, escudos, orden, estados, tiempos de vuelta y puntos.
  Funciona con carreras rápidas, fin de semana, modo carrera, Mi Equipo con 24 coches y mods de equipos o logos.
- **Si algo falla, vuelve el tablero original del juego** y el motivo queda en `swofty/loader.log`, con el prefijo
  `[tv-tower]`.

## Requisitos

- F1 25 (Steam; EA app con un Swofty que la soporte) con el 2026 Season Pack.
- Swofty Launcher 1.9.x (1.9.83 o posterior para el escudo de Mi Equipo). **Tras una actualización del juego** la torre solo vuelve cuando Swofty da por buena la
  versión nueva (hasta entonces el log dice `[live] off: build … has no proven live layouts` y solo funciona la demo).
- Solo offline. El addon no hace conexiones de red ni toca nada del online ni del antitrampas.
- El elemento **Posiciones** del HUD tiene que estar activado, porque la torre vive dentro de él.

## Instalación

Swofty Launcher → **Addons** → si tienes una versión anterior, **elimínala primero** → arrastra el `.zip` (o el
`.swoftyaddon`) → actívalo en tu perfil → **Jugar**.

## Ajustes (ADDON SETTINGS)

En el juego cada ajuste aparece en inglés y español (`Rows / Filas`).

| Ajuste | Por defecto | Qué hace |
|---|---|---|
| TV tower | Sí | Activa la torre (desactivada: vuelve la del juego) |
| Rows | All | Todas / Top 10 + yo / Compacta |
| Names | Auto | Auto (abreviatura con datos, apellido sin datos) / Apellido / Abreviatura |
| Race gaps | Auto | Auto (como en TV) / Intervalo / Al líder / Ganadas-perdidas / Paradas / Neumáticos |
| Race gap decimals | 2026 | 2026 (`+0.9`) / Clásico (`+0.934`) |
| New race leader banner | Sí | Franja NEW RACE LEADER en carrera |
| Fastest lap banner | Sí | Franja FASTEST LAP y el reloj morado en la fila |
| Championship on the formation lap | Sí | Modo carrera: campeonatos de pilotos y constructores |
| Qualifying lap card | Sí | La tarjeta de tu vuelta en clasificación |
| Tyres · Tyre age | Sí · No | Compuesto; vueltas con el juego actual |
| Background | Broadcast | Opacidad del fondo |
| Position arrows | Sí | Triángulo verde o rojo 3 s al ganar o perder puestos |
| Team logos | Game | Game (escudos del juego en color) / Game white (los blancos del HUD) / TV (iconos de la emisión) |
| Animations | Sí | Las filas se deslizan en los adelantamientos |
| Highlight my car | Sí | Tu fila un poco más clara |
| In practice / qualifying / sprint qualifying / races | Sí | Por tipo de sesión |
| Show/hide key | F8 | Muestra u oculta (cada sesión nueva la vuelve a mostrar) · **Ctrl+F8** demo · **Mayús+F8** siguiente escenario |
| Demo mode · Demo scenario | No · Cycle | Datos simulados |
| Verbose log | No | Coste por actualización y cambios de estado en el log |

La escala, la posición y la opacidad se cambian en el editor de HUD del juego (Ajustes → HUD → Personalizar).

## Rendimiento

La torre se actualiza 30 veces por segundo y solo escribe los valores que cambian: unos 30 µs por actualización.

## Problemas conocidos

- `SC ENDING` y `VSC ENDING` se deducen: el juego no avisa a los addons de «safety car en esta vuelta».
- Contrarreloj, repeticiones y modo espectador usan el tablero del juego.
- Los tiempos de vuelta, el campeonato y el escudo de Mi Equipo se leen de la memoria del juego (Swofty no los
  ofrece), siempre con lecturas protegidas: si algo no cuadra, simplemente no salen y el log dice por qué.
- El campeonato de constructores solo sale si todos los pilotos con puntos están en la carrera.
- Con 24 coches y un bloque de bandera la torre puede no caber: escálala un poco en el editor de HUD.

## Desinstalación

Swofty → **Addons** → TV Timing Tower → desactivar o **Eliminar**. No queda nada en la carpeta del juego.

## Compilar

```
python tools/fetch_assets.py     # iconos de equipo, logos F1 y FIA en assets/private (fuera de git)
python tools/build.py            # DLL, registro HUD, tests, compose y build de perfil de prueba, paquete en dist/
```
Hace falta Python 3.10+ con Pillow y Zig 0.16.

Créditos: SDK de Swofty (MIT). Las filas por coche, la regla de eliminación y el método del reloj vienen de Project
Halo de nyannoying (<https://github.com/nyannoying1337/f1-25-project-halo>). Los logos de F1, de la FIA y de los
equipos son de sus dueños y aparecen igual en el juego oficial.

---

## English

A **Swofty Launcher** addon for F1 25 (PC) that replaces the game's standings board with the 2026 F1 broadcast
timing tower, **drawn by the game's own HUD**: no external app, no overlay, no UDP telemetry.

- **Header:** F1 logo, `FIA Formula 1 World Championship` (or `SPRINT`), and `LAP 23 / 57`, `FORMATION LAP`,
  `Q2 08:12` or `P1 33:56`.
- **Rows (up to 24 cars, My Team included):** position, the game's own team badge (follows logo mods, at the TV
  icons' size; My Team's own badge too), the code beside data or the full surname without, the data column and the
  tyre letter.
- **Practice:** P1's best lap and gaps to P1, and the pit box.
- **Qualifying:** the session clock, P1's best lap and gaps (the gap to the last car through in the knockout zone),
  `Out Lap` and `-` without a time; cars knocked out earlier leave the tower. **The 2026 TV lap card** beside the
  tower: your lap running, the gap at each split in big figures with `SECTOR n`, the target's time (P1 or the cut)
  to the end of the sector, and S1 S2 S3 bars.
- **Races:** STARTING TYRES on the grid and, in a career, the drivers' then the constructors' championship on the
  formation lap; one-decimal intervals (or thousandths) and the gap to the leader on laps 5, 15, 25…; GAINED / LOST
  as lap 2 starts; TYRES on laps 7, 17…; PIT STOPS on laps 10, 20…; the gaps at the line and a chequered flag per
  finisher; mint `IN PIT`; penalties as the red "!" square; position triangles; rows slide on overtakes; NEW RACE
  LEADER and FASTEST LAP banners top centre, and the purple clock on the fastest lap's row.
- **Flags:** FOM-style blocks with the FIA logo. Names only under the safety car or a red flag.

Everything comes from the game (drivers, names, teams, colours, badges, order, lap times, points), so careers,
My Team and team or logo mods work. If the session cannot be read, the game's own board comes back and
`[tv-tower]` explains why in `swofty/loader.log`.

**Install:** in Swofty's **Addons** page remove any older version first, then drag the `.zip` (or
`.swoftyaddon`) onto the page, enable it, play.

**Requirements:** F1 25 (Steam; EA app with a Swofty that supports it) with the 2026 Season Pack, Swofty Launcher
1.9.x (1.9.83 or later for My Team's badge), the game's Position HUD element switched on. After a game update the tower comes back only once Swofty
supports the new build (until then the log says `[live] off: build … has no proven live layouts` and only the demo
works). Offline only: no network connections, nothing online or anti-cheat related.

**Settings:** see the table above. Size, position and opacity are set in the game's HUD editor.

**Performance:** 30 updates a second, about 30 µs each, and only changed values are written.

**Known issues:** SC ENDING and VSC ENDING are inferred; Time Trial, replays and the spectator view keep the game's
board; lap times, the championship and My Team's badge are read from the game's memory with guarded reads (if
anything looks wrong they simply don't show, and the log says why); the constructors' table needs every driver
with points in the race; with 24 cars and a flag block, scale the tower down a little in the HUD editor.

**Build:** `python tools/fetch_assets.py && python tools/build.py`. MIT licence. Credits: Swofty SDK (MIT);
Project Halo by nyannoying https://github.com/nyannoying1337/f1-25-project-halo.
