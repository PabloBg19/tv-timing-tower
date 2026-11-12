# Texto para la página de OverTake.gg

Copia cada bloque en su campo del formulario "Post resource" (sección F1 25). El editor de OverTake admite negrita,
listas y títulos con su barra de herramientas; aquí van en Markdown para que se vea la estructura.

---

## Title

TV Timing Tower – 2026 broadcast timing tower (Swofty)

## Tag line (short description)

The 2026 F1 TV timing tower, drawn by the game's own HUD: intervals, knockout zone, SC/VSC/red flag blocks, team icons and more.

## Version

0.7.2

## Category / prefix

F1 25 → Mods (or Apps/Utilities, whichever fits) · Requires Swofty Launcher

---

## Description

**TV Timing Tower** replaces F1 25's standings board with the timing tower from the 2026 F1 world feed. It isn't an
overlay or an external app: the tower is **drawn by the game's own HUD**, so it uses the game's fonts, sits where the
standings board sits, and can be moved and scaled in the game's HUD editor.

*(Add 3 or 4 screenshots here: race with intervals, qualifying with the knockout zone, a Safety Car block, and the
starting tyres on the grid. A short video helps a lot.)*

### What it shows

**Header**
- F1 logo with *FIA Formula 1 World Championship* (or *SPRINT*)
- `LAP 23 / 57`, `FORMATION LAP`, `Q2 08:12` or `P1 33:56` with the session clock

**Every car (up to 24, My Team included)**
- Position, with P1 in the red box
- The 2026 broadcast team icons (or the game's own badges, so logo mods show too)
- The three-letter code beside the data, the full surname when there is none
- Data column and the tyre compound letter in its colour

**Practice**
- Best lap for P1, gaps to P1 for the rest
- White `P` box for cars in the pits

**Qualifying and sprint qualifying**
- Gaps to P1; the knockout zone shows the gap to the last car through
- Cars knocked out earlier leave the tower
- Green and purple lap flashes

**Races, cycling like the TV**
- **STARTING TYRES** on the grid
- One-decimal intervals
- **GAINED / LOST** as lap 2 starts
- Gap to the leader on laps 5, 15, 25…
- **TYRES** (laps on the set) on laps 7, 17…
- **PIT STOPS** on laps 10, 20…
- At the flag: the gaps at the line and a chequered flag on each finisher

**Race states**
- `IN PIT` in mint
- Penalties as the red **!** square
- Fastest lap in purple
- Green and red triangles when places change
- `+1 Lap`, `Out`, `DSQ`
- Rows fade when cars swap places

**Flags, as on the FOM world feed**
- Yellow, VSC, Safety Car, *SAFETY CAR IN THIS LAP*, *VSC ENDING*, red flag and *TRACK CLEAR* blocks with the FIA logo
- Under the Safety Car or a red flag the tower shows the names only

Everything comes from the game: drivers, names, teams, colours, badges, order and status. It works in Grand Prix,
career, My Team with 24 cars, and with team or logo mods.

### Requirements

- F1 25 on Steam (build 25251167) with the 2026 Season Pack
- [Swofty Launcher](https://swofty.net) 1.9.x (tested up to 1.9.21)
- The game's **Position** HUD element switched on: the tower lives inside it

### Installation

1. Open Swofty Launcher → **Addons**.
2. Drag `tv-timing-tower.swoftyaddon` onto the page.
3. Enable it in your profile and press **Play**.

### Settings (ADDON SETTINGS, in game or in the launcher)

- Rows: all / top 10 + me / compact
- Names: auto / surname / code
- Race gaps: auto (as on TV) / interval / leader / gained-lost / pit stops / tyres
- Team logos: TV icons / game badges / game white badges
- Tyres, tyre age, background opacity, position arrows, animations, team colour bar, highlight my car
- On or off per session type
- Show/hide key (F8). **Ctrl+F8** runs a demo with every state; **Shift+F8** goes to the next scenario.

Size, position and opacity: the game's HUD editor (Settings → HUD → Customise).

### Offline only

The addon makes **no network connections**, uses **no UDP telemetry** and touches **nothing online or anti-cheat
related**. It reads the session from Swofty's live state, inside the game.

### Performance

The tower updates 30 times a second and writes only the values that change: about 20 µs per update.

### Known limits

- Practice and qualifying lap times are measured by the addon, because Swofty's live state doesn't provide them
  yet. Expect a few hundredths of difference.
- *SAFETY CAR IN THIS LAP* and *VSC ENDING* are inferred.
- Time Trial, replays and the spectator camera keep the game's own board.
- The TV slides rows on overtakes; here they fade, which is what Swofty's HUD composer allows.
- With 24 cars and a flag block, scale the tower down a little in the HUD editor if it touches the bottom of the
  screen.

### Credits

- Swofty Launcher and its SDK (MIT)
- Per-car rows, the knockout rule and the session clock method from **Project Halo** by nyannoying
- F1, FIA and team logos belong to their owners

### Feedback

Found a bug? Turn on **Verbose log** in the settings, play the session, and post the `[tv-tower]` lines from
`swofty/loader.log` with a screenshot.

---

## Descripción en español (opcional, debajo de la inglesa)

**TV Timing Tower** sustituye la torre de posiciones de F1 25 por la torre de la retransmisión de 2026. No es un
overlay ni una app externa: la dibuja **el propio HUD del juego**, con sus fuentes, en el sitio de la torre original,
y se puede mover y escalar en el editor de HUD.

- **Libres:** diferencias con P1.
- **Clasificación:**
  - diferencias con P1;
  - la zona de eliminación, con la diferencia con el último que pasa;
  - los eliminados salen de la torre.
- **Carrera:**
  - neumáticos de salida en parrilla e intervalos con un decimal;
  - ganadas/perdidas en la vuelta 2 y gap al líder a ratos;
  - neumáticos (vueltas del juego) y paradas;
  - `IN PIT`, sanciones con la «!» roja, vuelta rápida en morado y bandera a cuadros al final.
- **Banderas:** amarilla, VSC, Safety Car y roja con el logo FIA, como en la señal de FOM.
- **Todo sale del juego:** funciona en carrera, Mi Equipo con 24 coches y con mods de equipos o logos.

**Requisitos:** F1 25 en Steam con el 2026 Season Pack, Swofty Launcher 1.9.x y el elemento *Posiciones* del HUD
activado.

**Instalación:** Swofty → Addons → arrastra el archivo → actívalo → Jugar.

Solo offline: sin conexiones de red, sin UDP y sin tocar nada del online ni del antitrampas.

---

## Changelog for this first upload

**0.7.2 – first public release**
- 2026 broadcast timing tower drawn by the game's HUD
- Practice, qualifying (knockout zone gaps), sprint and race views
- STARTING TYRES, GAINED / LOST, TYRES, PIT STOPS and the race end as on TV
- FOM-style flag blocks with the FIA logo
- 24 cars and My Team
- Settings in English and Spanish
