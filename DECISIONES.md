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

**D3 — (quitada en D57) Overlay solo para el logo de cadena (arriba a la derecha).** Opción `Logo de cadena` (PRIVATE: DAZN,
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

**D53 — (corregida en D55) FINAL CLASSIFICATION.** Cuando el ganador cruza con la bandera a cuadros, la banda pasa a
`FINAL CLASSIFICATION` y cada coche que termina muestra su diferencia en la meta con el ganador. El addon la mide en
el momento en que cada coche cambia a «terminado»; la precisión es de una actualización, unos 0,03 s. Los doblados
muestran `+1 Lap` y los retirados `Out`.

**D54 — Ajustes en dos idiomas.** Swofty muestra un solo texto por ajuste y no tiene traducciones. Por defecto
cada etiqueta y descripción lleva inglés y español (`Rows / Filas`), y `python tools/build.py --lang es` (o `en`)
genera el paquete en un solo idioma. Swofty no acepta textos de más de 255 bytes; `build.py` lo comprueba y la
prueba de build de perfil lo detectó.

## 0.7.1 (captura real del final de carrera, vuelta 78/78)

**D55 — El final de carrera, como en la emisión.** En la emisión, al acabar la carrera la torre no cambia: no hay
banda `FINAL CLASSIFICATION` ni tira a cuadros bajo la cabecera. Sigue `LAP 78 / 78`, `Leader`, las diferencias
con el ganador y la bandera a cuadros en cada coche que termina; los retirados salen atenuados con `Out`. Se quitan
la banda y la tira. Se mantienen las diferencias en meta medidas por el addon (D53).

**D56 — Sanción: cuadrado rojo con «!».** En la emisión, una sanción es un cuadrado rojo (#C8201F) con una «!»
blanca fuera de la fila; el amarillo es una investigación. Sustituye al chip amarillo `+5s`, y se usa para tiempo,
drive-through y stop & go. El amarillo de investigación no se puede hacer porque el juego no expone
investigaciones.

## 0.7.2

**D57 — Fuera el logo de la cadena.** El logo de DAZN arriba a la derecha quedaba raro y solo tiene sentido en
España. Se quitan el ajuste *Broadcaster logo*, el dibujo con el overlay, la imagen del paquete y su descarga en
`fetch_assets.py`. El addon ya no usa la función experimental `overlay`, así que el Addon Browser deja de
mostrarlo con la etiqueta *Draws over the game*; solo usa `telemetry`.

## 0.8.0 (reportes tras 100 descargas en OverTake)

**D58 — Q2/Q3 se cortaba en una posición equivocada.** Causa encontrada: `field_` (el tamaño de la parrilla que
decide el corte de eliminación) se reiniciaba a 0 en cada cambio de `s.type`, y Q1→Q2→Q3 (o SQ1→SQ2→SQ3) cambian de
`type` aunque sean la misma clasificación. Si el estado en vivo da en Q2 un número de clasificados ya reducido
(los 16 que pasaron, no los 22 de salida), `field_` se recalculaba con ese 16 y el corte de Q1 usado para saber
quién sale de la torre en Q2 salía mal: 10 + (16−10)/2 = 13 en vez de 10 + (22−10)/2 = 16. Por eso la tabla se
cortaba en el piloto 13. Arreglo: `field_` ya no se reinicia si el cambio de `type` es dentro de la misma
clasificación (incluida la sprint); sí se reinicia al cambiar de tipo de sesión de verdad (libres→clasificación,
clasificación→carrera). Tests nuevos reproducen el caso con 22→16 (y 20→15 en sprint) y comprueban que no se corta.
Revisión: eso solo arreglaba el caso de ver Q1 entera. Si el addon no ve Q1 (Q1 simulada, o el juego arrancado ya
en Q2), el tamaño de parrilla se vuelve a sacar de Q2 y el corte volvía a salir en 13. Ahora una lista de 17 coches
o menos en Q2 se toma como los que ya están en Q2 (los que pasan de 20, 22 o 24 son 15, 16 o 17), y no se quita a
nadie; con la parrilla entera (20 o más) se quitan los eliminados en Q1 como antes. Q3 ya funcionaba en los dos
casos (queda en 10).

**D59 — Tiempos que no salían en libres y clasificación.** El filtro que evita que el reloj propio ponga a un
coche por delante de otro que el juego clasifica mejor (D5) dejaba el hueco en blanco cuando ningún tiempo del
coche cumplía esa condición, en vez de mostrar igualmente la vuelta medida. Con IAs que entran y salen de boxes
constantemente en libres, esto podía dejar sin tiempo a bastantes coches. Ahora, si el filtro no encuentra ningún
tiempo que respete el orden del juego, se muestra el mejor tiempo real del coche de todas formas: enseñar un
tiempo que puede estar algo descolocado es mejor que no enseñar nada.

**D60 — IN PIT no salía siempre.** El aviso de boxes se comprobaba después de las vistas automáticas (GAINED/LOST,
PIT STOPS, TYRES), así que mientras la torre rotaba por una de esas vistas, un coche que entraba a boxes en ese
momento no lo mostraba hasta que la vista cambiase. Ahora IN PIT se comprueba primero (solo por detrás de
nombres-solamente, abandono y bandera roja) y gana siempre a esas vistas, igual que en la retransmisión.
Revisión: con Safety Car la torre muestra solo nombres y tampoco salía IN PIT, justo cuando más coches paran. Ahora
con SC un coche en boxes muestra `IN PIT` (no con bandera roja, en parrilla ni en la vuelta de formación). Además,
el tiempo de reserva de D59 ya no baja el mínimo que se exige a los coches de detrás.

**D61 — SC ENDING, igual que VSC ENDING.** El Safety Car real solo tenía la fase «entrando en boxes»
(`SAFETY CAR IN THIS LAP`, D8) antes de desactivarse, y al desactivarse pasaba directamente a `TRACK CLEAR` sin una
fase de transición, a diferencia del VSC, que sí la tiene (`VSC ENDING` 4 s y luego `TRACK CLEAR`). Se añade la
misma transición para el SC real: al pasar de activo a desactivado aparece un bloque `SC` / `ENDING` 4 s (mismo
color y escudo FIA que los demás bloques amarillos) y después `TRACK CLEAR`, igual que el VSC.
Revisión: en la tele `SC ENDING` es lo que el ingeniero llama «safety car en esta vuelta», así que sustituye a
`SAFETY CAR IN THIS LAP` (bloque quitado). Ni el juego ni Swofty dan esa señal a los addons (el evento `flag` no
se dispara y `live` solo da 0/1/2); lo primero que se ve es el coche de seguridad metiéndose al pit lane, así que
`SC ENDING` sale desde ahí y sigue 4 s tras el relanzamiento. El VSC no tiene ninguna señal previa: `VSC ENDING`
sigue saliendo los 4 s posteriores a que el juego lo quite.

**D62 — Mi Equipo con 24 coches: la torre sube un poco más.** Con más de 22 filas visibles (23 o 24, solo posible
con Mi Equipo y sus 12 equipos) la torre es más alta que con una parrilla normal de 20-22, porque las filas que se
muestran sí ocupan sitio (las ocultas no). Como el compositor de Swofty no permite que la posición de un elemento
dependa de un valor en tiempo real, salvo con una animación (`motion`) disparada por una hoja booleana, se añade
una hoja `wide` (true con más de 22 filas) y una animación de deslizamiento que sube la torre 24 unidades extra
mientras está activa. La cantidad (24 unidades, algo menos de 1,5 filas) es una estimación prudente: no se ha
podido medir en el juego cuánto margen hay realmente por encima antes de chocar con otro elemento del HUD; si la
torre se corta por arriba o sigue saliéndose por abajo con 24 coches, hay que avisar para ajustar
`extra_up` en `tools/hudgen.py`. Esta animación es del mismo tipo que la de los adelantamientos (D41), cuyo
funcionamiento en el juego aún está por confirmar del todo.

## 0.8.1

**D63 — F8 solo dentro de una sesión.** En una prueba la torre no salía: el log mostraba `tower hidden (hotkey)`
pulsado en el Team HQ de la carrera, donde la torre no se ve, y esa ocultación seguía en todas las sesiones de esa
partida. Ahora la tecla solo oculta o muestra la torre con una sesión en marcha (o la demo), y al empezar una
sesión nueva la torre vuelve a mostrarse aunque se hubiera ocultado en la anterior. Para quitarla siempre está el
ajuste *TV tower* = No.

## 0.8.2

**D64 — Clasificación sin tiempos: paso por meta y diagnóstico.** En el juego, una Q1 no mostró ningún tiempo.
Swofty no da tiempos ni diferencias en libres y clasificación (su doc: el sistema de diferencias del juego no se
alimenta en esas sesiones), y leerlos de la memoria del juego como hace Project Halo va contra D2. El reloj propio
tenía un punto débil: si la distancia de la vuelta vuelve a 0 en la meta unos fotogramas antes de que suba el
contador del juego, ese salto atrás se tomaba como un coche que retrocede (garaje, teletransporte) y anulaba la
vuelta, en todas las vueltas de todos los coches. Ahora ese salto se reconoce como el paso por meta y su momento es
el que cuenta; si el contador sube antes, el paso se calcula con la velocidad. Tests para los dos órdenes.
Como no se ha podido confirmar que esa fuera la causa, cada minuto (si algo cambió) el log escribe
`[tv-tower] laps:` con los coches con tiempo, los pasos por meta y por qué no se cronometró cada uno (primera
vuelta, boxes, vuelta sucia, avisos de límites, fuera de rango), los retrocesos, flashbacks, saltos de meta y saltos
de contador. Sin tiempo, la fila se queda vacía, como en la emisión.

## 0.9.0

**D65 — Decimales clásicos en carrera.** Petición en OverTake: poder ver `+1.200` en lugar de `+1.2`, como en los F1
anteriores. Ajuste *Race gap decimals*: 2026 (por defecto, un decimal truncado) o Clásico (milésimas, el mismo
formato que libres y clasificación, `+1:05.432` pasado el minuto). El resto del aspecto sigue siendo el de 2026: una
segunda torre con el diseño antiguo sería otro registro HUD entero, y no se ha pedido más que los números.

**D66 — Versión EA.** Comentario en OverTake: en la versión EA la torre solo salía en el editor de HUD y en el juego se
veía la del juego. Swofty lee el estado en vivo de la build EA (1789128892) con todos los grupos desde el 11-11
(`live_rows.hpp`, `EA_BUILD`), así que el addon no necesita nada distinto. Lo que sí lo marcaba como «no
compatible» era `compat.game_builds: [">=25251167"]`: es el número de build de Steam, y el launcher enseña «Made for
game build 25251167 (yours: …)» en cualquier otra copia. Se quita (vacío = cualquier build; si el estado en vivo no
está, la torre ya se apaga sola y vuelve la del juego). `platforms` sigue en `steam` hasta que alguien lo pruebe en
EA. Si en EA siguiera sin verse, las líneas `[tv-tower] source:` y `[live]` del `loader.log` dicen por qué.

## 0.10.0

**D67 — Franjas NEW RACE LEADER y FASTEST LAP.** Como en la emisión, arriba en el centro unos segundos.
- **Dónde:** dentro del mismo registro de la torre (pantalla `standings`): el compositor solo pone sprites en el
  tablero de posiciones o en los relojes, y un `scene` no admite sprites. Se colocan con las proporciones medidas en
  la emisión respecto a la torre: empiezan 2,07 anchos de torre a la derecha de su borde, a la altura de su parte de
  arriba, y miden 0,26 anchos de alto. Fuera del grupo de la torre, así no suben con la animación de 24 coches. Si
  se escala o mueve la torre en el editor de HUD, las franjas van con ella.
- **NEW RACE LEADER:** cuando el líder cambia durante 1 s seguido (un reordenamiento o una lectura rota no cuentan),
  desde la vuelta 2, sin formación, bandera roja ni cuadros. El apellido en blanco (un texto no puede tomar el color
  del coche), pegado a la etiqueta por la derecha (el juego no da el ancho de un texto), y el bloque del color del
  equipo con su logo en el sitio de la cara.
- **FASTEST LAP:** cuando una vuelta mejora la vuelta rápida de la carrera (la primera no se anuncia, como en la
  tele). Etiqueta morada con su chevron, logo, apellido y tiempo en morado claro. El nombre de pila no existe en los
  datos del juego que puede leer el HUD (solo apellido y abreviatura). El tiempo sale del reloj de vueltas propio.
- **Nombres:** la franja usa el apellido del propio juego por coche (`session_car`, 30 huecos) como las filas. Los
  huecos van directamente bajo un grupo siempre visible, porque el juego pierde los cambios bajo un nodo oculto, y se
  apagan todos con la franja.
- 6 s cada una, fundido de 0,25 s (`motion`), y si llegan dos a la vez, una detrás de otra; una nueva del mismo tipo
  sustituye a la que espera. Ajustes *New race leader banner* y *Fastest lap banner*. +60 copias en el registro.

## 0.11.0

**D68 — En clasificación y libres el contador de vueltas no se mueve.** La línea `[tv-tower] laps:` de una Q1 real
dio `0 crossings` y `10 line wraps`: los coches cruzaban la meta (la distancia de la vuelta volvía a 0), pero el
contador de vueltas del juego no subía nunca en esa sesión, y el reloj solo cerraba una vuelta con el contador. Por
eso no había tiempos en ninguna clasificación (los arreglos de D59 y D64 no llegaban a actuar). Ahora un paso por
meta es lo primero que llegue de las dos señales: la distancia que vuelve del final de la vuelta al principio, o el
contador que sube. En carrera llegan las dos a pocos fotogramas, en cualquier orden, y la segunda (dentro de 20 s)
se descarta. Tests: clasificación con el contador quieto (vuelta de salida sin tiempo, las siguientes a ±50 ms) y
carrera con las dos señales (cada paso contado una vez).

FASTEST LAP, segunda versión (D67): la primera no se parecía a la emisión. Ahora, como en la captura: más estrecha
(236 × 28), esquinas izquierdas redondeadas, etiqueta morada con su flecha y una estela de tres flechas cada vez más
oscuras, fondo casi negro, apellido en blanco pegado al logo y el tiempo en morado intenso a la derecha.

## 0.12.0

**D69 — Sin tiempos en libres y clasificación.** Con D68 el reloj ya veía los pasos por meta, pero en la Q1 de prueba
el jugador adelantó el tiempo desde el garaje: en 3,5 minutos reales la sesión pasó 8 minutos y las IA hicieron sus
vueltas simuladas, sin cruzar la meta en pista. Medirlas es imposible. Vías comprobadas para los tiempos del juego:
- `swofty::live`: no da tiempos de vuelta.
- `swofty::telemetry`: los tiempos vienen solo de los paquetes UDP (cero en el formato 2026), y en la build Steam el
  core no engancha `sendto` porque todos los grupos se leen de memoria (`telemetry_wants_udp` falso; en el log no
  hay `[udp] sendto hooked`).
- El HUD del juego: por coche, la plantilla solo enlaza `name_code`, `second_name` y `nub_colour`.
- Leer la memoria como Project Halo (`lap_s` de las entradas de resultados) va contra D2.
Decisión del usuario: quitar los tiempos de libres y clasificación hasta que Swofty los dé (petición enviada a sus
desarrolladores). `Config::timed_times` (apagado) deja la lógica entera (P1 su vuelta, diferencias a P1, al corte,
destellos), con sus tests, para encenderla con los tiempos oficiales. Sin datos, los nombres salen completos, como en
la tele. El reloj de vueltas sigue para la vuelta rápida de carrera.

## 0.13.0

**D70 — Iconos de la tele solo en la temporada 2026.** Comentario en OverTake: «en temporadas antiguas, un equipo
sale con el logo de Kick». Los iconos de la emisión del paquete son los de los equipos de 2026 y se eligen por la
clave del escudo del juego. En otras temporadas, Sauber/Alfa Romeo se llama `alfa_romeo` (sin `_26`) y caía en el
icono de Kick, y lo mismo podía pasar con Toro Rosso/AlphaTauri o Renault. Ahora un escudo solo lleva icono de la
tele si su clave es de 2026 (acaba en `26`: `ferrari_26`, `alfa_romeo_26`); cualquier otro usa el escudo del propio
juego, que siempre es el de esa temporada (y sigue a los mods de logos).

**D71 — 24 coches con bandera: el bloque encima del logo.** Mismo comentario: con 24 coches, al salir el Safety Car o
una amarilla, la torre se salía por abajo (el bloque añade 44-54 unidades bajo la cabecera). Con más de 22 filas, el
bloque se dibuja sobre la parte del logo de la cabecera (49,5 unidades), que se oculta, y la torre no cambia de
altura; no depende de la animación de D62. Es una segunda copia de los bloques con sus propias hojas (`t_*`) y una
hoja `cardtop`, así ninguna cambia bajo un nodo oculto. Con 22 coches o menos sigue como en la tele, bajo la cabecera.

## 0.13.1

**D72 — Escudos del juego más grandes.** En una temporada 2015 (escudos del juego, D70) los logos alargados (Force
India, Lotus, Manor) se veían diminutos: el `team_logo` del juego es una textura cuadrada y un logo ancho ocupa solo
una franja. El cuadrado pasa de 13 a 19 px de la referencia (un 46 % más) y se centra en el hueco entre la caja de
posición (19) y el nombre (42), sin tocar ninguno. Los iconos de la tele (2026) no cambian.

## 0.13.2

**D73 — Todas las torres iguales (se quita D71).** La variante de 24 coches (bloque de bandera sobre el logo de la
cabecera) tenía un fallo: la hoja `cardtop` se encendía con 24 coches aunque no hubiera bandera, y la cabecera perdía
el logo F1 y el lockup toda la carrera. Además, el usuario prefiere que la torre de 24 se vea exactamente igual que
las demás. Se quita entera: el bloque va siempre bajo la cabecera, como en la tele. El compositor solo deja deslizar
o fundir un grupo, no escalarlo, así que con 24 coches y bandera la torre puede no caber: se escala en el editor de
HUD. Se queda el arreglo de los textos: una amarilla en tres sectores dice `ALL SECTORS` (`SECTORS 1, 2 & 3` se salía
del bloque) y las líneas de los bloques llevan `clampwidth`.

## 0.14.0

**D74 — Franjas: siempre, más grandes, centradas y en lugar del aviso del juego.** En el juego salían «solo de vez en
cuando», no estaban centradas y eran más cortas que las de la emisión.
- **Siempre:** llevaban un fundido con `motion`, cuyo disparador sobre una hoja propia Swofty avisa que no se ha
  visto funcionar en el juego; si se perdía el evento de entrada, la franja se quedaba con opacidad 0. Fuera el
  fundido: aparecen y desaparecen de golpe. FASTEST LAP sale con cada vuelta rápida de la carrera, también la primera,
  como el aviso del juego.
- **Tamaño:** como la emisión, 2,9 anchos de torre: FASTEST LAP 320 × 31, NEW RACE LEADER 296 × 31, textos más grandes
  y logos propios (`tmb_*`, 1,6 veces los de las filas; el escudo del juego 1,4 veces).
- **Centro:** las dos se centran en `ban_cx` (364 unidades a la derecha del borde de la torre, medido en la emisión).
  El centro real depende de dónde pone el juego el tablero; se ajusta con una captura de pantalla completa.
- **En lugar del aviso del juego:** `stock_nodes` envuelve (`wrap`) la pieza `centre` de `message_system.idf` bajo
  `hud_notifications_dynamic…hud_message` y la oculta mientras sale nuestra FASTEST LAP (`hide` no vale: el juego ya
  la controla). Esa notificación central sirve para más avisos, así que esos 6 s se oculta cualquiera.

## 0.14.1

**D75 — Franjas: medidas en una captura completa, capas y una vuelta rápida que se parezca a la del juego.**
- **Sitio y tamaño:** en una captura de 2000 × 1125 con el HUD en su sitio por defecto, la torre empieza a 41 px y mide
  207 px (1,88 px por unidad): el centro de la pantalla queda a 510 unidades de su borde (estaba en 364, de ahí que
  saliera desplazada) y el aviso central del juego empieza 27 unidades por encima de su parte de arriba. Las franjas
  van ahí, y miden el 37 % del ancho de la pantalla como en la emisión (FASTEST LAP 393 × 38, NEW RACE LEADER 364 × 38).
  Si se mueve o escala la torre en el editor de HUD, las franjas la siguen.
- **Capas:** el fondo estaba en la capa 40 y la unidad `team_logo` del juego siempre dibuja en la 30: el escudo quedaba
  debajo, y el texto «FASTEST LAP» tampoco salía. Fondo en 20-21 y logos en 27, como en las filas.
- **Vuelta rápida:** el juego no deja leer la suya. Una carrera mostró 0:59.0 (imposible) y otra marcaba ERI mientras el
  juego avisaba de HAMILTON 1:28.157, así que la nuestra nunca saltaba y salía el aviso del juego. Dos filtros: ninguna
  vuelta a más de 75 m/s de media (270 km/h; Monza ronda los 260) y la vuelta rápida de la sesión necesita 8 vueltas
  medidas y no puede quedar más de un 4 % por debajo de su mediana.
