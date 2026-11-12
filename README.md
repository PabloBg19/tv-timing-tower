# TV Timing Tower

*Español abajo · [English below](#english)*

Addon para **Swofty Launcher** (F1 25, PC, Steam) que sustituye la torre de posiciones del juego por una torre de
tiempos con el estilo de la retransmisión de F1 TV / DAZN 2025-2026. **Se dibuja con el propio HUD del juego** (no es
una app externa, ni overlay de Electron/OBS/SimHub, ni usa telemetría UDP).

## Qué hace

- **Cabecera** con el logo de F1 y la sesión: `P1 33:56`, `Q2 08:12`, `SQ1 05:40` o `LAP 23/57`.
- **Una fila por piloto**: posición (P1 en caja roja), logo de equipo **en color del propio juego** (si tienes un
  mod de logos, sale el del mod), abreviatura (`RUS`) junto a los datos o apellido completo cuando no hay datos
  (como en TV), dato a la derecha (`Interval`, `+6.7`, `Out`) y la letra del neumático en su color.
- **Libres**: orden del juego, mejor vuelta del líder y diferencia del resto, `P` blanca cuando el coche está en
  boxes o en el garaje.
- **Clasificación y clasificación sprint** (Q1/Q2/Q3, SQ1/SQ2/SQ3): cronómetro de la sesión, zona de eliminación
  con las posiciones en rojo y línea roja, los eliminados de la ronda anterior desaparecen de la torre (Q2: 16
  filas, Q3: 10), tiempos en verde (mejor personal) o morado (mejor de la sesión) durante 3 s.
- **Carrera y sprint**: `LAP X / Y`. En la vuelta 1 y con Safety Car, solo nombres. Después, intervalo al de
  delante con un decimal (formato 2026) y, a ratos, gap al líder. Al empezar la vuelta 2, `GAINED / LOST`
  (puestos ganados o perdidos desde la parrilla). Además `+1 Lap`, `PIT`, `Out` / `DSQ` / `NC`, penalizaciones
  (`+5s`, `DT`, `SG`), vuelta rápida en morado, flechas de posición y bandera a cuadros al final.
- **Banderas como en la señal de FOM 2026**: un bloque de color con el escudo FIA bajo la cabecera para
  `SAFETY CAR`, `VIRTUAL SAFETY CAR`, `SAFETY CAR IN THIS LAP`, `VSC ENDING`, `YELLOW FLAG` + sector, `RED FLAG` y
  `TRACK CLEAR`; al final, bandera a cuadros.
- **Modo demo** con datos simulados de todos esos casos, y un **visor HTML** para revisar el diseño sin el juego.
- Si el addon no puede leer la sesión (otra build del juego, core sin estado en vivo), **vuelve la torre original**
  y lo deja escrito en `swofty/loader.log` con el prefijo `[tv-tower]`.

## Requisitos

- F1 25 en Steam, build 25251167, con el 2026 Season Pack.
- Swofty Launcher 1.9.x (probado contra 1.9.14) con Swofty Core 0.2.x.
- Sesiones offline (Swofty siempre arranca el juego offline). El addon no hace llamadas de red ni toca nada del
  online ni del antitrampas.
- En el editor de HUD del juego, el elemento **Track Position / Posiciones** tiene que estar activado: la torre vive
  dentro de él.

## Instalación

1. Abre Swofty Launcher → **Addons**.
2. Arrastra `dist/tv-timing-tower-PRIVATE.swoftyaddon` (tu versión personal con logos reales) o
   `dist/tv-timing-tower-PUBLIC.swoftyaddon` a la página. Los dos tienen el mismo id: instalar uno sustituye al otro.
3. Actívalo en tu perfil y pulsa **Jugar**.

## Ajustes (ADDON SETTINGS, en el menú principal bajo Carrera, o en el launcher)

| Ajuste | Por defecto | Qué hace |
|---|---|---|
| Torre de TV | Sí | Activa o desactiva la torre (desactivada: vuelve la del juego) |
| Filas | Todas | Todas / Top 10 + yo / Compacta (P1-P3 y los coches a tu alrededor) |
| Nombres | Automático | Automático (abreviatura con datos, apellido sin datos, como en TV) / Apellido / Abreviatura |
| Diferencias en carrera | Automático | Automático (como en TV) / Intervalo / Al líder / Ganadas-perdidas |
| Neumáticos | Sí | Compuesto actual |
| Vueltas del neumático | No | Vueltas con el juego actual (contadas desde la última parada vista) |
| Fondo | TV | Sólido / TV / Translúcido (opacidad del fondo) |
| Flechas de posición | Sí | Flecha verde/roja 3 s al ganar o perder posiciones (solo carrera) |
| Logos de equipo | TV (PRIVATE) / Juego (PUBLIC) | TV: los iconos de la emisión 2026 (solo PRIVATE). Juego: los escudos en color del propio juego (siguen a los mods de logos). Juego blanco: los del HUD |
| Barra de color de equipo | No | Fina barra con el color del equipo |
| Resaltar mi coche | Sí | Tu fila más clara |
| En libres / clasificación / clasificación sprint / carrera | Sí | Activar por tipo de sesión |
| Tecla mostrar/ocultar | F8 | F8 oculta/muestra · **Ctrl+F8** modo demo · **Mayús+F8** siguiente escenario |
| Modo demo / Escenario demo | No / Todos | Datos simulados para validar el diseño |
| Logo de la cadena | Sí | (Solo PRIVATE) logo arriba a la derecha mientras se ve la torre |
| Log detallado | No | Coste por fotograma y cambios de estado en `swofty/loader.log` |

**Escala, posición y opacidad general** se ajustan como cualquier elemento del HUD: Ajustes → HUD →
Personalizar (o desde la pausa en sesión), seleccionando el elemento de posiciones. Por defecto está arriba a la
izquierda, en el hueco del contador de vueltas (que se oculta), y con 22 coches cabe entera.

## Modo demo

Entra en una sesión de **libres** (Gran Premio o fin de semana) y pulsa **Ctrl+F8** (o activa *Modo demo* en
ADDON SETTINGS). La torre recorre: carrera, Q1 con eliminación, Q2 con eliminados, Safety Car, VSC → VSC ENDING →
TRACK CLEAR, bandera roja, bandera a cuadros, libres y clasificación sprint (14 s cada uno). **Mayús+F8** pasa al
siguiente. Fuera del juego: `python tools/build.py` y abre `preview/index.html` servido con
`python -m http.server 8765 --directory preview` → <http://127.0.0.1:8765>.

## Problemas conocidos

- Los **tiempos de vuelta** los mide el addon (el estado en vivo de Swofty aún no los da). Error esperado de unas
  centésimas; una vuelta invalidada por el juego sin aviso de límites de pista podría colarse (el orden siempre es
  el del juego).
- `SAFETY CAR IN THIS LAP` y `VSC ENDING` se deducen (el juego no expone el aviso): SC → cuando el coche de
  seguridad entra al pit lane; VSC → al terminar el VSC.
- En **Contrarreloj**, repeticiones y modo espectador se ve la torre del juego.
- Mi Equipo: su logo puede quedar vacío (el juego no da clave de escudo para él).
- Si el propio juego oculta el tablero (garaje, menús) mientras cambia el orden, podría verse un apellido repetido en
  una fila hasta el siguiente cambio de esa fila; si lo ves, apunta cuándo.
- Parrillas de más de 24 coches (mods) no caben.
- Animaciones de deslizamiento de filas: no incluidas en 0.1.0.

## Desinstalación

Swofty → **Addons** → TV Timing Tower → desactivar o **Eliminar**. Al cerrar la sesión de juego Swofty deja la
carpeta del juego como estaba; no queda nada del addon en el juego.

## Compilar desde el código

```
python tools/fetch_assets.py     # solo PRIVATE: iconos de equipo de la TV, logo F1, logos FIA (fuera de git)
python tools/build.py            # DLL, registros HUD, tests, validación con swofty hud compose, paquetes en dist/
```
Necesita Python 3.10+ con Pillow y Zig 0.16 (en `.tools/` o en el PATH). Fuente oficial opcional para el visor:
`assets/private/fonts/Formula1-Bold.ttf` / `Formula1-Regular.ttf`.

Créditos: SDK de Swofty (MIT). El diseño de filas por coche y la regla de eliminación se inspiran en
Project Halo de nyannoying (<https://github.com/nyannoying1337/f1-25-project-halo>). Logos y marcas reales:
propiedad de sus dueños; solo en la build PRIVATE, para uso personal.

---

## English

An addon for **Swofty Launcher** (F1 25, PC, Steam) that replaces the game's standings board with an F1 TV / DAZN
2025-2026 broadcast-style timing tower, **drawn with the game's own HUD** (no external app, no Electron/OBS/SimHub
overlay, no UDP telemetry).

**What it does:** header with the F1 logo and `P1 33:56` / `Q2 08:12` / `LAP 23/57`; one row per driver with
position (red box for P1), the game's own colour team logo (logo mods show too), surname in capitals (or 3-letter
code), data on the right and the tyre compound letter in its colour.
Practice: best lap and gaps, white `P` in the pits. Qualifying and sprint qualifying: session clock, knockout zone,
cars knocked out earlier leave the tower, green/purple lap time flashes. Races: `LAP X/Y`, one-decimal intervals with the gap to the
leader at times (as on TV), `+1 LAP`, `PIT`, `DNF/OUT/DSQ/NC`, penalties, purple fastest lap, arrows, chequered
flag. Codes beside data, full surnames without it (lap 1, safety car); GAINED / LOST as lap 2 starts. Flag
states as on the 2026 FOM world feed: a colour block with the FIA badge under the header.
If the session cannot be read the game's own board comes back and `[tv-tower]` explains why in `swofty/loader.log`.

**Requirements:** F1 25 Steam build 25251167 + 2026 Season Pack, Swofty Launcher 1.9.x, offline sessions, and the
game's Track Position HUD element switched on.

**Install:** drag `dist/tv-timing-tower-PUBLIC.swoftyaddon` (or the PRIVATE one) onto Swofty's **Addons** page,
enable it, play.

**Settings** (ADDON SETTINGS under Career, or in the launcher): tower on/off, rows (all / top 10 + me / compact),
names (surname / code), race gaps (auto / interval / leader), tyres, tyre age, background opacity, position arrows,
team logos in colour, team colour bar, highlight my car, per-session switches, show/hide key (F8; Ctrl+F8 demo; Shift+F8 next scenario),
demo mode and scenario, verbose log. Size, position and overall opacity: the game's HUD editor.

**Demo:** in a practice session press Ctrl+F8. Outside the game: `python tools/build.py`, then serve `preview/`
and open `index.html`.

**Known issues:** lap times are measured by the addon (Swofty's live state has none yet); SC IN THIS LAP and VSC
ENDING are inferred; Time Trial, replays and the spectator view keep the game's board; My Team logo may be empty;
grids over 24 cars do not fit; no row slide animations yet.

**Uninstall:** Swofty → Addons → TV Timing Tower → disable or Remove.

**Build:** `python tools/build.py` (Python 3.10+ with Pillow, Zig 0.16). MIT licence. Credits: Swofty SDK (MIT);
row-per-car design and knockout rule inspired by Project Halo by nyannoying
(<https://github.com/nyannoying1337/f1-25-project-halo>).
