#!/usr/bin/env python3
"""Downloads the real logos for the PRIVATE build into assets/private/logos (git-ignored, personal use only).

    python tools/fetch_assets.py [--force]

Sources (public, official where possible):
  - team logos: Formula 1's own media CDN, the white versions the F1 website shows (2026 set, 2025 for Sauber)
  - F1 logo:    Wikimedia Commons "F1.svg" rendered PNG, recoloured white as on the broadcast header
  - DAZN logo:  Wikimedia Commons, recoloured white for the corner bug
Every image is converted to a white-on-transparent PNG. Nothing here goes into git or the PUBLIC build.
"""
import argparse
import io
import json
import sys
import urllib.parse
import urllib.request
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "assets" / "private" / "logos"
UA = {"User-Agent": "tv-timing-tower-asset-fetch/0.1 (personal build script)"}

F1CDN = "https://media.formula1.com/image/upload/c_fit,h_512/q_auto/v1740000000/common/f1/{y}/{s}/{y}{s}logowhite.webp"
TEAMS = {  # our name -> (year, f1.com slug)
    "mercedes": (2026, "mercedes"),
    "ferrari": (2026, "ferrari"),
    "redbull": (2026, "redbullracing"),
    "mclaren": (2026, "mclaren"),
    "astonmartin": (2026, "astonmartin"),
    "alpine": (2026, "alpine"),
    "williams": (2026, "williams"),
    "racingbulls": (2026, "racingbulls"),
    "haas": (2026, "haasf1team"),
    "sauber": (2025, "kicksauber"),
    "audi": (2026, "audi"),
    "cadillac": (2026, "cadillac"),
}
# The broadcast tower's team icons (2026 world feed, as in the reference captures): the published colour icon,
# the white one, or the white one tinted (Audi's rings are red on the tower).
F1CDN_VARIANT = "https://media.formula1.com/image/upload/c_fit,h_512/q_auto/v1740000000/common/f1/{y}/{s}/{y}{s}{v}.webp"
TV_TEAMS = {  # our name -> (year, slug, variant, tint)
    "mercedes": (2026, "mercedes", "logowhite", None),
    "ferrari": (2026, "ferrari", "logo", None),
    "redbull": (2026, "redbullracing", "logo", None),
    "mclaren": (2026, "mclaren", "logo", None),
    "astonmartin": (2026, "astonmartin", "logowhite", None),
    "alpine": (2026, "alpine", "logo", None),
    "williams": (2026, "williams", "logowhite", None),
    "racingbulls": (2026, "racingbulls", "logowhite", None),
    "haas": (2026, "haasf1team", "logo", None),
    "sauber": (2025, "kicksauber", "logo", None),
    "audi": (2026, "audi", "logowhite", (214, 32, 42)),
    "cadillac": (2026, "cadillac", "logowhite", None),
}
COMMONS = {"f1": ["File:F1.svg", "File:F1 logo.svg"],
           "fia_black": ["File:FIA logo (black).png"],
           "fia_white": ["File:FIA logo (white).png"]}


def get(url):
    req = urllib.request.Request(url, headers=UA)
    with urllib.request.urlopen(req, timeout=30) as r:
        return r.read()


def commons_png(title, width=1024):
    q = urllib.parse.urlencode({"action": "query", "titles": title, "prop": "imageinfo", "iiprop": "url",
                                "iiurlwidth": width, "format": "json"})
    data = json.loads(get("https://commons.wikimedia.org/w/api.php?" + q))
    for page in data["query"]["pages"].values():
        info = page.get("imageinfo")
        if info:
            return get(info[0].get("thumburl") or info[0]["url"]), info[0].get("descriptionurl", "")
    return None, ""


def whiten(img, keep_dark_as_transparent=False):
    img = img.convert("RGBA")
    px = img.load()
    w, h = img.size
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if keep_dark_as_transparent:
                lum = (r * 299 + g * 587 + b * 114) // 1000
                a = min(a, lum)
            px[x, y] = (255, 255, 255, a)
    bb = img.getbbox()
    return img.crop(bb) if bb else img


def is_mono(img):
    """True when the opaque pixels are one colour (a flat logo): safe to recolour white."""
    img = img.convert("RGBA")
    small = img.resize((96, max(1, int(96 * img.size[1] / img.size[0]))))
    counts = {}
    total = 0
    raw = small.tobytes()
    for i in range(0, len(raw), 4):
        r, g, b, a = raw[i], raw[i + 1], raw[i + 2], raw[i + 3]
        if a < 200:
            continue
        key = (r // 48, g // 48, b // 48)
        counts[key] = counts.get(key, 0) + 1
        total += 1
    return total == 0 or max(counts.values()) / total > 0.9


def prepare(img, mode):
    """white: always recolour; auto: recolour flat logos, keep multi-colour badges; keep: as published."""
    img = img.convert("RGBA")
    if mode == "white" or (mode == "auto" and is_mono(img)):
        return whiten(img)
    bb = img.getbbox()
    return img.crop(bb) if bb else img


def tint(img, rgb):
    img = img.convert("RGBA")
    r, g, b = rgb
    out = Image.new("RGBA", img.size, (r, g, b, 0))
    out.putalpha(img.getchannel("A"))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--force", action="store_true")
    a = ap.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    sources = []
    for name, (year, slug) in TEAMS.items():
        dst = OUT / f"{name}.png"
        if dst.exists() and not a.force:
            continue
        url = F1CDN.format(y=year, s=slug)
        try:
            img = Image.open(io.BytesIO(get(url)))
            prepare(img, "auto").save(dst)
            sources.append(f"{name}: {url}")
            print(f"ok   {name}")
        except Exception as e:  # keep going: a missing logo falls back to the game's own badge
            print(f"miss {name}: {e}")
    for name, (year, slug, variant, colour) in TV_TEAMS.items():
        dst = OUT / f"tv_{name}.png"
        if dst.exists() and not a.force:
            continue
        url = F1CDN_VARIANT.format(y=year, s=slug, v=variant)
        try:
            img = Image.open(io.BytesIO(get(url))).convert("RGBA")
            bb = img.getbbox()
            img = img.crop(bb) if bb else img
            if colour:
                img = tint(img, colour)
            img.save(dst)
            sources.append(f"tv_{name}: {url}")
            print(f"ok   tv_{name}")
        except Exception as e:
            print(f"miss tv_{name}: {e}")
    for name, titles in COMMONS.items():
        dst = OUT / f"{name}.png"
        if dst.exists() and not a.force:
            continue
        for t in titles:
            try:
                raw, desc = commons_png(t)
            except Exception as e:
                print(f"miss {name} {t}: {e}")
                continue
            if not raw:
                continue
            img = Image.open(io.BytesIO(raw))
            prepare(img, "white" if name == "f1" else "keep").save(dst)  # FIA logos: as published
            sources.append(f"{name}: {desc or t}")
            print(f"ok   {name} ({t})")
            break
        else:
            print(f"miss {name}")
    if sources:
        with open(OUT / "SOURCES.txt", "a", encoding="utf-8") as f:
            f.write("\n".join(sources) + "\n")
    print(f"logos in {OUT}")


if __name__ == "__main__":
    sys.exit(main())
