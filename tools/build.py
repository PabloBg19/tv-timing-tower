#!/usr/bin/env python3
"""TV Timing Tower - build everything and package the addon into dist/.

    python tools/build.py            # art, record, DLL, tests, compose check, preview data, packages
    python tools/build.py test       # host tests only
    python tools/build.py --no-compose

Needs: Python 3.10+ with Pillow, Zig 0.16 (.tools/ or PATH), and for the compose check Swofty Launcher's
swofty.exe (found automatically in %LOCALAPPDATA%\\Programs\\SwoftyLauncher\\bin).
Outputs:
    dist/tv-timing-tower/              the addon folder (team icons, F1 and FIA logos from assets/private)
    dist/tv-timing-tower.swoftyaddon   drop this on Swofty's Addons page
"""
import argparse
import json
import os
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build"
DIST = ROOT / "dist"
VERSION = "0.25.0"
LANG = "both"  # settings labels: both, en, es (--lang)
ADDON_ID = "pablo.tv-timing-tower"
DLL = "tv_timing_tower.dll"

sys.path.insert(0, str(ROOT / "tools"))
import hudgen  # noqa: E402


def zig():
    for c in sorted((ROOT / ".tools").glob("zig-*/zig.exe")) + [Path(p) / "zig.exe" for p in
                                                                os.environ.get("PATH", "").split(os.pathsep)]:
        if c.exists():
            return str(c)
    sys.exit("zig not found: put Zig 0.16 in .tools/ (see README) or on PATH")


def swofty_exe():
    c = Path(os.environ.get("LOCALAPPDATA", "")) / "Programs" / "SwoftyLauncher" / "bin" / "swofty.exe"
    if c.exists():
        return str(c)
    return shutil.which("swofty")


def run(cmd, **kw):
    print("+", " ".join(str(c) for c in cmd))
    r = subprocess.run(cmd, **kw)
    if r.returncode:
        sys.exit(f"failed ({r.returncode}): {cmd[0]}")
    return r


def zig_env():
    env = dict(os.environ)
    cache = str(BUILD / "zig-cache")
    env["ZIG_GLOBAL_CACHE_DIR"] = cache
    env["ZIG_LOCAL_CACHE_DIR"] = cache
    return env


def cxx(out, src, shared=False, opt="-O2"):
    cmd = [zig(), "c++", "-target", "x86_64-windows-gnu", "-std=c++23", opt, "-Wall", "-Wextra",
           "-Wno-nullability-completeness", "-I", str(ROOT / "native" / "include"),
           f'-DTVT_VERSION="{VERSION}"']
    if shared:
        cmd += ["-shared", "-s"]
    cmd += ["-o", str(out), str(src)]
    r = subprocess.run(cmd, env=zig_env(), capture_output=True, text=True)
    errors = [l for l in (r.stdout + r.stderr).splitlines() if "error" in l.lower() and "libcxx" not in l]
    if r.returncode:
        print("\n".join(errors) or (r.stdout + r.stderr)[-4000:])
        sys.exit(f"compile failed: {src}")
    for f in Path(out).parent.glob(Path(out).stem + ".lib"):
        f.unlink()
    for f in Path(out).parent.glob(Path(out).stem + ".pdb"):
        f.unlink()
    print(f"built {out}")


# ------------------------------------------------------------------------------------------------- manifest
OPTIONS = [
    # id, type, es label, en label, es desc, en desc, extra
    ("enabled", "toggle", "Torre de TV", "TV tower",
     "Sustituye la torre de tiempos del juego por la de la retransmisión.",
     "Replaces the game's standings board with the broadcast tower.", {"default": True}),
    ("rows", "choice", "Filas", "Rows",
     "Todas las posiciones, el top 10 más tu coche, o compacta (P1-P3 y los coches a tu alrededor).",
     "Every position, the top 10 plus your car, or compact (P1-P3 and the cars around you).",
     {"values": [("all", "Todas", "All"), ("top10", "Top 10 + yo", "Top 10 + me"),
                 ("compact", "Compacta", "Compact")], "default": "all"}),
    ("names", "choice", "Nombres", "Names",
     "Auto: abreviatura con datos al lado, apellido sin datos (parrilla, Safety Car).",
     "Auto: the code beside data, the full surname without (grid, safety car).",
     {"values": [("auto", "Automático", "Auto"), ("surname", "Apellido", "Surname"), ("code", "Abreviatura", "Code")],
      "default": "auto"}),
    ("gap_mode", "choice", "Diferencias en carrera", "Race gaps",
     "Como en TV: intervalos; al líder 5/15/25; neumáticos 7/17; paradas 10/20; ganadas/perdidas en la 2.",
     "As on TV: intervals; leader gap laps 5/15/25; tyres 7/17; stops 10/20; gained/lost on lap 2.",
     {"values": [("auto", "Automático", "Auto"), ("interval", "Intervalo", "Interval"),
                 ("leader", "Al líder", "To leader"), ("gained", "Ganadas/perdidas", "Gained/lost"),
                 ("pitstops", "Paradas", "Pit stops"), ("tyres", "Neumáticos", "Tyres")],
      "default": "auto"}),
    ("gap_decimals", "choice", "Decimales en carrera", "Race gap decimals",
     "2026: un decimal, como en la emisión actual (+0.9). Clásico: milésimas, como en los F1 anteriores (+0.934).",
     "2026: one decimal, as on today's broadcast (+0.9). Classic: thousandths, as in earlier F1 games (+0.934).",
     {"values": [("one", "2026 (+0.9)", "2026 (+0.9)"), ("three", "Clásico (+0.934)", "Classic (+0.934)")],
      "default": "one"}),
    ("ban_leader", "toggle", "Aviso de nuevo líder", "New race leader banner",
     "Carrera: franja arriba en el centro con NEW RACE LEADER cuando cambia el líder (desde la vuelta 2).",
     "Races: a banner top centre with NEW RACE LEADER when the lead changes hands (from lap 2).", {"default": True}),
    ("ban_fastest", "toggle", "Aviso de vuelta rápida", "Fastest lap banner",
     "Carrera: franja arriba en el centro con FASTEST LAP, el piloto y su tiempo (los del juego), y el icono morado en su fila.",
     "Races: a banner top centre with FASTEST LAP, the driver and the time (the game's own), and the purple icon on the row.",
     {"default": True}),
    ("champ_formation", "toggle", "Campeonato en la vuelta de formación", "Championship on the formation lap",
     "Modo carrera: tras los neumáticos de salida, la clasificación de pilotos con sus puntos (los del juego, sprints incluidas).",
     "Career: after the starting tyres, the drivers' standings with their points (the game's own, sprints included).",
     {"default": True}),
    ("tyres", "toggle", "Neumáticos", "Tyres", "Muestra el compuesto actual de cada coche.",
     "Shows each car's current compound.", {"default": True}),
    ("tyre_age", "toggle", "Vueltas del neumático", "Tyre age",
     "Vueltas con el juego actual, contadas desde la última parada vista.",
     "Laps on the current set, counted from the last stop seen.", {"default": False}),
    ("background", "choice", "Fondo", "Background",
     "Opacidad del fondo de la torre. La opacidad general, la escala y la posición se cambian en el editor de HUD del juego.",
     "Tower background opacity. Overall opacity, size and position are set in the game's HUD editor.",
     {"values": [("solid", "Sólido", "Solid"), ("broadcast", "TV", "Broadcast"),
                 ("light", "Translúcido", "Light")], "default": "broadcast"}),
    ("arrows", "toggle", "Flechas de posición", "Position arrows",
     "Flecha verde o roja durante 3 s cuando un coche gana o pierde posiciones.",
     "A green or red arrow for 3 s when a car gains or loses places.", {"default": True}),
    ("logos", "choice", "Logos de equipo", "Team logos",
     "TV: iconos de la emisión 2026. Juego: escudos del juego en color (siguen a los mods). Juego blanco: los del HUD.",
     "TV: the 2026 broadcast icons. Game: the game's colour badges (follow logo mods). Game white: HUD badges.",
     {"values": [("tv", "TV", "TV"), ("game", "Juego", "Game"), ("game_white", "Juego blanco", "Game white")],
      "default": "game", "public_values": ["game", "game_white"], "public_default": "game"}),
    ("animations", "toggle", "Animaciones", "Animations",
     "Al cambiar el coche de una fila (adelantamientos), la fila se desvanece y vuelve con el nuevo coche.",
     "When a row's car changes (overtakes), the row fades out and back in with the new car.", {"default": True}),
    ("highlight", "toggle", "Resaltar mi coche", "Highlight my car", "Tu fila un poco más clara.",
     "Your row slightly brighter.", {"default": True}),
    ("quali_card", "toggle", "Tarjeta de clasificación", "Qualifying lap card",
     "Clasificación: tu vuelta como en la TV, al lado de la torre: tiempo, parciales, diferencia con P1 o el corte.",
     "Qualifying: your lap as on TV, beside the tower: time, sectors, gap to P1 or the cut.", {"default": True}),
    ("in_practice", "toggle", "En libres", "In practice", "Torre en los entrenamientos libres.",
     "Tower in practice.", {"default": True}),
    ("in_quali", "toggle", "En clasificación", "In qualifying", "Torre en Q1, Q2 y Q3.",
     "Tower in Q1, Q2 and Q3.", {"default": True}),
    ("in_sprint_quali", "toggle", "En clasificación sprint", "In sprint qualifying", "Torre en SQ1, SQ2 y SQ3.",
     "Tower in SQ1, SQ2 and SQ3.", {"default": True}),
    ("in_race", "toggle", "En carrera y sprint", "In races", "Torre en la carrera y en la sprint.",
     "Tower in the race and the sprint.", {"default": True}),
    ("hotkey", "choice", "Tecla mostrar/ocultar", "Show/hide key",
     "Muestra u oculta la torre (vuelve la del juego). Ctrl+tecla: modo demo. Mayús+tecla: siguiente escenario demo.",
     "Shows or hides the tower (the game's board comes back). Ctrl+key: demo mode. Shift+key: next demo scenario.",
     {"values": [(f"F{i}", f"F{i}", f"F{i}") for i in range(1, 13)] + [("off", "Ninguna", "None")],
      "default": "F8"}),
    ("demo", "toggle", "Modo demo", "Demo mode",
     "Datos simulados (carrera, qualy con eliminados, SC, VSC, bandera roja, final, parrilla) en una sesión de libres.",
     "Simulated data (race, qualifying knockouts, SC, VSC, red flag, finish, grid) in a practice session.",
     {"default": False}),
    ("demo_scenario", "choice", "Escenario demo", "Demo scenario", "Qué escenario muestra el modo demo.",
     "Which scenario demo mode shows.",
     {"values": [("cycle", "Todos en bucle", "Cycle all"), ("race", "Carrera", "Race"),
                 ("quali_q1", "Q1 (eliminación)", "Q1 (knockout)"), ("quali_q2", "Q2 (eliminados)", "Q2 (knocked out)"),
                 ("safety_car", "Safety Car", "Safety car"), ("vsc", "VSC", "VSC"),
                 ("red_flag", "Bandera roja", "Red flag"), ("finish", "Bandera a cuadros", "Chequered flag"),
                 ("practice", "Libres", "Practice"), ("sprint_quali", "Clasificación sprint", "Sprint qualifying"),
                 ("gained", "Ganadas/perdidas", "Gained/lost"), ("grid", "Parrilla (neumáticos de salida)", "Grid (starting tyres)"),
                 ("tyres", "Vista de neumáticos", "Tyres view"),
                 ("banners", "Avisos (líder y vuelta rápida)", "Banners (leader, fastest lap)"),
                 ("championship", "Campeonato (vuelta de formación)", "Championship (formation lap)")],
      "default": "cycle"}),
    ("debug_log", "toggle", "Log detallado", "Verbose log",
     "Escribe en swofty/loader.log el coste de cada actualización y los cambios de estado.",
     "Writes the update cost and state changes to swofty/loader.log.", {"default": False}),
]


def manifest(flavour):
    # Swofty shows one text per setting (no translations): both languages by default, "English / Español"
    def pick(en, es):
        if LANG == "en" or en == es:
            return en
        if LANG == "es":
            return es
        return f"{en} / {es}"
    opts = []
    for oid, typ, les, len_, des, den, extra in OPTIONS:
        if extra.get("private") and flavour != "private":
            continue
        o = {"id": oid, "type": typ, "label": pick(len_, les), "description": pick(den, des),
             "default": extra["default"], "binds": "native"}
        if typ == "choice":
            o["values"] = [{"value": v, "label": pick(b, a)} for v, a, b in extra["values"]]
        for key in ("label", "description"):
            if len(o[key].encode()) > 255:  # Swofty refuses longer strings
                sys.exit(f"setting {oid}: {key} is {len(o[key].encode())} bytes (Swofty's limit is 255)")
        opts.append(o)
    files = ["native/tvt_logos.txt"]
    # one line (Swofty's manifest: "a one-line summary"); the Addon Browser's long text is set on the portal
    desc_es = "La torre de tiempos de la TV de F1 2026, dibujada con el propio HUD del juego. Solo offline."
    desc_en = "The 2026 F1 broadcast timing tower, drawn by the game's own HUD. Offline only."
    return {
        "schema_version": 1,
        "id": ADDON_ID,
        "name": "TV Timing Tower",
        "version": VERSION,
        "author": "Pablo Belascoain",
        "description": pick(desc_en, desc_es),
        "license": "MIT",
        "repository": "https://github.com/PabloBg19/tv-timing-tower",  # needs launcher 1.8.19+ (compat: 1.9.0)
        "icon": "icon.png",
        "category": "utility",
        "kind": ["ui", "native"],
        "experimental_features": ["telemetry"],
        "compat": {"game": "f1_25", "launcher": ">=1.9.0",
                   "seasons": ["f1_2026"], "platforms": ["steam"]},
        "requires": [{"id": "swofty.core", "version": ">=0.1.0"}],
        "options": opts,
        "operations": {
            "hud": ["registry/tower.json"],
            "native": [dict({"dll": f"native/{DLL}"}, **({"files": files} if files else {}))],
        },
    }


# ---------------------------------------------------------------------------------------------------- steps
def make_icon(path):
    """The Addon Browser's icon (a square PNG, shown as it is): the tower itself, drawn at 4x and scaled down. Only
    our own drawing and a system font: no F1, FIA or team marks."""
    from PIL import Image, ImageDraw, ImageFont
    n, k = 512, 4
    N = n * k
    img = Image.new("RGBA", (N, N))
    d = ImageDraw.Draw(img)
    for y in range(N):  # a dark track-side background
        t = y / N
        d.line([(0, y), (N, y)], fill=(int(28 + 18 * t), int(34 + 22 * t), int(30 + 10 * t), 255))
    fonts = Path(os.environ.get("WINDIR", "C:/Windows")) / "Fonts"

    def font(px, bold=True):
        for name in (("bahnschrift.ttf", "segoeuib.ttf", "arialbd.ttf") if bold else ("bahnschrift.ttf", "segoeui.ttf", "arial.ttf")):
            if (fonts / name).exists():
                f = ImageFont.truetype(str(fonts / name), px * k)
                if name == "bahnschrift.ttf":
                    try:
                        f.set_variation_by_name("Bold" if bold else "Regular")
                    except Exception:
                        pass
                return f
        return ImageFont.load_default()

    x0, x1, top = 64, 448, 40
    S = lambda *v: [a * k for a in v]
    # header: slate block, LAP 23 / 57
    d.rounded_rectangle(S(x0, top, x1, top + 92), radius=14 * k, fill=(64, 63, 77, 250), corners=(True, True, False, False))
    d.rectangle(S(x0, top + 92, x1, top + 96), fill=(80, 80, 94, 255))
    d.text(S(n / 2, top + 48), "LAP 23 / 57", font=font(46), fill=(240, 240, 244, 255), anchor="mm")
    rows = [("1", "RUS", "#00D7B6", "Interval"), ("2", "VER", "#4781D7", "+0.8"), ("3", "LEC", "#ED1131", "+1.4"),
            ("4", "NOR", "#F47600", "+0.3"), ("5", "HAM", "#ED1131", "IN PIT"), ("6", "PIA", "#F47600", "+2.1")]
    rh, y = 60, top + 96
    for i, (pos, code, col, gap) in enumerate(rows):
        last = i == len(rows) - 1
        d.rounded_rectangle(S(x0, y, x1, y + rh), radius=14 * k, fill=(38, 37, 44, 245),
                            corners=(False, False, last, last))
        d.rectangle(S(x0, y + 3, x0 + 62, y + rh - 3), fill=(225, 6, 0, 255) if i == 0 else (22, 21, 30, 255))
        d.text(S(x0 + 31, y + rh / 2), pos, font=font(36, False), fill=(255, 255, 255, 255), anchor="mm")
        d.rounded_rectangle(S(x0 + 76, y + 18, x0 + 100, y + rh - 18), radius=4 * k, fill=col)
        d.text(S(x0 + 116, y + rh / 2), code, font=font(38), fill=(255, 255, 255, 255), anchor="lm")
        gc = (94, 214, 180, 255) if gap == "IN PIT" else (189, 186, 196, 255) if gap == "Interval" else (242, 242, 244, 255)
        d.text(S(x1 - 18, y + rh / 2), gap, font=font(32 if gap[0].isalpha() else 36, gap == "IN PIT"), fill=gc,
               anchor="rm")
        y += rh
    img.resize((n, n), Image.LANCZOS).save(path)


def build_flavour(flavour, dll_path, logos):
    name = "tv-timing-tower"
    stage = BUILD / flavour / "addon"
    if stage.exists():
        shutil.rmtree(stage)
    stage.mkdir(parents=True)
    # record and sprites
    run([sys.executable, str(ROOT / "tools" / "hudgen.py"), str(stage), "--flavour", flavour, "--logos", str(logos)])
    # the game's UI walk budget (the HUD customisation screen holds the record six times): stop here if it would close
    run([sys.executable, str(ROOT / "tools" / "walkcost.py"), str(stage / "registry" / "tower.json")])
    # element ids are djb2 hashes of their paths: two that clash make the composer refuse the scene
    run([sys.executable, str(ROOT / "tools" / "idcheck.py"), str(stage / "registry" / "tower.json")])
    layout = json.loads((stage / "layout.json").read_text())
    (stage / "native").mkdir(exist_ok=True)
    shutil.copy2(dll_path, stage / "native" / DLL)
    found = [hudgen.TEAMS.index(t) for t in layout.get("logos", [])]
    (stage / "native" / "tvt_logos.txt").write_text(" ".join(str(i) for i in found) + "\n")
    make_icon(stage / "icon.png")
    (stage / "swofty.json").write_text(json.dumps(manifest(flavour), indent=2, ensure_ascii=False) + "\n",
                                       encoding="utf-8")
    shutil.copy2(ROOT / "LICENSE", stage / "LICENSE")
    shutil.copy2(ROOT / "README.md", stage / "README.md")
    return name, stage, layout


def compose_check(stage, flavour):
    exe = swofty_exe()
    if not exe:
        print("compose check skipped: swofty.exe not found")
        return
    out = BUILD / flavour / "compose"
    if out.exists():
        shutil.rmtree(out)
    aid = manifest(flavour)["id"]
    run([exe, "hud", "compose", str(stage), "registry/tower.json", "--id", aid, "--out", str(out)])


def profile_check(archive, flavour):
    """Install the packaged addon in a scratch Swofty data dir, enable it and run a full profile build, as Play
    does. `hud compose` alone is not enough: 0.4.0 composed but its profile build stopped (os error 3). Your own
    Swofty data dir and the game folder are not touched."""
    exe = swofty_exe()
    if not exe:
        print("profile check skipped: swofty.exe not found")
        return
    if game_running():
        # a build reads (and a real one rewrites) the game's UI archives: with the game open they are already
        # overlaid, and rewriting them under a running game crashed it once
        print("profile check skipped: F1 25 is running (close it and build again)")
        return
    data = BUILD / flavour / "swofty-data"
    if data.exists():
        shutil.rmtree(data)
    data.mkdir(parents=True)
    aid = manifest(flavour)["id"]
    base = [exe, "--data-dir", str(data)]
    run(base + ["addons", "install", str(archive)], capture_output=True)
    run(base + ["addons", "enable", aid], capture_output=True)
    r = subprocess.run(base + ["build"], capture_output=True, text=True)
    if r.returncode:
        # the same build of an empty profile tells a launcher problem (1.9.41: "the tooltips already have mod_menu"
        # in any scratch data dir) from one of this package
        empty = BUILD / flavour / "swofty-empty"
        if empty.exists():
            shutil.rmtree(empty)
        empty.mkdir(parents=True)
        e = subprocess.run([exe, "--data-dir", str(empty), "build"], capture_output=True, text=True)
        last = lambda out: (out.strip().splitlines() or [""])[-1]
        if e.returncode and last(e.stdout + e.stderr) == last(r.stdout + r.stderr):
            print(f"profile check inconclusive: an empty profile fails the same way ({last(e.stdout + e.stderr)})")
            return
        print((r.stdout + r.stderr)[-3000:])
        sys.exit(f"profile build failed with the {flavour} package")
    print(f"profile check: {flavour} package installs and builds")


def game_running():
    if os.name != "nt":
        return False
    r = subprocess.run(["tasklist", "/FI", "IMAGENAME eq F1_25.exe", "/NH"], capture_output=True, text=True)
    return "F1_25.exe" in r.stdout


def package(name, stage):
    DIST.mkdir(exist_ok=True)
    folder = DIST / name
    if folder.exists():
        shutil.rmtree(folder)
    shutil.copytree(stage, folder, ignore=shutil.ignore_patterns("layout.json"))
    archive = DIST / f"{name}.swoftyaddon"
    if archive.exists():
        archive.unlink()
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as z:
        for f in sorted(folder.rglob("*")):
            if f.is_file():
                z.write(f, f.relative_to(folder).as_posix())
    print(f"packaged {archive} ({archive.stat().st_size // 1024} KB)")


def tests():
    out = BUILD / "test"
    out.mkdir(parents=True, exist_ok=True)
    exe = out / "tower_test.exe"
    cxx(exe, ROOT / "native" / "test" / "tower_test.cpp", opt="-O1")
    run([str(exe)], cwd=str(ROOT))
    dump = out / "preview_dump.exe"
    cxx(dump, ROOT / "native" / "test" / "preview_dump.cpp", opt="-O1")
    data = ROOT / "preview" / "data"
    data.mkdir(parents=True, exist_ok=True)
    run([str(dump), str(data / "frames.json")])


def preview_assets(stage, layout):
    data = ROOT / "preview" / "data"
    data.mkdir(parents=True, exist_ok=True)
    if (data / "art").exists():
        shutil.rmtree(data / "art")
    shutil.copytree(stage / "art", data / "art")
    (data / "layout.json").write_text(json.dumps(layout, indent=1))
    (data / "layout.js").write_text("window.TVT_LAYOUT = " + json.dumps(layout) + ";\n")
    frames = data / "frames.json"
    if frames.exists():
        (data / "frames.js").write_text("window.TVT_FRAMES = " + frames.read_text() + ";\n")
    fonts = ROOT / "assets" / "private" / "fonts"
    if fonts.exists():
        dst = data / "fonts"
        if dst.exists():
            shutil.rmtree(dst)
        shutil.copytree(fonts, dst)
    logos = ROOT / "assets" / "private" / "logos"
    if logos.exists():
        dst = data / "logos"
        if dst.exists():
            shutil.rmtree(dst)
        dst.mkdir()
        for t in hudgen.TEAMS:
            if (logos / f"{t}.png").exists():
                shutil.copy2(logos / f"{t}.png", dst / f"{t}.png")



def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("what", nargs="?", default="all", choices=["all", "test"])
    ap.add_argument("--no-compose", action="store_true")
    ap.add_argument("--lang", choices=["both", "en", "es"], default="both",
                    help="language of the settings' labels (Swofty has no translations): both, en or es")
    a = ap.parse_args()
    global LANG
    LANG = a.lang
    BUILD.mkdir(exist_ok=True)
    logos = ROOT / "assets" / "private" / "logos"

    if a.what == "test":
        build_flavour("private", BUILD / DLL if (BUILD / DLL).exists() else ROOT / "LICENSE", logos)
        tests()
        return

    dll = BUILD / DLL
    cxx(dll, ROOT / "native" / "src" / "addon.cpp", shared=True)
    run([sys.executable, str(ROOT / "tools" / "pe_imports.py"), str(dll)])
    staged = [build_flavour("private", dll, logos)]
    for old in DIST.glob("tv-timing-tower-*"):  # the PRIVATE / PUBLIC packages before 0.6
        shutil.rmtree(old) if old.is_dir() else old.unlink()
    tests()
    for name, stage, layout in staged:
        if not a.no_compose:
            compose_check(stage, "private")
        package(name, stage)
        if not a.no_compose:
            profile_check(DIST / f"{name}.swoftyaddon", "private")
    preview_assets(staged[0][1], staged[0][2])
    print("done")


if __name__ == "__main__":
    main()
