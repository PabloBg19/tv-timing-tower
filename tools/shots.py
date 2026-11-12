#!/usr/bin/env python3
"""Screenshots of every demo scenario from the HTML preview (headless Edge), tiled into build/shots.png.
Needs the preview served on 127.0.0.1:8765 (python -m http.server 8765 --directory preview)."""
import subprocess, sys
from pathlib import Path
from PIL import Image
ROOT = Path(__file__).resolve().parent.parent
EDGE = r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"
shots = [("race", 7, "all"), ("race", 12, "all"), ("quali_q1", 6, "all"), ("quali_q2", 2, "all"),
         ("safety_car", 9, "compact"), ("vsc", 6, "top10"), ("red_flag", 2, "all"), ("finish", 6, "all"),
         ("practice", 2, "codes"), ("sprint_quali", 10, "all")]
out = ROOT / "build" / "shots"
out.mkdir(parents=True, exist_ok=True)
tiles = []
for sc, t, rows in shots:
    f = out / f"{sc}_{t}_{rows}.png"
    url = f"http://127.0.0.1:8765/index.html?sc={sc}&t={t}&rows={rows}&scale=1.45"
    subprocess.run([EDGE, "--headless=new", "--disable-gpu", "--hide-scrollbars", f"--screenshot={f}",
                    "--window-size=1320,860", "--virtual-time-budget=3000", url], capture_output=True)
    tiles.append(Image.open(f).crop((40, 90, 470, 860)))
W = 430
sheet = Image.new("RGB", (W * 5, 770 * 2), (0, 0, 0))
for i, im in enumerate(tiles):
    sheet.paste(im, ((i % 5) * W, (i // 5) * 770))
sheet.save(ROOT / "build" / "shots.png")
print("build/shots.png")
