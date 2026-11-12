# Resumen — TV Timing Tower

## 0.2.0 (12-11-2026, tras la primera prueba en el juego)

La 0.1.0 se vio en el juego: la torre, los nombres, las P de boxes, las tarjetas y la demo funcionaban. Cambios a
partir de las capturas y del log:

| Problema visto | Cambio |
|---|---|
| La torre no cabía entera | Filas más bajas (18 u) y torre subida al hueco del contador de vueltas (D22) |
| Logos incorrectos y blancos | Escudos del propio juego en color, `team_badges_broadcast` (siguen a los mods de logos) (D23) |
| Neumático con círculo | Solo la letra en su color, como en 2026 (D24) |
| SC / bandera roja distintos de TV | Tarjeta bajo la cabecera + marco de color en toda la torre, textos FOM (D25) |
| Eliminados en gris en Q2/Q3 | Salen de la torre (D26) |
| Dos apellidos en una fila tras la demo | Los nombres solo cambian con la fila visible (D27) |
| Reloj de libres/qualy vacío | Pulso de la hoja de arranque del reloj (D28) |
| `LAP23 /57` pegado, apellidos largos sobre el tiempo | Espaciado corregido, nombres limitados a su caja |

El cierre del juego de las 15:31 que dejó un informe de crash fue el propio juego terminando al salir
(`TerminateProcess` durante el cierre, sin el addon en la pila).

Verificado aquí: compila, tests en host (incluidos los nuevos: 16 filas en Q2, 10 en Q3, logos en color, marco,
nombres que no cambian bajo una fila oculta, pulso del reloj), `swofty hud compose` de las dos builds contra los
ficheros del juego (~4.700 enlaces) y el visor HTML. **Sin ver en el juego**: los escudos en color
(`team_badges_broadcast`), el reloj con pulso, el marco y la nueva posición.

---

## 0.1.0

## Entregado

| Entregable | Dónde |
|---|---|
| Paquete personal con logos reales | `dist/tv-timing-tower-PRIVATE.swoftyaddon` (y la carpeta `dist/tv-timing-tower-PRIVATE/`) |
| Paquete publicable sin logos ni marcas | `dist/tv-timing-tower-PUBLIC.swoftyaddon` |
| Código fuente (git, commits por hitos) | `native/src` (DLL), `tools/hudgen.py` (registro HUD + sprites), `tools/build.py` |
| Investigación / decisiones | `INVESTIGACION.md`, `DECISIONES.md` |
| README es/en, checklist de pruebas | `README.md`, `PRUEBAS.md` |
| Modo demo en el juego + visor HTML | ajuste/Ctrl+F8; `preview/index.html` |

Arquitectura: registro `hud.element` del compositor de Swofty sobre el tablero de posiciones nativo (el juego
dibuja la torre con su propia UI y fuente) + DLL nativo que lee `swofty::live` (memoria del juego, sin UDP) y
escribe hojas `view_model_data.hud_tvt_*`. Nombres, colores de equipo, logos del juego, reloj de sesión y total de
vueltas salen del propio view model del juego.

## Verificado aquí (sin poder ver el juego)

- `swofty hud check` contra los ficheros de tu juego: 90/90 OK.
- `swofty hud compose` de los dos registros: compone y comprueba ~4.900 enlaces con los tipos del juego (un tipo
  equivocado es lo que puede colgar el juego; el compositor lo habría rechazado).
- Instalación y **build completo de perfil** con la CLI de Swofty 1.9.14 en un directorio de datos temporal:
  manifiesto aceptado, ajustes listados (`swofty settings list`), ficheros colocados en
  `swofty/addons/pablo.tv-timing-tower/…`. Tu perfil real y la carpeta del juego no se tocaron.
- Tests en host (`python tools/build.py test`): modelo de torre en todos los escenarios, formatos, zona de
  eliminación, tarjetas SC/VSC/roja/final, modos de filas, reloj de vueltas (pausas, avisos de límites),
  adaptador `live` sobre el fake oficial de Swofty, hojas sobre el fake del core del HUD y coherencia hojas ↔
  registro (85 enlaces).
- El DLL solo importa `kernel32`, `user32` y la CRT: ninguna librería de red (comprobado en cada build).
- Tus logs muestran que en tu instalación el estado en vivo de Swofty está activo con todos los grupos necesarios.
- Diseño revisado en el visor HTML con los mismos sprites, medidas y modelo C++ (`tools/shots.py`).

## No verificado (necesita jugar; ver `PRUEBAS.md`)

1. **Que la torre se vea en partida** en el sitio y tamaño esperados (el compositor de Swofty avisa de que su
   modo `screen` todavía no se ha visto en juego; Project Halo, que usa el mismo mecanismo y los mismos
   archivos, sí funciona). Posibles ajustes: tamaños de texto (`cap_*` en `tools/hudgen.py`).
2. El **reloj de la cabecera** en libres/qualy (clon del formateador del juego; el build avisa de que ningún nodo
   nativo lo dispara así, igual que en la plantilla oficial).
3. **Precisión de los tiempos de vuelta** calculados por el addon frente a los del juego.
4. La inferencia de **SAFETY CAR IN THIS LAP** (entrada del coche de seguridad al pit lane).
5. Las **claves de escudo** que devuelve el juego para cada equipo 2026 (los logos privados se eligen por ellas;
   si alguna no casa, se ve el logo del propio juego).
6. Coste por fotograma real (objetivo < 0,3 ms; queda en el log con *Log detallado*).
7. Logo de la cadena con frame generation activo (tus logs indican que el overlay dibuja bien con DLSS FG).

## Siguientes pasos

1. Pasar `PRUEBAS.md` y mandarme el `loader.log` (líneas `[tv-tower]`, `[hud]`, `[live]`).
2. Ajuste fino visual con capturas reales (márgenes, tamaños de letra, alto de fila).
3. Animaciones de cambio de posición y de tarjetas con `motion` (desactivadas por precaución, D12).
4. Torre también en repeticiones y modo espectador (`also`, D17).
5. Sustituir el reloj de vueltas propio por los tiempos oficiales cuando Swofty los exponga en `live` (o integrar
   el lector de Project Halo, con su atribución).
6. Publicar la build PUBLIC en el Addon Browser (PR al registro de Swofty con el `.swoftyaddon` y `repository`).
7. Si consigues la fuente Formula1, déjala en `assets/private/fonts/` para el visor (en el juego ya se usa la
   fuente del propio HUD).
