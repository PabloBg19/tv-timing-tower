# Decisiones tomadas sin consultar

Cada decisión: qué, por qué y cómo revertirla.

**D1 — Motor de dibujo: el propio HUD del juego (`hud.element`, pantalla `standings`).**
La torre se compone dentro del tablero de posiciones nativo con el compositor de Swofty. Ventajas: nombres,
colores de equipo, logos del juego, reloj de sesión y total de vueltas salen del view model del juego (la API
`live` no los da); el tablero nativo queda oculto solo mientras nuestra hoja `show` es verdadera, así que si el
DLL falla o la build no cuadra vuelve el original (fallo seguro); coste de rendimiento ≈ el del tablero nativo;
el editor de HUD del juego mueve/escala/da opacidad. Alternativa descartada: overlay inmediato de Swofty (sin
nombres de piloto sin UDP; dibujaría encima del HUD sin respetar su ocultación).

**D2 — Datos: `swofty::live` (memoria, sin UDP)**, con `experimental_features: ["telemetry"]`. Activa en este
PC con todos los grupos. Sin hooks propios ni escaneo de memoria propio: no dependemos de offsets propios de
la build; si Swofty no verifica la build, `live` devuelve vacío, la torre se apaga y vuelve la nativa, y se
escribe una línea en el log.

**D3 — Overlay solo para el logo de cadena (arriba a la derecha).** Opción `Logo de cadena` (PRIVATE: DAZN,
activado; PUBLIC: no existe). Si el overlay no está disponible se ignora. Requiere `overlay` experimental solo
en PRIVATE.

**D4 — Escala, posición y opacidad general: el editor de HUD del juego** (Ajustes → HUD → Personalizar, o
pausa en sesión), por diseño de producto de F1 25 y porque los registros HUD no aceptan parámetros en build.
Además el addon ofrece `Fondo` (3 niveles de opacidad del fondo de la torre, en caliente) para que el texto
siga 100 % legible con fondo más transparente.

**D5 — Tiempos de vuelta calculados por el addon.** `live` 1.9.14 no da tiempos. Se miden los cruces de meta
(cambio del contador de vuelta) con interpolación por distancia y un reloj que se congela con el juego en
pausa. Vuelta inválida si suben los avisos de límites de pista durante la vuelta, o si contradice el orden del
juego (filtro de coherencia con la clasificación). Precisión esperada ±0,02 s. Si Swofty añade tiempos al
`live`, sustituir `LapClock` por la lectura oficial.

**D6 — Formato de gaps 2026:** un decimal truncado en carrera (`+0.9`), tres decimales en libres/qualy
(`+0.234`, líder `1:27.345`). `+1 LAP`/`+2 LAPS` por contador de vueltas.

**D7 — Intervalo/líder automático como TV:** por defecto intervalos; el gap al líder se muestra 8 s cada vez
que el líder completa una vuelta múltiplo de 5 y durante las 2 vueltas siguientes a SC/VSC. Modos fijos
disponibles en ajustes.

**D8 — SC/VSC ENDING inferidos.** El juego no expone el aviso "ending". SC ENDING (`SAFETY CAR IN THIS LAP`):
cuando el coche de seguridad (vehículo no-racer) entra en el pit lane o se desvía hacia él. VSC ENDING: los
primeros 4 s tras desactivarse el VSC se muestra `VSC ENDING` y después `TRACK CLEAR` 4 s (aproximación).

**D9 — Logos (0.1.0; sustituida por D23 en 0.2.0).** PRIVATE: PNG reales (descargados a `assets/private/logos`, fuera de git) como sprites del HUD,
elegidos por la clave de escudo del juego (`team_badge`). Si un equipo no tiene PNG, se usa el escudo del
propio juego (`unit: team_logo`). PUBLIC: solo el escudo del propio juego por referencia (no se distribuye
ningún logo) y logo de serie del juego en la cabecera.

**D10 — Tipografía.** Dentro del juego se usan los estilos de texto del propio HUD de F1 25 (familia
Formula1, ya en el juego, no se distribuye). Un registro HUD no puede cargar TTF externos. La fuente de
`assets/private/fonts` (o Titillium Web si no hay) se usa en el visor de diseño HTML.

**D11 — Filas:** 22 filas y 24 ranuras de coche (car0…car23, el máximo que nombra el juego) para respetar el
límite de 1024 copias del compositor. Parrillas > 24 coches (mods) no se cubren.

**D12 — Animaciones nativas (`motion`) desactivadas en 0.1.0.** Swofty avisa de que no se han visto en juego.
Los cambios de posición se marcan con flechas verde/roja 3 s (sin riesgo). Siguiente paso documentado.

**D13 — Tecla por defecto F8** para mostrar/ocultar (F1–F12 elegibles, u "off"). Se lee con
`GetAsyncKeyState` solo con la ventana del juego en primer plano. Ocultar devuelve el tablero nativo.

**D14 — Modo demo.** Se activa con un ajuste o con Ctrl+tecla; recorre escenarios (carrera, qualy con
eliminados, SC, VSC, bandera roja, final) con datos simulados dentro de cualquier sesión (incluido Time Trial).
Además hay un visor HTML (`preview/`) que usa el mismo modelo C++ compilado para host, para validar el diseño
sin abrir el juego.

**D15 — Sin red.** El DLL no importa `ws2_32` ni WinINet/WinHTTP (comprobado en el build con `objdump`); el
manifiesto lo declara en `description` y en `network: "none"`.

**D16 — Licencias.** Código MIT. SDK de Swofty (MIT) vendorizado en `native/include`. Las ideas de Project
Halo (contrato de filas, regla de eliminación) están reimplementadas; se acredita en README.

**D17 — Solo el HUD de sesión en 0.1.0 (sin repeticiones ni modo espectador).** El compositor puede poner la
misma torre en la pantalla de repeticiones y en la de espectador (`also`), pero en este PC el build avisa de que
esas pantallas cambian de layout y tienen que ir "built into the game files", y no hay forma de probarlas sin
jugar. Para minimizar riesgo quedan fuera; en esas pantallas se ve la torre del juego. Reactivar: añadir
`"also": {"replay": {"show": vm("on")}, "spectator": {"show": vm("on")}}` en `tools/hudgen.py`.

**D18 — Un único id para los dos sabores** (`pablo.tv-timing-tower`): instalar uno sustituye al otro y nunca hay
dos torres compitiendo por el mismo tablero. PRIVATE lleva textos de ajustes en español; PUBLIC en inglés.

**D19 — "Permisos" del manifiesto.** El formato de Swofty no tiene campo de permisos y rechaza campos
desconocidos (`deny_unknown_fields`). Lo equivalente son `kind: ["ui","native"]` (etiqueta *Code*) y
`experimental_features: ["telemetry","overlay"]` (etiquetas *Reads live race data* y *Draws over the game*).
La declaración "solo offline, sin red, sin antitrampas" va en `description`.

**D20 — Flechas de posición solo en carrera** (en TV no se usan en libres/qualy) y, con bandera roja, sin
diferencias en la columna derecha (los coches esperan en el pit lane).

**D21 — Modo demo: mejor en una sesión de libres.** En Contrarreloj el juego enseña su propio tablero de
tiempos, no el de posiciones, así que la torre (que vive dentro de este) puede no verse allí.

## 0.2.0 (tras la primera prueba en el juego, 12-11-2026)

**D22 — Torre más baja y más arriba.** Filas de 18,0 unidades (antes 21,6), cabecera de 38 y tarjeta de 16; toda
la torre sube 40 unidades (`offset_y`), al hueco del contador de vueltas que ya se oculta. Con 22 coches cabe entera
en pantalla. Ancho 186 y nombres con `clampwidth` en 78 unidades para que los apellidos largos no pisen la
diferencia. Se sigue pudiendo mover/escalar en el editor de HUD.

**D23 — Logos de equipo: siempre los del juego, en color.** El juego tiene dos juegos de escudos con la misma
clave: `osd.timing_tower.team_badges_hud` (blancos, los de la torre del HUD) y `team_badges_broadcast` (los de la
torre de retransmisión del juego). Por defecto se usa el de retransmisión. Como son texturas del juego, un mod de
logos cambia también la torre. Ajuste *Logos de equipo en color* = No vuelve a los blancos. Se quitan los PNG privados de las
filas (eran blancos y el mapeo fallaba: el juego llama `alfa_romeo_26` a Audi, `alphatauri_26` a Racing Bulls y
`gm_cadillac_26` a Cadillac). Los PNG privados quedan solo para el visor HTML.

**D24 — Neumático: solo la letra del compuesto en su color**, como en la torre de 2026 (captura real de
Australia 2026 P1: `LEC 1:21.227 M P`). Sin círculo.

**D25 — (sustituida por D29) Banderas con marco.** Con SC, VSC, sus finales, bandera amarilla o bandera roja la
tarjeta de color va pegada bajo la cabecera y **toda la torre lleva un marco de ese color** (amarillo o rojo).
Textos: `SAFETY CAR`, `SAFETY CAR ENDING`, `VIRTUAL SAFETY CAR`, `VIRTUAL SAFETY CAR ENDING`, `RED FLAG`,
`YELLOW FLAG S2`, `TRACK CLEAR` (sin marco). Fuente: capturas de la torre FOM y su rediseño público (ver
INVESTIGACION.md §5); no se encontró una captura oficial de 2026 con SC, así que se pide confirmación con una
captura real.

**D26 — Q2/Q3 (SQ2/SQ3): los eliminados salen de la torre**, como en TV (antes se atenuaban). Q2 muestra 16 filas
y Q3 10. Con *Top 10 + yo* y el jugador eliminado, solo el top 10.

**D27 — Nombres por coche sin cambios bajo un nodo oculto.** En la prueba salió una fila con dos apellidos
superpuestos al salir de la demo: el juego no aplica un cambio de visibilidad dentro de un grupo oculto, así que un
nombre apagado mientras su grupo estaba oculto volvía a verse. Ahora los interruptores de coche (`rNN_cVV`) solo
cambian con la fila visible: no hay grupo padre ligado a la demo, una fila oculta conserva su valor y apagar la
torre ya no toca las filas. Limitación: si el propio juego oculta el tablero (garaje, menús) y el orden cambia
mientras tanto, podría repetirse; si pasa, anotar cuándo.

**D28 — Reloj de la cabecera con pulso.** El formateador del reloj del juego solo se ejecuta cuando su hoja de
arranque pasa de 0 a 1; en la prueba se activó antes de existir el HUD y el reloj no salió. Ahora la hoja `clk`
pulsa 4 veces por segundo en sesiones cronometradas.

## 0.3.0 (capturas reales de la torre 2026 aportadas por Pablo)

Referencia: cinco capturas de la señal internacional de 2026 (carrera vuelta 8/71, VSC vuelta 26/71, vuelta 4/55
con gaps al líder, Safety Car vuelta 10/55, sprint vuelta 2/24 en modo GAINED / LOST).

**D29 — Bloques de bandera con el escudo FIA, sin marco.** Bajo `LAP x / y` va un bloque de color de 26 unidades
con el escudo FIA (textura del propio juego `ms_fia_logo.tif` sobre un cuadrado azul) y el texto en negrita a la
izquierda, en dos líneas: `SAFETY / CAR`, `VIRTUAL / SAFETY CAR`, `SAFETY CAR / IN THIS LAP`, `VSC / ENDING`,
`YELLOW FLAG / SECTOR 2`. `RED FLAG` y `TRACK CLEAR` van en una línea. La torre no lleva marco (se quita D25).

**D30 — Nombres automáticos y vista "solo nombres".** Como en TV: con datos al lado (gaps, tiempos) se ven las
abreviaturas (`RUS`, `VER`) y sin datos, los apellidos completos. En carrera, con *Diferencias* = Automático, la
vuelta 1, el Safety Car y la bandera roja muestran solo los nombres, sin gaps ni neumáticos. El VSC mantiene los
intervalos. Ajuste *Nombres*: Automático (por defecto) / Apellido / Abreviatura.

**D31 — Posiciones ganadas/perdidas.** Barra `GAINED / LOST` bajo la cabecera y, en cada fila, ▲ verde o ▼ roja
con el número de puestos, o una raya gris si es 0. La parrilla la guarda el addon porque `live` no la da: es el
orden durante la vuelta de formación (estado `formation` del SC) o en los primeros instantes de la vuelta 0, y se
congela con el primer cambio de orden tras la salida. En Automático sale 10 s al empezar la vuelta 2; también es un
modo fijo de *Diferencias*. Si se entra en una carrera ya empezada no hay parrilla y no sale.

**D32 — Textos como la señal 2026:** `Interval`, `Leader`, `Out` y `+1 Lap` en mayúscula inicial; el neumático,
como letra de color tras el dato. En la cabecera, el logo F1 y debajo `FIA Formula 1 World Championship` (solo
PRIVATE), o `SPRINT` en la clasificación sprint. En la carrera sprint no sale `SPRINT`, porque el juego no la
distingue con seguridad del Gran Premio.

## 0.4.0 (réplica al milímetro de las capturas 2026)

**D33 — Geometría medida, no estimada.** Todas las medidas salen de las capturas a 4x con rejilla y de perfiles de
luminancia, en píxeles de una fila de 21,5 px (vueltas sin bandera), y se escalan a una fila de 17,5 unidades
(`REF` y `u()` en `tools/hudgen.py`):

- **Torre:** 135 px de ancho, unas 6,3 filas (antes era 10,3).
- **Fila:** columna de posición de 19 px con celdas más oscuras separadas por una línea fina (1,75 px). La caja
  roja de P1 tiene la altura de la celda.
- **Columnas:** logo centrado a 29 px, nombre desde 42 px, dato hasta 117,5 px, letra del neumático a 127 px.
- **Cabecera:** bloque gris pizarra de 49,5 px con el logo F1 (70×17,5 px) y el lockup
  `FIA Formula 1 World Championship`, una línea clara de 1,5 px y la banda oscura de `LAP` de 22 px.
- **GAINED / LOST:** añade 22 px dentro de esa misma banda.
- **Bloques de bandera:** 42 px con dos líneas y 52 px con tres, más una separación de 2 px. Logo FIA de 44 px de
  ancho, texto desde 55 px, mayúsculas de 9,8 px e interlineado de 13,75 px.

Colores muestreados: cabecera #444351→#32303C, banda #181723, filas #26252C, celdas #16151E, amarillo #ECD024,
rojo P1 #E0120F, morado #C35BDC.

Diferencia aceptada: con un bloque de bandera la TV comprime las filas (19,2 px en lugar de 21,5). Aquí el paso de
fila es fijo (las filas son un `repeat` con paso fijo), así que con bandera la torre crece unos 44 px.

**D34 — Logos de equipo como la TV (build PRIVATE).** Los iconos de la torre 2026 se descargan de formula1.com en
la variante que usa la emisión:

- En color: Ferrari, Red Bull, McLaren, Alpine y Haas.
- En blanco: Mercedes, Aston Martin, Williams, Racing Bulls y Cadillac.
- Audi: los aros blancos teñidos de rojo.

Ajuste *Logos de equipo*: TV (por defecto en PRIVATE) / Juego (los escudos en color del juego, que siguen a los
mods de logos) / Juego blanco. Un equipo sin icono (por ejemplo, de un mod) usa siempre el escudo del juego. La
build PUBLIC no lleva iconos; usa Juego por defecto.

**D35 — Logo FIA real.** En los bloques de bandera va el logo FIA de Wikimedia Commons (`FIA logo (black).png`
sobre amarillo, `(white)` sobre rojo o verde), igual que en la emisión. La build PUBLIC usa la textura FIA del
propio juego teñida.

**D36 — Tipografía.** El juego solo ofrece 11 estilos de texto (`OSD_17/19/24/26/30/34`, `17/24/34`), todos de su
familia F1. Medido sobre las capturas, la proporción de la fuente coincide con la de TV (`VERSTAPPEN` ocupa ≈10
alturas de mayúscula en las dos), así que se mantiene la fuente del juego, con los mismos pesos que la TV:

- Negrita: nombres, número de vuelta, bloques y `GAINED / LOST`.
- Regular: posición, diferencias, `Interval`/`Leader`, `LAP` y `/ 71`.
- `IN PIT` en rojo.

El visor HTML usa Titillium Web, más estrecha, así que ahí el texto se ve algo más pequeño que en el juego.

**D37 — Ningún sprite desde textura del juego (0.4.1).** La 0.4.0 se instaló pero el botón Jugar falló: `the wrapper
exited (exit code: 1) ... addons\pablo.tv-timing-tower\: El sistema no puede encontrar la ruta especificada (os
error 3)`. Lo reproduje en un directorio de datos temporal. La causa son los dos sprites dibujados desde una textura
del juego (`"texture": ".../ms_fia_logo.tif"`, el escudo FIA de la build PUBLIC): `swofty hud compose` los acepta,
pero el build de perfil de Swofty 1.9.14 busca un fichero para cada sprite y falla. Sin ellos, el build termina.

Cambios:
- PRIVATE usa sus PNG del logo FIA. PUBLIC no lleva escudo en los bloques y el texto empieza en el margen.
- `hudgen.py` impide los sprites sin PNG.
- `build.py` hace ahora un build de perfil completo de cada paquete (instalar, activar, `swofty build`) en
  `build/<sabor>/swofty-data`, así que un paquete que rompa Jugar no llega a `dist/`.

En tu instalación faltaba además `swofty.json` en la carpeta del addon, tras el fallo. Al reinstalar con
`swofty addons install` quedó completa, y `swofty build` del perfil `default` terminó bien.

## 0.5.0 (segunda prueba en el juego, 12-11-2026)

**D38 — Corrección vertical del texto.** En las capturas del juego, las mayúsculas salían 7 px más abajo de lo
calculado en una fila de 42 px (unas 2,9 unidades, 0,45 alturas de mayúscula): por eso los nombres quedaban por
debajo de los logos y de la caja de P1. Ahora todos los textos del registro suben 0,45 × su altura de mayúscula
(`GAME_TEXT_DY` en `hudgen.py`). El visor HTML dibuja la línea base real.

**D39 — Reloj de la sesión, como Project Halo.** El DLL registra la hoja `hud_tvt_clock` y la escribe una vez
vacía, para que exista el nodo donde el formateador clonado del juego escribe el tiempo. El arranque vuelve a ser
`timed`, como en Halo, que funciona en el juego. Se quita el pulso de la 0.2.

**D40 — Intervalos desde la vuelta 1.** Solo el Safety Car y la bandera roja muestran solo nombres; antes también
la vuelta 1. `IN PIT` va en verde menta (#5ED6B4, muestreado de la captura de la vuelta 15/66) y ocupa también la
columna del neumático. `LAP 1 / 5` tiene tres variantes según los dígitos de la vuelta (`ld1`..`ld3`), para que el
espaciado sea el de la emisión.

**D41 — Animación de adelantamientos: fundido.** En el vídeo de la emisión las dos filas se cruzan deslizándose en
unos 0,3 s, con un triángulo verde en el coche que sube y uno rojo en el que baja. El `motion` del compositor de
Swofty reproduce la animación al revés cuando la hoja vuelve a falso, así que un deslizamiento que vuelva a reposo
sin salto no se puede hacer. Lo más cercano y limpio es un fundido: la fila se apaga (0,22 s) con el coche antiguo,
el DLL cambia el coche con la fila oculta y la fila vuelve a aparecer con el nuevo. Los triángulos se mantienen.
Swofty avisa de que los disparadores de `motion` sobre hojas propias no se han visto aún en el juego. Si no
funcionara, el ajuste *Animaciones* = No deja el cambio inmediato.

**D42 — Tipografía.** El juego no carga fuentes externas en el HUD; los estilos `OSD_*` ya son la familia F1 del
propio juego (misma proporción que la emisión, D36). La única forma de usar el archivo de la fuente oficial sería
sustituir la fuente del juego entero, fuera del alcance de este addon.

## 0.6.0 (para publicar, 12-11-2026)

**D43 — Un solo paquete, con logos.** El juego oficial ya muestra los logos de la F1, de la FIA y de los equipos,
así que se elimina la build PUBLIC. Queda un único paquete `dist/tv-timing-tower.swoftyaddon`, con los iconos de
equipo de la emisión, el logo F1 y los logos FIA. Swofty no traduce los ajustes, así que van en inglés para el
público del registro.

**D44 — Tiempos en libres y clasificación.** Error encontrado al revisar: cuando cualquier coche retrocedía en su
vuelta (las IA vuelven al garaje constantemente), el reloj de vueltas lo tomaba por un flashback y anulaba la vuelta
en curso de todos los coches, así que casi nunca había tiempos. Ahora el retroceso anula solo la vuelta de ese
coche, y se considera flashback únicamente si retrocede más de la mitad del campo a la vez (hay tests para los dos
casos).

**D45 — Diferencias en clasificación y libres, como en la TV.**
- Libres: todos con la diferencia con P1.
- Clasificación: P1 con su vuelta, el resto con la diferencia con P1, y los coches en la zona de eliminación con la
  diferencia con el último que pasa (P16 en Q1 con 22 coches, P10 en Q2).

**D46 — Hasta 24 coches y Mi Equipo.**
- 24 filas, y 30 huecos por coche: Project Halo vio en el juego ids de vehículo hasta 29 en parrillas de Mi Equipo.
- Corte de eliminación 10 + (n−10)/2 (17 con 24 coches).
- El compositor solo permite 4 valores del juego por coche (posición, apellido, abreviatura, color), así que el
  escudo no puede leerse por coche. Un equipo sin clave de escudo (Mi Equipo, equipos con escudo renderizado) lleva
  una ficha con su color (`nub_colour` del juego).
- Nada viene fijado: nombres, abreviaturas, colores, escudos, orden, estados y vueltas totales salen del juego. Los
  iconos de la TV se usan solo cuando la clave de escudo es de un equipo 2026 conocido; si no, sale el escudo del
  juego.

**D47 — Rendimiento.** En la sesión de las 18:44 el addon costaba 82 µs por fotograma de media (11 ms/s, 1,1 % de un
núcleo); era el segundo consumidor después del propio Swofty. Causas: 21 lecturas de ajustes por fotograma a
través de la API del core, y el modelo y las 1.600 hojas recalculados en cada fotograma. Ahora:
- los ajustes se leen 4 veces por segundo;
- la torre se actualiza 30 veces por segundo; la tecla se sigue leyendo en cada fotograma, y la interpolación por
  distancia del reloj de vueltas no pierde precisión;
- coste medido en el PC de pruebas: unos 20 µs por actualización, unos 0,6 ms/s.

**D48 — Fundido más sobrio.** En el vídeo del arranque las filas se desvanecían todas a la vez. Ahora:
- si cambian más de 4 filas en la misma actualización, no hay fundido (salidas, reinicios);
- cada fila hace como mucho un fundido por segundo;
- la columna de posición (número, caja de P1, triángulos) no se desvanece, solo el coche;
- si se desactivan las animaciones en mitad de un fundido, la fila vuelve siempre.

**D49 — Texto dentro de su caja.** En el juego las minúsculas de `OSD_24_regular` salen casi el doble de anchas que
en la TV (`NORInterval`), así que `Interval`/`Leader`, las diferencias e `IN PIT` se comprimen dentro de su caja
(`clampwidth`). `LAP 1 / 5` se separa más; los dígitos del juego son más anchos de lo estimado.

**D50 — Lo que se añade de la investigación de grafismos** (fuentes en INVESTIGACION.md §7):
- `PIT STOPS`: modo fijo y, en Automático, 10 s en las vueltas múltiplo de 10 cuando alguien ha parado. Usa el
  contador de paradas del propio juego.
- `FORMATION LAP` en la cabecera, solo con nombres. Usa el estado `formation` del juego.

Lo que no se puede hacer o queda fuera:
- `FINAL LAP`: el DLL no conoce las vueltas totales.
- Gráficos de batalla, rótulos de piloto, sectores en vivo, avisos de investigación o el rendimiento del neumático:
  no hay datos o son gráficos fuera de la torre.
- Tiempo de parada en boxes: no hay datos.

## 0.7.0 (12-11-2026)

**D51 — STARTING TYRES.** En parrilla y en la vuelta de formación, la banda oscura de `LAP` lleva
`STARTING TYRES`, con los apellidos y la letra del neumático de salida de cada coche. Condiciones: vuelta 0, antes
del primer cambio de orden y con todos los coches parados (velocidad del juego por debajo de 3 km/h), o la vuelta
de formación. Solo en el modo Automático y con los neumáticos activados.

**D52 — Vista de neumáticos (`TYRES`).** La columna de datos muestra las vueltas con el juego actual (la letra
sigue a la derecha). En Automático sale 10 s cuando el líder empieza las vueltas 7, 17, 27…; también es un modo fijo
de *Diferencias en carrera*. Las vueltas se cuentan desde la última parada vista: si se entra a mitad de carrera, se
cuentan desde ese momento.

**D53 — FINAL CLASSIFICATION.** Cuando el ganador cruza con la bandera a cuadros, la banda pasa a
`FINAL CLASSIFICATION` y cada coche que termina muestra su diferencia en la meta con el ganador. El addon la mide en
el momento en que cada coche cambia a «terminado»; la precisión es de una actualización, unos 0,03 s. Los doblados
muestran `+1 Lap` y los retirados `Out`.

**D54 — Ajustes en dos idiomas.** Swofty muestra un solo texto por ajuste y no tiene traducciones. Por defecto
cada etiqueta y descripción lleva inglés y español (`Rows / Filas`), y `python tools/build.py --lang es` (o `en`)
genera el paquete en un solo idioma. Swofty no acepta textos de más de 255 bytes; `build.py` lo comprueba y la
prueba de build de perfil lo detectó.
