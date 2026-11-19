# Resumen — TV Timing Tower

## 0.26.0 (19-11-2026, en prueba): escudo de Mi Equipo desde el estado en vivo de Swofty

- Swofty (1.9.83) da ya el escudo de Mi Equipo: `live::team_badge_texture` (tabla v3) devuelve la ruta que usa el
  propio juego (`eaid://ui_package/art/textures/render_badge_<tabla> <índice>`), sacada de la asignación guardada de
  la carrera de Mi Equipo. Se usa para los coches sin clave de escudo; la lectura propia de la 0.25 (`mtbadge.hpp`)
  se quita: podía coger el escudo de reserva del inscrito, que según Swofty no siempre es el de la partida.
- SDK de Swofty actualizado (tabla en vivo v5). Swofty 1.9.83 ya da el estado en vivo en el build nuevo de Steam
  (26566308), así que la torre debería volver a salir.

## 0.25.1 (17-11-2026, en prueba): repaso antes de publicar

- README del paquete al día (tiempos en libres y clasificación, tarjeta, vuelta rápida, constructores, Mi Equipo,
  filas que se deslizan, logos del juego por defecto, qué pasa tras una actualización del juego, eliminar la
  versión anterior antes de instalar).
- Textos de ajustes: Animaciones (deslizan), campeonato (pilotos y constructores), clasificación (también corta y a
  una vuelta).
- La búsqueda por nombre del sistema de tiempos se rinde tras 120 intentos en lugar de repetirse siempre.
- Escudo de Mi Equipo: la lista de coches se lee entera y cada id de vehículo tiene que salir una sola vez.

## 0.25.0 (17-11-2026, en prueba): el escudo de Mi Equipo, leído del juego

- Swofty no da la clave del escudo de Mi Equipo porque el juego no lo saca de `team_badges_hud`: lo renderiza en
  ejecución. Leído del constructor de escudos del HUD (0x1417617a0 en el build 1794312636) y del de nombres de
  textura (0x14bd1e150): `g_Game_Manager → +0xc0 → +0x88 → +0x260` es la sesión, sus coches de `+0x2e0` a `+0x2e8`
  (0x88 bytes, id de vehículo en `+0x82`), y el escudo en `+0x68` (tipo) y `+0x6a` (id). La textura es
  `eaid://ui_package/art/textures/render_badge_<tipo> <id>` (`ePreLoaded`, `eUserDefined`, `eNetworkReplicated`).
- Solo para coches sin clave de escudo; lecturas protegidas y comprobadas (tamaño de la lista, id, tipo). Si algo no
  cuadra, queda la ficha de color como antes. El log dice qué textura encontró o por qué no.
- Sin ver en el juego: hace falta que Swofty vuelva a dar el estado en vivo en el build nuevo.

## 0.24.1 (17-11-2026, en prueba): aviso de nuevo líder aunque el segundo vaya pegado

- `NEW RACE LEADER` sale también cuando el nuevo líder lleva 4 s delante, aunque el segundo siga a menos de 0,6 s
  (antes solo con 0,6 s de ventaja y un usuario no lo veía nunca). Con ventaja sigue saliendo al segundo.
- Mi Equipo sin escudo: Swofty no da su clave (`team_badge` vacío) y el HUD no tiene un campo de escudo por coche;
  queda el color del equipo. Haría falta investigar la memoria del juego.

## 0.24.0 (17-11-2026, en prueba): arreglos de los comentarios de Overtake

- **Tras la actualización del juego la torre solo sale en la demo:** Swofty 1.9.59 apaga su estado en vivo en el
  build nuevo (`[live] off: build 1794312636 has no proven live layouts`), y la torre depende de él. No se puede
  arreglar desde el addon: vuelve cuando Swofty dé por buenos los datos del build nuevo. Ahora el log apunta el tipo
  de cada sesión para diagnosticar.
- **Tiempos de clasificación y vuelta rápida en carrera:** el lector de los tiempos del juego solo funcionaba en el
  build de Steam anterior (en EA nunca). Ahora en cualquier otro build toma `g_Game_Manager` de Swofty
  (`api->resolve`) y busca el sistema de tiempos por su nombre de clase (`VehicleLapTimesSystem`), con las mismas
  comprobaciones de siempre.
- **Diferencias con milésimas (`+10.007`, `+1:02.597`) encima del código del piloto:** la caja de la diferencia
  termina 6 unidades antes del código (46 en lugar de 52 y 70) y una cifra larga se comprime dentro.
- **`NEW RACE LEADER` y `FASTEST LAP` encima del logo:** el texto se comprime para acabar antes del bloque del equipo.
- **Tarjeta de clasificación cortada abajo:** su parte de abajo va ahora a la altura de la última fila de una torre
  de 22 coches, en lugar de cerca del borde de la pantalla (en la pantalla de un usuario se salía).

## 0.23.3 (15-11-2026, en prueba): escudos del juego del mismo tamaño que los iconos de la tele

- Con `Logos de equipo: Juego` los escudos salían enormes (tamaño 19, en el juego). Ahora cada equipo tiene el tamaño
  de su icono de la tele en el mismo sitio: el escudo es un cuadrado que su logo llena por el lado largo (medido en
  el juego con Mercedes, Ferrari, Red Bull y McLaren), así que un logo ancho (Red Bull, McLaren) va a 15 y uno
  redondo o alto (Mercedes, Ferrari) a 11, como sus iconos en su hueco de 15 x 11. Igual en los avisos (x2) y la
  tarjeta de clasificación (46 x 30 px de la tele). Un equipo desconocido: 13.

## 0.23.1 (15-11-2026, en prueba): sin `No Time` y logos del juego por defecto

- Un coche sin tiempo en clasificación ya no pone `No Time`: con nuestra fuente, más ancha que la de la tele, tapaba
  todos los apellidos largos (Q2 antes de la primera vuelta, en el juego). Queda `-` de posición y sin neumático.
- `Logos de equipo` pasa a `Juego` por defecto: los escudos del propio juego, que siguen a los mods de logos (el
  usuario cambió los de Williams y Alpine y seguían los iconos de la tele). `TV` sigue en los ajustes.

## 0.23.0 (15-11-2026, en prueba): Out Lap, No Time y campeonato de constructores

- Como la torre de clasificación de la tele (vídeo del usuario): `Out Lap` en lugar de la diferencia para un coche en
  su vuelta de salida (P1 conserva su tiempo), y `No Time` con `-` de posición y sin neumático para un coche sin
  tiempo. La vuelta de salida: el reloj de vueltas recuerda si el coche pasó por el pit lane desde que cruzó la meta.
- Vuelta de formación: tras los neumáticos (20 s) y el campeonato de pilotos (30 s), 20 s de `CONSTRUCTORS'
  CHAMPIONSHIP`: una fila por equipo con su logo, su nombre y sus puntos, la suma de los puntos de sus pilotos en la
  tabla del propio juego según el equipo de su coche en esta carrera. Solo si cada piloto con puntos tiene coche aquí
  y se conoce cada equipo (un reserva que puntuó o un equipo retro / Mi Equipo: no sale). Empates: el equipo del
  piloto mejor clasificado primero.

## 0.22.0 (15-11-2026, en prueba): adelantamientos deslizando, como en la tele

- En el vídeo de la tele, la fila que sube se desliza desde su sitio antiguo hasta el nuevo (~0,5 s) y las que
  adelanta bajan un puesto. Ahora igual: el contenido de cada fila (logo, nombre, datos) va dentro de diez
  deslizamientos anidados (1, 2, 4, 8 y 16 filas hacia abajo y hacia arriba). Cuando llega un coche desde k filas,
  el DLL enciende los que suman k mientras el contenido viejo se funde, cambia el coche y los apaga: el nuevo se
  desliza desde donde estaba hasta su sitio mientras aparece. El fondo y el número de posición se quedan quietos.
- Hasta 12 filas cambiando a la vez (antes 4); más (una salida, un reinicio): cambio directo. Ajuste `Animaciones`
  para quitarlo. Editor del HUD: ~54.400 de 65.536 (techo nuestro 60.000).

## 0.21.0 (15-11-2026, en prueba): la tarjeta de clasificación como la de 2026

- Rehecha con los vídeos de la tele de 2026 del usuario (medida en píxeles): el tiempo corre en gris hasta el primer
  parcial y en blanco después; al pasar un parcial, en su lugar sale la diferencia en grande (verde, morado o
  amarillo) 4,5 s con `SECTOR n` debajo; a la derecha, el tiempo del objetivo hasta el final del sector en curso
  (su parcial 1, luego S1+S2, luego la vuelta) y su nombre, centrados; al cerrar la vuelta, la diferencia con
  `SECTOR 3` y luego el tiempo. Sin la píldora `CUT P16` ni el aro del neumático (no están en 2026), cabecera más
  clara que el cuadro de la posición, `S1 S2 S3` centrados en tercios, barras grises claras.

## 0.20.2 (15-11-2026, en prueba): lecturas más baratas

- El crash de la sesión 20261115-123320 (error de memoria del propio juego al cargar unos libres de Modo
  Carrera) no viene de la torre: nuestro DLL no tenía estado en vivo y no leyó nada en esa sesión.
- Revisando los logs: en 0.18.x la lectura de tiempos gastaba ~3,9 s de cada minuto en comprobaciones de memoria
  (VirtualQuery, ~1,4 ms cada una, en un hilo del juego). Ahora una comprobación vale para toda la región de
  memoria durante 2 s y se descarta al cambiar de sesión (`native/src/guarded_read.hpp`).

## 0.20.1 (15-11-2026, en prueba)

- Probada la 0.20.0 en carrera: la franja y el icono de vuelta rápida salen bien. El tiempo morado de la fila
  (1:07.743) se metía en el nombre: una fila con un tiempo completo usa ahora la columna estrecha de libres y
  clasificación (más a la derecha y cifras algo más pequeñas), también en carrera.

## 0.20.0 (15-11-2026, en prueba): vuelta rápida en carrera, la del juego

- En carrera también se leen los tiempos del juego, y la vuelta rápida es la suya: la vuelta válida más rápida de
  la carrera, la misma que anuncia el aviso «Mejor vuelta» del juego. Vuelven la franja FASTEST LAP (piloto, logo
  y tiempo; quitada en 0.15, D76), el destello morado del tiempo y el icono morado en la fila del que la tiene
  (quitados en 0.15.1, D77). Mientras sale nuestra franja, el aviso central del juego se oculta.
- Solo con los tiempos del juego: si no se pueden leer, ni franja ni icono (el reloj propio no coincidía).
- Una vuelta rápida que ya existía al entrar en una carrera empezada no se anuncia. Ajuste `Aviso de vuelta rápida`.

## 0.19.0 (15-11-2026, en prueba): tarjeta de clasificación

- En clasificación (Q1/Q2/Q3, SQ1/SQ2/SQ3), a la derecha de la parte baja de la torre, la tarjeta de tu vuelta como
  en la TV: posición, logo, apellido y neumático; el tiempo de la vuelta corriendo (décimas) y, al cerrarla, en
  milésimas durante 7 s; la diferencia en el último parcial con la vuelta objetivo (morado si mejoras a P1, verde si
  mejoras el corte, amarillo si no); el objetivo a la derecha (P1, o `CUT P16` y el piloto del corte si estás en la
  zona de eliminación); y las barras S1 S2 S3 (morado mejor de la sesión, verde tu mejor, amarillo más lento).
- El tiempo en curso es el reloj del addon (se para con la pausa); los parciales se miden al pasar el final de
  cada sector y se cambian por los del juego cuando coinciden. Vuelta de salida y vuelta de enfriamiento: sin
  tarjeta. Ajuste `Tarjeta de clasificación` (activado). Medidas sacadas de capturas de la tele (388 x 127 px).

## 0.18.1 (15-11-2026, en prueba): la columna de tiempos de libres y clasificación

- Probada la 0.18.0 en Q1: los tiempos salen. El tiempo del líder (1:05.173) se metía en el nombre. En libres y
  clasificación la columna acaba más a la derecha y sus cifras son algo más pequeñas, como la torre de clasificación
  de la tele (medida en una captura de Q1 de 2026). En carrera no cambia nada.

## 0.18.0 (15-11-2026, en prueba): tiempos de libres y clasificación, del propio juego

- Los tiempos de vuelta se leen del sistema de tiempos del juego (`VehicleLapTimesSystem`), los mismos que el juego
  muestra y envía por UDP («Session History»), vueltas simuladas incluidas (`native/src/laptimes.hpp`). Camino y
  estructura sacados del ejecutable (Steam, `d873a1c8`): el registro de sistemas como el núcleo de Swofty, la clave
  0x146096ae8, por coche +0x58 → +0x38, 200 vueltas de 0xc8 con los tiempos cifrados con XOR.
- Solo en esa versión del juego (sello del ejecutable), con lecturas protegidas, y solo si los datos cuadran (cada
  vuelta = suma de sus sectores). Si no: sin tiempos, como antes. El log dice `[tv-tower] lap times: ...`.
- La mejor vuelta es la del juego: la más rápida válida (no cuentan las vueltas anuladas por límites de pista).

---

## 0.17.4 (15-11-2026): sin colores sueltos en las filas

- En el juego, un sprite teñido con el color del coche no respeta su propia visibilidad (visto en la torre 2021): en
  la 0.17.3 la ficha de color de equipo y la barra de color opcional salían siempre. La ficha vuelve a su grupo; la
  opción «Team colour bar» desaparece (para no pasar del límite del editor de HUD). Editor de HUD: ~47.100 de 65.536.
- El build rechaza un sprite teñido con visibilidad propia (`tools/idcheck.py`).

---

## 0.17.3 (14-11-2026): el editor de HUD ya no cierra el juego

- Aviso de Swofty: abrir la personalización del HUD cerraba el juego a dos jugadores (0.7.2). Esa pantalla recorre la
  torre seis veces en un árbol de 65.536 huecos; la 0.17.2 usaba unos 69.200. Las 30 ranuras de coche de cada fila
  pierden sus grupos envoltorio (cada texto y sprite lleva su propia visibilidad): ahora unos 51.500, con margen. La
  torre se ve y funciona igual (D81).
- El build mide el coste con la misma regla que Swofty (`tools/walkcost.py`) y se para por encima de 60.000.

---

## 0.17.2 (14-11-2026): para el Addon Browser de Swofty

- Ficha para el Addon Browser: descripción de una línea, enlace al código (`repository`) y un icono nuevo (la propia
  torre, sin logos de F1, FIA ni equipos). Sin cambios en la torre.

---

## 0.17.1 (14-11-2026)

- Clasificación de la vuelta de formación: sale el piloto creado por el jugador (su fila es de otro tipo y se
  saltaba; se reconoce por la marca de jugador y, si hace falta, como el único de la tabla que no está en la lista);
  los puntos van al borde de la fila y algo más pequeños, sin pisar los apellidos largos (D80). En el juego, la 0.17.0
  ya leyó la tabla bien (`22 drivers, leader 135 pts`).

---

## 0.17.0 (13-11-2026)

- Clasificación de pilotos en la vuelta de formación (modo carrera), como la emisión: 20 s de neumáticos de salida y
  30 s de `DRIVERS' / CHAMPIONSHIP` con los pilotos por puntos. Los puntos son la tabla del propio juego (sprints y
  puntos extra incluidos), leída de su memoria con lecturas protegidas; si algo no cuadra, no sale y la torre sigue
  normal (D79). Ajuste para quitarla, escenario demo, tests con una memoria falsa.

---

## 0.16.0 (13-11-2026)

- STARTING TYRES como la emisión: abreviaturas y el nombre del compuesto (`MEDIUM`) en su color (D78).
- NEW RACE LEADER solo cuando el nuevo líder ya saca 0,6 s al segundo, no mientras pelean (D78).
- El campeonato de pilotos en la vuelta de formación no se puede hacer: Swofty no da los puntos (D78).

---

## 0.15.1 (13-11-2026)

- Fuera el icono morado de vuelta rápida y el destello del tiempo en morado en las filas: salían del reloj propio y no
  siempre coincidían con el juego. Queda apagado (`Config::race_fastest`) para cuando Swofty dé los tiempos (D77).

---

## 0.15.0 (13-11-2026)

- Fuera la franja FASTEST LAP y su ajuste (D76): el juego no da tiempos a los addons, así que nunca coincidía con el
  aviso «Mejor vuelta» del juego, que vuelve a salir sin ocultarse. Se queda NEW RACE LEADER (el orden lo da el juego),
  centrada y más grande.

---

## 0.14.1 (13-11-2026)

- Franjas centradas y a la altura del aviso del juego, medido en una captura de pantalla completa (2000 × 1125); un
  23 % más grandes (37 % del ancho de la pantalla, como la tele) (D75).
- El fondo de las franjas pasa a la capa 20: en la 40 tapaba el escudo del juego (que siempre va en la 30) y el texto
  FASTEST LAP no se veía.
- Vuelta rápida más fiable: una vuelta por debajo de 75 m/s de media no existe (la de 0:59.0) y la de la sesión no
  puede quedar más de un 4 % por debajo de la mediana de las vueltas medidas (el juego decía HAMILTON y el addon ERI).

---

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
