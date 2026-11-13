# TV Timing Tower

*Español · [English below](#english)*

Addon para **Swofty Launcher** (F1 25, PC, Steam) que sustituye la torre de posiciones del juego por la torre de
tiempos de la retransmisión de F1 2026. **La dibuja el propio HUD del juego**: no es una app externa ni un overlay
(Electron, OBS o SimHub), y no usa telemetría UDP.

## Qué hace

- **Cabecera** como la de la TV: logo F1, `FIA Formula 1 World Championship` (o `SPRINT`), y debajo `LAP 23 / 57`,
  `FORMATION LAP`, `Q2 08:12` o `P1 33:56`.
- **Una fila por coche (hasta 24, Mi Equipo incluido):**
  - posición, con P1 en caja roja;
  - logo del equipo: el icono de la tele o el escudo del propio juego (ajuste); un equipo sin escudo, como Mi
    Equipo, lleva una ficha con su color;
  - abreviatura (`RUS`) cuando hay datos al lado y apellido completo cuando no;
  - el dato de la derecha y la letra del neumático en su color.
- **Libres:** mejor vuelta de P1 y diferencia de los demás con P1; `P` blanca en boxes o en el garaje.
- **Clasificación y clasificación sprint:**
  - tiempo restante de la sesión;
  - P1 con su vuelta y los demás con la diferencia con P1;
  - los coches en la zona de eliminación, con la diferencia con el último que pasa;
  - los eliminados de la ronda anterior salen de la torre;
  - destellos en verde (mejor personal) o morado (mejor de la sesión).
- **Carrera y sprint:**
  - intervalos con un decimal, como en 2026; la diferencia con el líder en las vueltas 5, 15, 25…;
  - `STARTING TYRES` en parrilla, `GAINED / LOST` al empezar la vuelta 2, `TYRES` (vueltas de cada juego) en las
    vueltas 7, 17…, `PIT STOPS` en las 10, 20… y, al acabar, las diferencias en meta con la bandera a cuadros en cada coche;
  - `IN PIT` en verde menta, `+1 Lap`, `Out`/`DSQ`/`NC`;
  - sanciones con el cuadrado rojo «!», vuelta rápida en morado, triángulos de posición y bandera a cuadros;
  - en los adelantamientos, las filas se desvanecen y vuelven con el orden nuevo.
- **Banderas como en la señal de FOM:** bloque de color con el logo FIA para `SAFETY CAR`, `VIRTUAL SAFETY CAR`,
  `SC ENDING`, `VSC ENDING`, `YELLOW FLAG` + sector, `RED FLAG` y `TRACK CLEAR`. Con Safety Car o
  bandera roja la torre muestra solo los nombres.
- **Todo sale del juego:** pilotos, nombres, equipos, colores, escudos, orden, estados y vueltas totales. Funciona
  con carreras rápidas, fin de semana, Mi Equipo con 24 coches y mods de equipos o logos.
- **Si algo falla, vuelve el tablero original del juego** y el motivo queda en `swofty/loader.log`, con el prefijo
  `[tv-tower]`.

## Requisitos

- F1 25 (Steam o EA app) con el 2026 Season Pack. En la versión EA hace falta tener Swofty actualizado;
  probado solo en Steam.
- Swofty Launcher 1.9.x (probado hasta 1.9.21).
- Solo offline. El addon no hace conexiones de red ni toca nada del online ni del antitrampas.
- El elemento **Posiciones** del HUD tiene que estar activado, porque la torre vive dentro de él.

## Instalación

Swofty Launcher → **Addons** → arrastra `tv-timing-tower.swoftyaddon` → actívalo en tu perfil → **Jugar**.

## Ajustes (ADDON SETTINGS)

En el juego cada ajuste aparece en inglés y español (`Rows / Filas`); `python tools/build.py --lang es` genera el
paquete solo en español.

| Ajuste | Por defecto | Qué hace |
|---|---|---|
| TV tower | Sí | Activa la torre (desactivada: vuelve la del juego) |
| Rows | All | Todas / Top 10 + yo / Compacta |
| Names | Auto | Auto (abreviatura con datos, apellido sin datos) / Apellido / Abreviatura |
| Race gaps | Auto | Auto (como en TV) / Intervalo / Al líder / Ganadas-perdidas / Paradas / Neumáticos |
| Race gap decimals | 2026 | 2026 (`+0.9`) / Clásico (`+0.934`, como en los F1 anteriores) |
| Tyres · Tyre age | Sí · No | Compuesto; vueltas con el juego actual |
| Background | Broadcast | Opacidad del fondo |
| Position arrows | Sí | Triángulo verde o rojo 3 s al ganar o perder puestos (carrera) |
| Team logos | TV | TV (iconos de la emisión) / Game (escudos del juego, en color; siguen a los mods) / Game white |
| Animations | Sí | Fundido de las filas en los adelantamientos |
| Team colour bar · Highlight my car | No · Sí | |
| In practice / qualifying / sprint qualifying / races | Sí | Por tipo de sesión |
| Show/hide key | F8 | F8 muestra u oculta (solo en sesión; cada sesión nueva la vuelve a mostrar) · **Ctrl+F8** demo · **Mayús+F8** siguiente escenario |
| Demo mode · Demo scenario | No · Cycle | Datos simulados |
| Verbose log | No | Coste por actualización y cambios de estado en el log |

La escala, la posición y la opacidad se cambian en el editor de HUD del juego (Ajustes → HUD → Personalizar).

## Rendimiento

La torre se actualiza 30 veces por segundo (la tecla F8 se lee en cada fotograma) y solo se escriben los valores
que cambian. Cada actualización cuesta unos 23 µs en un PC de pruebas; en el juego, menos de 1 ms por segundo.

## Problemas conocidos

- Los tiempos de libres y clasificación los mide el addon, porque el estado en vivo de Swofty no los da. Error de
  unas centésimas.
- `SC ENDING` y `VSC ENDING` se deducen: el juego no avisa a los addons de «safety car en esta vuelta», así que
  `SC ENDING` sale cuando el coche de seguridad entra al pit lane y `VSC ENDING` justo al acabar el VSC.
- Contrarreloj, repeticiones y modo espectador usan el tablero del juego.
- La TV junta las filas cuando hay un bloque de bandera; aquí la torre crece. Con 24 coches (Mi Equipo) la torre
  sube algo más por su cuenta, pero si aun así se corta por arriba o por abajo, escálala o muévela en el editor de
  HUD.
- No hay deslizamiento de filas como en la TV (el compositor de Swofty no lo permite sin saltos); se usa un fundido.

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

A **Swofty Launcher** addon for F1 25 (PC, Steam) that replaces the game's standings board with the 2026 F1
broadcast timing tower, **drawn by the game's own HUD**: no external app, no overlay, no UDP telemetry.

- **Header:** F1 logo, `FIA Formula 1 World Championship` (or `SPRINT`), and `LAP 23 / 57`, `FORMATION LAP`,
  `Q2 08:12` or `P1 33:56`.
- **Rows (up to 24 cars, My Team included):** position, team icon or the game's badge (a team colour chip when
  the game has none), the code beside data or the full surname without, the data column and the tyre letter.
- **Practice:** gaps to P1.
- **Qualifying:** the session clock and gaps to P1; the knockout zone shows the gap to the last car through, and
  cars knocked out earlier leave the tower.
- **Races:** STARTING TYRES on the grid; one-decimal intervals (or classic thousandths, a setting) and the gap to the leader on laps 5, 15, 25…;
  GAINED / LOST as lap 2 starts; TYRES (laps on the set) on laps 7, 17…; PIT STOPS on laps 10, 20…; the gaps at the
  line and a chequered flag per finisher at the end; penalties as the red "!" square; mint `IN PIT`; penalties, fastest lap, position triangles, chequered flag and row
  fades on overtakes.
- **Flags:** FOM-style blocks with the FIA logo. Names only under the safety car or a red flag.

Everything comes from the game (drivers, names, teams, colours, badges, order), so careers, My Team and team or
logo mods work. If the session cannot be read, the game's own board comes back and `[tv-tower]` explains why in
`swofty/loader.log`.

**Install:** drag `tv-timing-tower.swoftyaddon` onto Swofty's **Addons** page, enable it, play.

**Requirements:** F1 25 (Steam or EA app; EA needs a recent Swofty, tested on Steam only) with the 2026 Season Pack, Swofty Launcher 1.9.x (tested up to
1.9.21), the game's Position HUD element switched on. Offline only: no network connections, nothing online or
anti-cheat related.

**Settings:** see the table above. Size, position and opacity are set in the game's HUD editor.

**Performance:** 30 updates a second, about 23 µs each, and only changed values are written.

**Known issues:** practice and qualifying lap times are measured by the addon; SC ENDING (from the safety car entering the pit lane)
and VSC ENDING are inferred; Time Trial, replays and the spectator view keep the game's board; with a flag block the tower
grows instead of compressing its rows; with 24 cars (My Team) it slides a bit further up on its own, by an amount
not yet confirmed in game; rows fade instead of sliding.

**Build:** `python tools/fetch_assets.py && python tools/build.py`. MIT licence. Credits: Swofty SDK (MIT);
Project Halo by nyannoying.
