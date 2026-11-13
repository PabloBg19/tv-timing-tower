# Resumen — TV Timing Tower

## 0.14.0 (13-11-2026)

- Franjas sin fundido (la animación de Swofty no es fiable y a veces las dejaba invisibles): salen siempre (D74).
- Más largas y grandes, como la emisión (320 × 31 FASTEST LAP, 296 × 31 NEW RACE LEADER), centradas en un punto que
  se ajusta con una captura de pantalla completa; logos de equipo más grandes en ellas.
- FASTEST LAP sustituye al aviso del juego: la notificación central del juego se oculta mientras sale la nuestra, y
  sale también con la primera vuelta rápida.

---

## 0.13.2 (13-11-2026)

- Fuera la variante de 24 coches de la 0.13.0 (bloque sobre el logo): tenía un fallo que quitaba el logo de la
  cabecera en toda la carrera con 24 coches, y el usuario quiere todas las torres iguales. El bloque va siempre bajo
  la cabecera; con 24 coches y bandera, escalar en el editor de HUD (D73).
- Amarilla en tres sectores: `ALL SECTORS` (antes `SECTORS 1, 2 & 3` se salía del bloque); textos de los bloques
  limitados a su caja.

---

## 0.13.1 (13-11-2026)

- Escudos del juego más grandes (cuadrado de 19 en vez de 13, centrado en el hueco entre la posición y el nombre):
  en temporadas antiguas los logos alargados salían diminutos (D72).

---

## 0.13.0 (13-11-2026, comentario de OverTake, más de 400 descargas)

- Temporadas que no son la 2026: un equipo salía con el icono de Kick. Los iconos de la tele son los de 2026 y se
  elegían por la clave del escudo (`alfa_romeo` es Sauber/Alfa Romeo en otros años). Ahora solo las claves de 2026
  (`…_26`) llevan icono; el resto, el escudo del propio juego (D70).
- 24 coches con bandera: el bloque va encima del logo de la cabecera en lugar de debajo, y la torre no crece (D71).
- `build.py`: con Swofty 1.9.41 la comprobación de perfil en una carpeta temporal falla incluso sin addons
  (`the tooltips already have mod_menu`); lo detecta y lo avisa. El perfil real compila bien.

---

## 0.12.0 (13-11-2026)

- Sin tiempos en libres y clasificación (D69): la Q1 de prueba demostró que el addon no puede medirlos (tiempo
  adelantado en el garaje = vueltas simuladas) y Swofty no los da, ni por memoria ni por UDP en la build Steam.
  La lógica se queda apagada para cuando Swofty dé los tiempos del juego. Fuera la línea `laps:` del log.

---

## 0.11.0 (13-11-2026)

- **Tiempos de clasificación y libres, por fin.** El log de una Q1 dio `10 line wraps, 0 crossings`: en esas
  sesiones el contador de vueltas del juego no sube, y el reloj solo cronometraba cuando subía. Ahora el paso por
  meta es la distancia volviendo a 0 o el contador, lo que llegue antes (en carrera llegan los dos y se cuenta uno)
  (D68).
- FASTEST LAP rehecha como la de la emisión: esquinas redondeadas, estela de tres flechas, fondo casi negro y
  apellido, logo y tiempo pegados a la derecha (D67).
- Sin ver en el juego.

---

## 0.10.0 (13-11-2026, primera parte de la 1.0)

- Franjas arriba en el centro, como en la emisión: `NEW RACE LEADER` (apellido, bloque del color del equipo y su
  logo donde la tele pone la cara) y `FASTEST LAP` (etiqueta morada, logo, apellido y tiempo). 6 s cada una, con
  fundido; si coinciden, una detrás de otra (D67).
- Ajustes para quitarlas y escenario demo «Avisos». Tests del modelo y visor actualizados.
- Sin ver en el juego: la posición exacta (calculada con las proporciones de la emisión respecto a la torre).

---

## 0.9.0 (13-11-2026, comentarios de OverTake)

- Ajuste *Race gap decimals*: 2026 (`+0.9`) o Clásico (`+0.934`) (D65).
- Versión EA: fuera el `game_builds` de Steam del manifiesto, que hacía que el launcher avisara de que el addon era
  para otra versión del juego (D66). Swofty lee la versión EA; sin probar allí.

---

## 0.8.2 (13-11-2026)

- Clasificación sin ningún tiempo en el juego: el reloj de vueltas ya no confunde el paso por meta (la distancia
  vuelve a 0 un momento antes o después que el contador del juego) con un coche que retrocede, que anulaba la vuelta.
- Línea `[tv-tower] laps:` en el log cada minuto en libres y clasificación, para ver qué pasa si sigue sin tiempos
  (D64). Sin ver en el juego.
- El cierre del juego de la 01:24 lo provocó un `swofty build` del perfil lanzado desde aquí con el juego abierto
  (reescribe los archivos de interfaz). `build.py` ya no hace su comprobación con el juego abierto, y no se vuelve a
  instalar ni compilar el perfil real mientras se juega.

---

## 0.8.1 (13-11-2026)

- La torre no salía: se había pulsado F8 en un menú de la carrera (Team HQ) y quedaba oculta en todas las
  sesiones siguientes. Ahora F8 solo actúa dentro de una sesión y cada sesión nueva vuelve a mostrar la torre (D63).

---

## 0.8.0 (13-11-2026, reportes tras 100 descargas)

- Arreglado el corte mal hecho en Q2/Q3 (se cortaba en el piloto 13 en vez de donde correspondía): `field_` no se
  reinicia entre rondas de la misma clasificación, y una Q2 vista sin Q1 tampoco se corta (D58).
- Arreglados los tiempos que no salían en libres y clasificación: si el filtro de coherencia no encuentra nada,
  se muestra el tiempo real del coche en vez de dejarlo en blanco (D59).
- Arreglado IN PIT para que salga siempre, también durante GAINED/LOST, PIT STOPS, TYRES y con Safety Car (D60).
- `SC ENDING` sustituye a `SAFETY CAR IN THIS LAP`: sale cuando el coche de seguridad entra al pit lane y sigue 4 s
  tras el relanzamiento (D61).
- La torre sube un poco más con 24 coches (Mi Equipo); cantidad estimada, pendiente de confirmar en el juego (D62).
- Tests nuevos para cada arreglo y para el caso de 24 coches.
- Revisado después con Opus: Q2 sin Q1 e IN PIT con Safety Car, que seguían fallando.
- Verificado: tests, compose y build de perfil completo. Sin ver en el juego.

---

## 0.7.2 (12-11-2026)

- Fuera el logo de la cadena (DAZN) y la función `overlay` (D57).

---


## 0.7.1 (12-11-2026)

- Final de carrera como en la emisión: sin banda ni tira a cuadros; diferencias en meta y bandera en cada coche
  (D55).
- Sanciones: cuadrado rojo con «!» (D56).

---


## 0.7.0 (12-11-2026)

- `STARTING TYRES` en parrilla y en la vuelta de formación (D51).
- Vista `TYRES` con las vueltas de cada juego de neumáticos (D52).
- `FINAL CLASSIFICATION` con las diferencias en meta con el ganador (D53).
- Ajustes en inglés y español (D54).
- Demo: escenarios nuevos «grid» y «tyres».
- Verificado con tests (vistas nuevas), compose y build de perfil completo. Sin ver en el juego.

---


## 0.6.0 (12-11-2026, lista para publicar)

- Un solo paquete con logos: `dist/tv-timing-tower.swoftyaddon` (D43).
- Corregido el reloj de vueltas que anulaba los tiempos de todos los coches (D44). Diferencias de libres y
  clasificación como en la TV, incluida la zona de eliminación (D45).
- 24 coches y Mi Equipo, con ficha de color si no hay escudo (D46).
- Rendimiento: de 82 µs por fotograma a unos 20 µs por actualización, 30 veces por segundo (D47).
- Fundido más sobrio (D48), textos dentro de su caja (D49), PIT STOPS y FORMATION LAP (D50).
- Verificado: tests en host (nuevos para el reloj de vueltas, clasificación, libres, 24 coches, paradas y formación),
  compose y build de perfil completo del paquete, y el visor.
- Sin ver en el juego: lo nuevo de esta versión y el reloj de sesión de la 0.5.

---


## 0.5.0 (12-11-2026)

- Texto alineado con logos y cajas (D38).
- Reloj de sesión según el método de Halo (D39).
- Intervalos desde la vuelta 1, `IN PIT` en menta, `LAP n / N` bien espaciado (D40).
- Fundido de filas en los adelantamientos (D41).
- Verificado: tests, compose y build de perfil completo de los dos paquetes.
- Sin ver en el juego: el reloj, el fundido y la corrección vertical.

---


## 0.4.1 (12-11-2026)

- Arreglo del error al pulsar Jugar con la 0.4.0 (`os error 3`): los sprites desde textura del juego rompían el
  build de perfil de Swofty (D37).
- `build.py` hace un build de perfil completo de cada paquete en un directorio temporal.
- Reinstalado en tu perfil; `swofty build` del perfil `default` termina bien.

---


## 0.4.0 (12-11-2026, réplica de las capturas 2026)

- La geometría está medida sobre las capturas: torre de 6,3 filas de ancho (antes 10,3) y columnas, cabecera,
  banda LAP y bloques al píxel (D33).
- Los colores están muestreados de las capturas.
- Iconos de equipo con los colores de la emisión (D34) y logo FIA real en los bloques (D35).
- `IN PIT` en rojo en carrera.
- Verificado en el visor lado a lado con las capturas (vueltas 4/55, 26/71 con VSC y 10/55 con SC), con tests y
  con el compositor de Swofty contra el juego.
- Sin ver en el juego: cómo queda la fuente del juego con estas medidas y lo pendiente de las versiones 0.2 y 0.3.

---


## 0.3.0 (12-11-2026, con capturas reales de la torre 2026)

- Bloques de bandera con escudo FIA (`SAFETY / CAR`, `VIRTUAL / SAFETY CAR`…) y sin marco (D29).
- Nombres automáticos: abreviaturas cuando hay datos y apellidos cuando no. En la vuelta 1, con SC y con bandera
  roja, solo nombres (D30).
- `GAINED / LOST`, con la parrilla guardada por el addon (D31).
- `Interval`, `Leader`, `Out`, `+1 Lap`; subtítulo `FIA Formula 1 World Championship` / `SPRINT` (D32).
- Verificado: tests en host (incluida la parrilla grabada en la vuelta de formación y congelada en la salida),
  compose contra el juego (~4.670 enlaces) y el visor.
- Sin ver en el juego: la textura FIA, la parrilla real y los cambios de la 0.2.0.

---


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
