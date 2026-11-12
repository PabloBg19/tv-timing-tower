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

**D9 — Logos.** PRIVATE: PNG reales (descargados a `assets/private/logos`, fuera de git) como sprites del HUD,
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
