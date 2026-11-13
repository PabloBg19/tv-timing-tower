# Checklist de pruebas en el juego — TV Timing Tower 0.14.1

Antes de empezar: instala `dist/tv-timing-tower.swoftyaddon`, actívalo, en ADDON SETTINGS pon
**Log detallado = Sí**, y juega. Tras cada sesión mira `%LOCALAPPDATA%\Swofty\sessions\<última>\game-logs\loader.log`
y busca `[tv-tower]`. Anota en cada casilla OK / FALLO + qué se vio.

## 0. Arranque y seguridad
- [ ] El juego arranca y llega al menú sin cuelgues.
- [ ] Log: `[tv-tower] TV Timing Tower 0.14.1 loaded: … 12 broadcast team icons, logos 'tv'`. … leaves, colour (broadcast) team logos, hotkey F8`.
- [ ] Log: `[live] on (Steam build): groups … classification, timing, pit, penalties, tyres, flags … badges`.
- [ ] Log sin `[hud] … refused` ni `invalid` con `hud_tvt_`.
- [ ] ADDON SETTINGS muestra la ficha **TV Timing Tower** con todos los ajustes en español.
- [ ] Editor de HUD (Ajustes → HUD → Personalizar): el elemento de posiciones muestra la vista previa de 5 filas
      (`RUSSELL 1:19.832`, `VERSTAPPEN` con P…) y se puede mover, escalar y cambiar la opacidad.

## 1. Libres (Gran Premio → Libres 1)
- [ ] Al salir a pista desaparece el tablero nativo y aparece la torre arriba a la izquierda, **entera** (las 22
      filas en pantalla), con cabecera F1 + `P1 mm:ss`.
- [ ] El reloj de la cabecera **sale y** cuenta hacia atrás (es el del propio juego).
- [ ] Nombres en mayúsculas correctos (los del juego), **nunca dos apellidos en una fila** (prueba también tras volver
      del garaje y tras salir de la demo con Ctrl+F8).
- [ ] Logos de equipo como en la TV: Ferrari, Red Bull, McLaren, Alpine y Haas en color; Mercedes, Aston,
      Williams, Racing Bulls y Cadillac en blanco; Audi con los aros rojos. *Logos de equipo* = Juego: los escudos
      del propio juego; Juego blanco: los blancos del HUD.
- [ ] Compara una captura del juego con una de la TV a la misma altura de fila: ancho de la torre (≈6,3 filas),
      columnas, cabecera (logo F1, lockup FIA, banda LAP) y bloques de bandera con el logo FIA.
- [ ] Neumático: solo la letra en su color (S roja, M amarilla, H blanca), sin círculo.
- [ ] Apellidos largos (VERSTAPPEN, HULKENBERG, ANTONELLI) no pisan el tiempo.
- [ ] Coches en garaje/boxes muestran la `P` blanca a la derecha.
- [ ] Tras completar vueltas: el líder muestra su tiempo (`1:2x.xxx`), el resto `+0.xxx`.
- [ ] Tu mejor vuelta parpadea en verde (o morado si es la mejor de la sesión) 3 s.
- [ ] Compara tu tiempo con el del juego (pantalla de tiempos / pausa): diferencia ≤ 0,05 s. Anótala.
- [ ] Una vuelta con aviso de límites de pista NO cuenta como mejor tiempo.
- [ ] Pausa 20 s en mitad de una vuelta: el tiempo de esa vuelta no incluye la pausa.
- [ ] Flashback: tras usarlo, la vuelta en curso no da un tiempo absurdo.
- [ ] F8 oculta la torre (vuelve la del juego) y F8 la muestra otra vez. F8 en los menús no hace nada, y la
      sesión siguiente la vuelve a mostrar (log `tower shown again (new session)`).
- [ ] Ajustes en caliente: Filas = Top 10 + yo / Compacta; Nombres = Abreviatura; Fondo = Sólido / Translúcido;
      Neumáticos = No. Cada cambio se ve sin reiniciar.
- [ ] FPS: con y sin la torre (F8) no hay diferencia apreciable. Log detallado: `frame cost … us mean` < 300 µs.

## 2. Clasificación (Q1, Q2, Q3)
- [ ] Cabecera `Q1 mm:ss`.
- [ ] Q1: posiciones 17-22 (con 22 coches) en caja roja oscura y línea roja encima de la 17.
- [ ] Q2: `Q2`, zona de eliminación 11-16; los eliminados de Q1 **no aparecen** (16 filas).
- [ ] Q3: `Q3`, sin zona roja, solo 10 filas.
- [ ] Coche en out-lap / garaje: `P` blanca cuando está en boxes.
- [ ] Al final de cada ronda la torre no se queda en blanco ni muestra filas raras.

## 3. Clasificación sprint (SQ1, SQ2, SQ3)
- [ ] Igual que el punto 2 con `SQ1/SQ2/SQ3`.

## 4. Carrera (y sprint)
- [ ] Cabecera `LAP X / Y` (vuelta del líder / total del juego).
- [ ] Vuelta 1: apellidos completos, sin diferencias ni neumáticos.
- [ ] Al empezar la vuelta 2 (si has visto la vuelta de formación o la salida): barra `GAINED / LOST` durante 10 s,
      con ▲/▼ y los puestos; compárala con la parrilla.
- [ ] Después: abreviaturas, el líder con `Interval` en gris y el resto con `+0.9`, `+12.3` (un decimal) y la letra
      del neumático.
- [ ] En cada vuelta múltiplo de 5 del líder: 8 s con `Leader` y gaps al líder (modo Automático).
- [ ] Doblados: `+1 Lap`.
- [ ] Parada en boxes: `PIT` mientras está en el pit lane; cambia el icono de neumático al salir.
- [ ] Vueltas del neumático (ajuste) se reinician tras la parada.
- [ ] Adelantamiento: flecha verde/roja 3 s en los dos coches implicados.
- [ ] Penalización (tiempo, drive-through, stop & go): cuadrado rojo con «!».
- [ ] Vuelta rápida: tiempo en morado 5 s en la fila y luego el icono morado de cronómetro fijo.
- [ ] Abandono: `Out` y fila atenuada. Descalificación: `DSQ`.
- [ ] Bandera a cuadros: cabecera a cuadros e icono a cuadros en cada coche que termina.

## 4n. Novedades 0.14.1
- [ ] Las franjas centradas donde sale el aviso del juego (arriba en el centro) y del ancho de la foto de la tele.
- [ ] FASTEST LAP con su texto morado «FASTEST LAP» y el logo del equipo (antes faltaban).
- [ ] Carrera: el icono morado de vuelta rápida de la torre y la franja coinciden con el aviso del juego
      («Mejor vuelta …»). Si no coinciden, captura de las dos cosas.

## 4m. Novedades 0.14.0 (franjas)
- [ ] Demo (Ctrl+F8, *Escenario demo* = Avisos): `NEW RACE LEADER` y luego `FASTEST LAP`, más largas y grandes, **en el
      centro** arriba. Manda una captura de pantalla completa (sin recortar) para centrarlas al píxel.
- [ ] Carrera: salen **siempre** que cambia el líder (desde la vuelta 2) y con cada vuelta rápida, también la primera.
- [ ] Mientras sale nuestra FASTEST LAP no sale la notificación central del juego (su aviso de vuelta rápida).

## 4l. Novedades 0.13.2
- [ ] Mi Equipo con 24 coches: la cabecera con el logo F1 y el lockup en toda la carrera (la 0.13.0 los quitaba).
- [ ] Con 24 coches y bandera, el bloque debajo de la cabecera, igual que con 22.
- [ ] Amarilla en varios sectores: `SECTORS 1 & 3` o `ALL SECTORS`, sin salirse del bloque.

## 4k. Novedades 0.13.1
- [ ] Temporada antigua (o *Team logos* = Game): los escudos del juego se ven más grandes (los alargados, como Force
      India, Lotus o Manor, se leen) y no tocan el número de posición ni el apellido.

## 4j. Novedades 0.13.0 (comentarios de OverTake)
- [ ] Una temporada que no sea la 2026 (la base de F1 25 u otra): cada equipo con el escudo del juego, ninguno con el
      icono de Kick que no le toca.

## 4i. Novedades 0.12.0
- [ ] Libres y clasificación: sin tiempos ni diferencias, apellidos completos, `P` de boxes, zona de eliminación y
      corte (Q1 hasta P16, Q2 16 filas, Q3 10).

## 4h. Novedades 0.11.0 (tiempos de qualy: quitados en la 0.12.0)
- [ ] Clasificación y libres: tras la vuelta de salida y una vuelta lanzada, P1 muestra su tiempo y los demás `+0.xxx`.
      El log (`[tv-tower] laps:`) debe dar `crossings` y `timed` mayores que 0.
- [ ] Compara un tiempo con el del juego (pausa → tiempos): diferencia de unas centésimas como mucho.
- [ ] FASTEST LAP con el diseño nuevo (estela de flechas, fondo casi negro, tiempo morado a la derecha).

## 4g. Novedades 0.10.0 (franjas)
- [ ] Demo (Ctrl+F8, o *Escenario demo* = Avisos): `VERSTAPPEN NEW RACE LEADER` con el bloque azul y el logo a la
      derecha, y luego `FASTEST LAP · LECLERC · 1:17.938`, arriba en el centro. Captura de pantalla completa: ¿quedan
      centradas y a la altura de la torre?
- [ ] Carrera real: al cambiar el líder (desde la vuelta 2) sale `NEW RACE LEADER` 6 s con su apellido, el color y el
      logo de su equipo. No sale en la salida ni con bandera roja.
- [ ] Carrera real: al mejorar alguien la vuelta rápida sale `FASTEST LAP` con su apellido, logo y tiempo (la primera
      vuelta rápida de la carrera no la anuncia, igual que la emisión).
- [ ] Las franjas aparecen y desaparecen con un fundido (si no, salen de golpe: también vale).
- [ ] ADDON SETTINGS: *New race leader banner* y *Fastest lap banner* = No las quitan.

## 4f. Novedades 0.9.0
- [ ] ADDON SETTINGS → *Race gap decimals* = Clásico: en carrera los intervalos salen con milésimas (`+0.934`),
      sin pisar la abreviatura. 2026 vuelve a `+0.9`.
- [ ] (Alguien con la versión EA) la torre sale en el juego y no solo en el editor de HUD.

## 4e. Novedades 0.8.0 (arreglos reportados)
- [ ] Libres y clasificación: salen tiempos en casi todos los coches (antes se quedaban en blanco con frecuencia).
      Cada minuto el log escribe `[tv-tower] laps: …` con cuántos coches tienen tiempo y por qué no se cronometró
      cada paso por meta; manda esas líneas si siguen sin salir.
- [ ] Clasificación con 22 coches: Q2 muestra **16 filas** (no se corta en el 13), Q3 muestra 10. Prueba también
      simulando la Q1 y entrando directamente en Q2.
- [ ] Clasificación sprint con 20 coches: SQ2 muestra 15 filas.
- [ ] Carrera: un coche que entra a boxes muestra `IN PIT` **siempre**, también si la torre está en ese momento en
      `GAINED / LOST`, `PIT STOPS` o `TYRES`, y con Safety Car (aunque la torre muestre solo los nombres).
- [ ] Safety Car real: cuando el coche de seguridad entra al pit lane aparece `SC` / `ENDING` (con el escudo FIA),
      sigue 4 s tras el relanzamiento y luego `TRACK CLEAR`. Anota cuánto antes del relanzamiento sale.
- [ ] Mi Equipo con 24 coches, filas = Todas: la torre sube un poco más que con una parrilla normal. Si se corta
      por arriba (tapa otro elemento del HUD) o sigue sin caber por abajo, dilo con una captura.

## 4d. Novedades 0.7.0
- [ ] Parrilla y vuelta de formación: banda `STARTING TYRES`, apellidos y letra del neumático; desaparece al apagarse
      los semáforos.
- [ ] Vuelta 7 (y 17, 27…): banda `TYRES` 10 s, con las vueltas de cada juego de neumáticos.
- [ ] Al acabar: la torre igual que en la emisión, con `Leader`, diferencias en meta (`+6.2`, `+1 Lap`), bandera a
      cuadros en cada coche que termina y `Out` atenuado. Sin banda ni tira bajo la cabecera.
- [ ] Un coche con sanción: cuadrado rojo con «!» a la derecha de su fila.
- [ ] ADDON SETTINGS: cada ajuste en `English / Español`.

## 4c. Novedades 0.6.0
- [ ] Libres: aparecen tiempos (P1 su vuelta, los demás `+0.xxx` respecto a P1) aunque las IA entren y salgan del
      garaje.
- [ ] Clasificación: P1 con su tiempo y los demás con la diferencia con P1; en la zona de eliminación, la diferencia
      con el último que pasa (P16 en Q1).
- [ ] Carrera: `PIT STOPS` en la vuelta 10 (si alguien ha parado). `FORMATION LAP` en la vuelta de formación.
- [ ] Adelantamientos: fundido solo de la parte del coche; en la salida, sin fundidos.
- [ ] `Interval` no pisa la abreviatura. `LAP 1 / 5` bien separado.
- [ ] Mi Equipo (24 coches): 24 filas, tu equipo con una ficha de su color en lugar del logo, y los nombres bien.
- [ ] FPS: el informe de rendimiento (`perf.log`) da a `pablo.tv-timing-tower` muy por debajo de 11 ms/s.

## 4b. Novedades 0.5.0
- [ ] En cada fila, el nombre, el número, el logo y el neumático están a la misma altura.
- [ ] Clasificación y libres: el tiempo restante de la sesión aparece junto a `Q1` / `P1` y cuenta hacia atrás.
- [ ] Carrera: intervalos desde la vuelta 1. Un coche en boxes: `IN PIT` en verde menta, sin neumático.
- [ ] Adelantamiento: las dos filas se desvanecen y vuelven con el orden nuevo, con los triángulos verde y rojo.
      Si las filas desaparecen y no vuelven, pon *Animaciones* = No y avísame.

## 5. Estados de carrera (usa SC/VSC en "Frecuente" o provoca un accidente)
- [ ] Bandera amarilla: bloque amarillo con escudo FIA, `YELLOW FLAG` / `SECTOR 2`.
- [ ] VSC: bloque `VIRTUAL / SAFETY CAR` con escudo FIA; siguen las abreviaturas y los intervalos. Al terminar,
      `VSC / ENDING` 4 s y luego `TRACK CLEAR` verde 4 s.
- [ ] Safety Car: bloque `SAFETY / CAR`; la torre pasa a apellidos completos, sin diferencias ni neumáticos. Cuando
      el coche de seguridad entra al pit lane, `SC / ENDING`, que sigue 4 s tras el relanzamiento; luego
      `TRACK CLEAR` verde 4 s.
- [ ] Bandera roja: bloque rojo `RED FLAG`, solo nombres.
- [ ] El escudo FIA del bloque se ve (es una textura del juego; si sale vacío o raro, haz una captura).
- [ ] Compara con la TV: si FOM lo muestra distinto, manda una captura de la emisión.
- [ ] La caja de banderas nativa de la izquierda no aparece mientras se ve la torre (la torre la sustituye).

## 6. Modo demo
- [ ] En una sesión de libres, **Ctrl+F8**: aparecen los nombres de la parrilla 2026 simulada y los escenarios
      cambian cada 14 s (carrera → Q1 → Q2 → SC → VSC → bandera roja → final → libres → SQ2).
- [ ] **Mayús+F8** pasa de escenario; **Ctrl+F8** sale de la demo y vuelven los datos reales.
- [ ] Ajuste *Modo demo* = Sí + *Escenario demo* fijo: se queda en ese escenario.

## 7. Logos (solo PRIVATE)
- [ ] Logo F1 real en la cabecera (los de equipo son los del juego en las dos builds).

## 8. Fallo seguro
- [ ] Desactiva el addon: vuelve el tablero nativo intacto.
- [ ] Contrarreloj: se ve el tablero de tiempos del juego; log `source: Time Trial`.
- [ ] (Si hay una actualización del juego) log `source: no live state …` y torre del juego, sin cuelgues.
