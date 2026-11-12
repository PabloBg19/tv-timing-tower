#!/usr/bin/env python3
"""TV Timing Tower - build everything and package both flavours into dist/.

    python tools/build.py            # art, record, DLL, tests, compose check, preview data, packages
    python tools/build.py test       # host tests only
    python tools/build.py --no-compose

Needs: Python 3.10+ with Pillow, Zig 0.16 (.tools/ or PATH), and for the compose check Swofty Launcher's
swofty.exe (found automatically in %LOCALAPPDATA%\\Programs\\SwoftyLauncher\\bin).
Outputs:
    dist/tv-timing-tower-PRIVATE/            the addon folder (real logos when assets/private has them)
    dist/tv-timing-tower-PRIVATE.swoftyaddon drop this on Swofty's Addons page
    dist/tv-timing-tower-PUBLIC/  and  .swoftyaddon   without any real logo, font or broadcaster mark
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
VERSION = "0.2.0"
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
     "Apellido completo en mayúsculas (como en TV) o abreviatura de tres letras.",
     "Full surname in capitals (as on TV) or the three-letter code.",
     {"values": [("surname", "Apellido", "Surname"), ("code", "Abreviatura", "Code")], "default": "surname"}),
    ("gap_mode", "choice", "Diferencias en carrera", "Race gaps",
     "Automático: intervalos y, en momentos puntuales, gap al líder (como en TV).",
     "Auto: intervals, with the gap to the leader at times (as on TV).",
     {"values": [("auto", "Automático", "Auto"), ("interval", "Intervalo", "Interval"),
                 ("leader", "Al líder", "To leader")], "default": "auto"}),
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
    ("logo_colour", "toggle", "Logos de equipo en color", "Team logos in colour",
     "Los escudos de la retransmisión del propio juego, en color (y los de cualquier mod de logos). No: los blancos del HUD.",
     "The game's own broadcast badges, in colour (and those of any logo mod). Off: the white HUD ones.",
     {"default": True}),
    ("team_bar", "toggle", "Barra de color de equipo", "Team colour bar",
     "Una fina barra con el color del equipo junto al nombre.", "A thin team colour bar beside the name.",
     {"default": False}),
    ("highlight", "toggle", "Resaltar mi coche", "Highlight my car", "Tu fila un poco más clara.",
     "Your row slightly brighter.", {"default": True}),
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
     "Datos simulados (carrera, qualy con eliminados, SC, VSC, bandera roja, final) en cualquier sesión, incluso Contrarreloj.",
     "Simulated data (race, qualifying knockouts, SC, VSC, red flag, finish) in any session, Time Trial too.",
     {"default": False}),
    ("demo_scenario", "choice", "Escenario demo", "Demo scenario", "Qué escenario muestra el modo demo.",
     "Which scenario demo mode shows.",
     {"values": [("cycle", "Todos en bucle", "Cycle all"), ("race", "Carrera", "Race"),
                 ("quali_q1", "Q1 (eliminación)", "Q1 (knockout)"), ("quali_q2", "Q2 (eliminados)", "Q2 (knocked out)"),
                 ("safety_car", "Safety Car", "Safety car"), ("vsc", "VSC", "VSC"),
                 ("red_flag", "Bandera roja", "Red flag"), ("finish", "Bandera a cuadros", "Chequered flag"),
                 ("practice", "Libres", "Practice"), ("sprint_quali", "Clasificación sprint", "Sprint qualifying")],
      "default": "cycle"}),
    ("broadcaster_logo", "toggle", "Logo de la cadena", "Broadcaster logo",
     "Logo de la cadena arriba a la derecha mientras se ve la torre.",
     "The broadcaster's logo in the top right while the tower shows.", {"default": True, "private": True}),
    ("debug_log", "toggle", "Log detallado", "Verbose log",
     "Escribe en swofty/loader.log el coste por fotograma y los cambios de estado.",
     "Writes the frame cost and state changes to swofty/loader.log.", {"default": False}),
]


def manifest(flavour):
    es = flavour == "private"
    opts = []
    for oid, typ, les, len_, des, den, extra in OPTIONS:
        if extra.get("private") and flavour != "private":
            continue
        o = {"id": oid, "type": typ, "label": les if es else len_, "description": des if es else den,
             "default": extra["default"], "binds": "native"}
        if typ == "choice":
            o["values"] = [{"value": v, "label": a if es else b} for v, a, b in extra["values"]]
        opts.append(o)
    files = []
    if flavour == "private":
        files.append("overlay")
    desc_es = ("Torre de tiempos estilo retransmisión F1 TV/DAZN dibujada con el propio HUD del juego, en lugar del "
               "tablero de posiciones. Solo offline: lee el estado en vivo desde la memoria del juego (sin UDP), "
               "sin conexiones de red ni nada relacionado con el online o el antitrampas.")
    desc_es += " Versión PRIVATE (logos reales, uso personal)."
    desc_en = ("Broadcast-style F1 TV timing tower drawn with the game's own HUD in place of the standings board. "
               "Offline only: reads the live state from the game's memory (no UDP), makes no network connections "
               "and touches nothing online or anti-cheat related.")
    return {
        "schema_version": 1,
        "id": ADDON_ID,
        "name": "TV Timing Tower",
        "version": VERSION,
        "author": "Pablo Belascoain",
        "description": desc_es if es else desc_en,
        "license": "MIT",
        "icon": "icon.png",
        "category": "utility",
        "kind": ["ui", "native"],
        "experimental_features": ["telemetry", "overlay"],
        "compat": {"game": "f1_25", "game_builds": [">=25251167"], "launcher": ">=1.9.0",
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
    from PIL import Image, ImageDraw
    n = 256
    img = Image.new("RGBA", (n, n), (12, 14, 30, 255))
    d = ImageDraw.Draw(img)
    d.rectangle([40, 30, 120, 226], fill=(20, 24, 48, 255))
    for i in range(8):
        y = 48 + i * 22
        d.rectangle([46, y, 62, y + 16], fill=(225, 6, 0, 255) if i == 0 else (40, 46, 80, 255))
        d.rectangle([68, y + 5, 112 - (i % 3) * 8, y + 11], fill=(235, 235, 240, 255))
    d.rectangle([130, 48, 214, 70], fill=(255, 209, 46, 255))
    d.rectangle([130, 82, 214, 104], fill=(225, 6, 0, 255))
    d.rectangle([130, 116, 214, 138], fill=(177, 56, 221, 255))
    img.save(path)


def build_flavour(flavour, dll_path, logos):
    name = f"tv-timing-tower-{flavour.upper()}"
    stage = BUILD / flavour / "addon"
    if stage.exists():
        shutil.rmtree(stage)
    stage.mkdir(parents=True)
    # record and sprites
    run([sys.executable, str(ROOT / "tools" / "hudgen.py"), str(stage), "--flavour", flavour, "--logos", str(logos)])
    layout = json.loads((stage / "layout.json").read_text())
    (stage / "native").mkdir(exist_ok=True)
    shutil.copy2(dll_path, stage / "native" / DLL)
    if flavour == "private":
        (stage / "overlay").mkdir(exist_ok=True)
        bc = logos / "broadcaster.png"
        if bc.exists():
            shutil.copy2(bc, stage / "overlay" / "broadcaster.png")
        else:
            (stage / "overlay" / "README.txt").write_text("Put broadcaster.png here (white logo, transparent).\n")
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
        if (logos / "broadcaster.png").exists():
            shutil.copy2(logos / "broadcaster.png", data / "broadcaster.png")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("what", nargs="?", default="all", choices=["all", "test"])
    ap.add_argument("--no-compose", action="store_true")
    a = ap.parse_args()
    BUILD.mkdir(exist_ok=True)
    logos = ROOT / "assets" / "private" / "logos"

    if a.what == "test":
        for fl in ("public", "private"):
            build_flavour(fl, BUILD / DLL if (BUILD / DLL).exists() else ROOT / "LICENSE", logos)
        tests()
        return

    dll = BUILD / DLL
    cxx(dll, ROOT / "native" / "src" / "addon.cpp", shared=True)
    run([sys.executable, str(ROOT / "tools" / "pe_imports.py"), str(dll)])
    staged = [build_flavour(fl, dll, logos) for fl in ("private", "public")]
    tests()
    for name, stage, layout in staged:
        if not a.no_compose:
            compose_check(stage, "private" if "PRIVATE" in name else "public")
        package(name, stage)
    preview_assets(staged[0][1], staged[0][2])
    print("done")


if __name__ == "__main__":
    main()
