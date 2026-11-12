# Checklist de pruebas en el juego — TV Timing Tower 0.4.0

Antes de empezar: instala `dist/tv-timing-tower-PRIVATE.swoftyaddon`, actívalo, en ADDON SETTINGS pon
**Log detallado = Sí**, y juega. Tras cada sesión mira `%LOCALAPPDATA%\Swofty\sessions\<última>\game-logs\loader.log`
y busca `[tv-tower]`. Anota en cada casilla OK / FALLO + qué se vio.

## 0. Arranque y seguridad
- [ ] El juego arranca y llega al menú sin cuelgues.
- [ ] Log: `[tv-tower] TV Timing Tower 0.4.0 loaded: … 12 broadcast team icons, logos 'tv'`. … leaves, colour (broadcast) team logos, hotkey F8`.
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
- [ ] F8 oculta la torre (vuelve la del juego) y F8 la muestra otra vez.
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
- [ ] Penalización de 5 s: chip amarillo `+5s`. Drive-through: `DT`.
- [ ] Vuelta rápida: tiempo en morado 5 s en la fila y luego el icono morado de cronómetro fijo.
- [ ] Abandono: `Out` y fila atenuada. Descalificación: `DSQ`.
- [ ] Bandera a cuadros: cabecera a cuadros e icono a cuadros en cada coche que termina.

## 5. Estados de carrera (usa SC/VSC en "Frecuente" o provoca un accidente)
- [ ] Bandera amarilla: bloque amarillo con escudo FIA, `YELLOW FLAG` / `SECTOR 2`.
- [ ] VSC: bloque `VIRTUAL / SAFETY CAR` con escudo FIA; siguen las abreviaturas y los intervalos. Al terminar,
      `VSC / ENDING` 4 s y luego `TRACK CLEAR` verde 4 s.
- [ ] Safety Car: bloque `SAFETY / CAR`; la torre pasa a apellidos completos, sin diferencias ni neumáticos. Cuando
      el coche de seguridad entra al pit lane, `SAFETY CAR / IN THIS LAP`; al reanudar, `TRACK CLEAR`.
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
- [ ] Logo de la cadena arriba a la derecha mientras se ve la torre; *Logo de la cadena* = No lo quita.
- [ ] Con la build PUBLIC: logo de serie del juego en la cabecera y sin logo de cadena.

## 8. Fallo seguro
- [ ] Desactiva el addon: vuelve el tablero nativo intacto.
- [ ] Contrarreloj: se ve el tablero de tiempos del juego; log `source: Time Trial`.
- [ ] (Si hay una actualización del juego) log `source: no live state …` y torre del juego, sin cuelgues.
