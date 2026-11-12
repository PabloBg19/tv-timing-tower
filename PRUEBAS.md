# Checklist de pruebas en el juego — TV Timing Tower 0.1.0

Antes de empezar: instala `dist/tv-timing-tower-PRIVATE.swoftyaddon`, actívalo, en ADDON SETTINGS pon
**Log detallado = Sí**, y juega. Tras cada sesión mira `%LOCALAPPDATA%\Swofty\sessions\<última>\game-logs\loader.log`
y busca `[tv-tower]`. Anota en cada casilla OK / FALLO + qué se vio.

## 0. Arranque y seguridad
- [ ] El juego arranca y llega al menú sin cuelgues.
- [ ] Log: `[tv-tower] TV Timing Tower 0.1.0 loaded: … leaves, 12 private team logos, hotkey F8`.
- [ ] Log: `[live] on (Steam build): groups … classification, timing, pit, penalties, tyres, flags … badges`.
- [ ] Log sin `[hud] … refused` ni `invalid` con `hud_tvt_`.
- [ ] ADDON SETTINGS muestra la ficha **TV Timing Tower** con todos los ajustes en español.
- [ ] Editor de HUD (Ajustes → HUD → Personalizar): el elemento de posiciones muestra la vista previa de 5 filas
      (`RUSSELL 1:19.832`, `VERSTAPPEN` con P…) y se puede mover, escalar y cambiar la opacidad.

## 1. Libres (Gran Premio → Libres 1)
- [ ] Al salir a pista desaparece el tablero nativo y aparece la torre a la izquierda (cabecera F1 + `P1 mm:ss`).
- [ ] El reloj de la cabecera cuenta hacia atrás (es el del propio juego).
- [ ] Nombres en mayúsculas correctos (los del juego) y logos de equipo correctos para cada piloto.
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
- [ ] Q2: `Q2`, zona de eliminación 11-16; si el juego sigue clasificando a los eliminados de Q1, salen atenuados.
- [ ] Q3: `Q3`, sin zona roja; P11+ atenuados si aparecen.
- [ ] Coche en out-lap / garaje: `P` blanca cuando está en boxes.
- [ ] Al final de cada ronda la torre no se queda en blanco ni muestra filas raras.

## 3. Clasificación sprint (SQ1, SQ2, SQ3)
- [ ] Igual que el punto 2 con `SQ1/SQ2/SQ3`.

## 4. Carrera (y sprint)
- [ ] Cabecera `LAP X/Y` (vuelta del líder / total del juego).
- [ ] Líder: `INTERVAL` en gris; resto `+0.9`, `+12.3` (un decimal).
- [ ] Cada vuelta múltiplo de 5 del líder: 8 s con `LEADER` y gaps al líder (modo Automático).
- [ ] Doblados: `+1 LAP`.
- [ ] Parada en boxes: `PIT` mientras está en el pit lane; cambia el icono de neumático al salir.
- [ ] Vueltas del neumático (ajuste) se reinician tras la parada.
- [ ] Adelantamiento: flecha verde/roja 3 s en los dos coches implicados.
- [ ] Penalización de 5 s: chip amarillo `+5s`. Drive-through: `DT`.
- [ ] Vuelta rápida: tiempo en morado 5 s en la fila y luego el icono morado de cronómetro fijo.
- [ ] Abandono: `DNF`/`OUT` y fila atenuada. Descalificación: `DSQ`.
- [ ] Bandera a cuadros: cabecera a cuadros e icono a cuadros en cada coche que termina.

## 5. Estados de carrera (usa SC/VSC en "Frecuente" o provoca un accidente)
- [ ] Bandera amarilla: tarjeta amarilla `YELLOW FLAG S2` bajo la cabecera, las filas bajan.
- [ ] VSC: tarjeta `VIRTUAL SAFETY CAR`; al terminar `VSC ENDING` 4 s y luego `TRACK CLEAR` verde 4 s.
- [ ] Safety Car: `SAFETY CAR`; cuando el coche de seguridad entra al pit lane `SAFETY CAR IN THIS LAP`;
      al reanudar `TRACK CLEAR`.
- [ ] Bandera roja: tarjeta roja `RED FLAG`, sin diferencias en la columna derecha.
- [ ] La caja de banderas nativa de la izquierda no aparece mientras se ve la torre (la torre la sustituye).

## 6. Modo demo
- [ ] En una sesión de libres, **Ctrl+F8**: aparecen los nombres de la parrilla 2026 simulada y los escenarios
      cambian cada 14 s (carrera → Q1 → Q2 → SC → VSC → bandera roja → final → libres → SQ2).
- [ ] **Mayús+F8** pasa de escenario; **Ctrl+F8** sale de la demo y vuelven los datos reales.
- [ ] Ajuste *Modo demo* = Sí + *Escenario demo* fijo: se queda en ese escenario.

## 7. Logos (solo PRIVATE)
- [ ] Logos reales de los 11 equipos 2026 en las filas; F1 en la cabecera.
- [ ] Logo de la cadena arriba a la derecha mientras se ve la torre; *Logo de la cadena* = No lo quita.
- [ ] Con la build PUBLIC: logos del propio juego y sin logo de cadena.

## 8. Fallo seguro
- [ ] Desactiva el addon: vuelve el tablero nativo intacto.
- [ ] Contrarreloj: se ve el tablero de tiempos del juego; log `source: Time Trial`.
- [ ] (Si hay una actualización del juego) log `source: no live state …` y torre del juego, sin cuelgues.
