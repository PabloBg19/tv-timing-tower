#!/usr/bin/env python3
"""TV Timing Tower - writes the hud.element record and its sprites for one flavour.

    python tools/hudgen.py <out_addon_dir> --flavour private|public [--logos assets/private/logos]

Geometry (0.4): measured on captures of the 2026 world feed tower (docs: INVESTIGACION.md section 6). Every number
in REF is in pixels of those captures, where a row is 21.5 px; `u()` scales them to HUD units so that a row is
LAYOUT["row"] units. The same LAYOUT goes to <out>/layout.json for the HTML preview, so preview and game draw from
one set of numbers.
"""
import argparse
import json
import math
import shutil
from pathlib import Path

from PIL import Image, ImageDraw

P = "view_model_data.hud_tvt_"   # leaf prefix (must match native/src/leaves.hpp)
ROWS = 24          # a career with 12 teams runs 24 cars
SLOTS = 30         # vehicle ids up to 29 (a My Team grid)
TEAMS = ["mercedes", "ferrari", "redbull", "mclaren", "astonmartin", "alpine",
         "williams", "racingbulls", "haas", "sauber", "audi", "cadillac"]
PX = 4  # sprite pixels per HUD unit

ROW_UNITS = 17.5        # one row of the tower in HUD units
REF_ROW = 21.5          # one row in the reference captures, px


def u(px):
    return round(px * ROW_UNITS / REF_ROW, 3)


# ---- measured on the captures (px, row = 21.5; x from the tower's left edge, y from the piece's top)
REF = {
    "width": 135.0,
    "pos_w": 19.0, "pos_x": 9.5,
    "logo_x": 29.0, "logo_w": 15.0, "logo_h": 11.0, "unit_logo": 13.0,
    "name_x": 42.0,
    "gap_right": 117.5,
    "tyre_x": 127.0,
    "base": 15.8,            # text baseline in a row (caps sit a little below the row's middle)
    "cell_inset": 1.75,      # the position cells (and the P1 box) leave a thin line between rows
    "cap_name": 8.0, "cap_pos": 7.6, "cap_gap": 7.6, "cap_label": 7.0, "cap_tyre": 5.8,
    "side_gap": 2.0, "chip": 11.0,
    "gain_x": 99.0, "gain_right": 115.5,
    # header
    "pad": 8.5, "f1_w": 70.0, "f1_h": 17.5,
    "lock_y": 30.0, "lock_h": 10.0, "lock_w": 72.0, "fia_small_w": 14.5,
    "rule_y": 49.5, "rule_h": 1.5, "band_y": 51.0, "band_h": 22.0, "lap_base": 66.0,
    "cap_lapword": 7.5, "cap_lapnum": 9.5,
    "sprint_cap": 11.0, "sprint_base": 41.0,
    "gain_h": 22.0, "gain_base": 15.5, "cap_gainword": 9.5,
    # flag blocks
    # 2 px of the band's colour above the yellow (42 px for two lines, 52 for three)
    "block2": 44.0, "block3": 54.0, "block_gap": 2.0, "fia_x0": 5.5, "fia_w": 44.0, "fia_h2": 28.0, "fia_h3": 33.0,
    "block_text_x": 55.0, "cap_block": 9.8,
    "block_pitch": 13.75, "base2": [22.0, 35.75], "base3": [18.5, 32.25, 46.0],
    "chq_h": 22.0,
}

LAYOUT = {
    "offset_y": -40.0,       # the whole tower, from the stock board's origin (into the hidden lap counter's space)
    "extra_up": 24.0,        # added on top of offset_y while more than 22 rows show (D62, not measured in game)
    "row": ROW_UNITS,
    "width": u(REF["width"]),
    "pos_w": u(REF["pos_w"]), "pos_x": u(REF["pos_x"]),
    "logo_x": u(REF["logo_x"]), "logo_w": u(REF["logo_w"]), "logo_h": u(REF["logo_h"]), "unit_logo": u(REF["unit_logo"]),
    "name_x": u(REF["name_x"]),
    "name_box": u(REF["gap_right"] - REF["name_x"] - 2),
    "gap_right": u(REF["gap_right"]), "gap_box": u(70),
    "tyre_x": u(REF["tyre_x"]),
    "text_y": u(REF["base"]),
    "cap_name": u(REF["cap_name"]), "cap_pos": u(REF["cap_pos"]), "cap_gap": u(REF["cap_gap"]),
    "cap_label": u(REF["cap_label"]), "cap_tyre": u(REF["cap_tyre"]),
    "side_x": u(REF["width"] + REF["side_gap"]), "chip": u(REF["chip"]),
    "gain_x": u(REF["gain_x"]), "gain_right": u(REF["gain_right"]),
    "header": u(REF["band_y"] + REF["band_h"]),
    "f1": [u((REF["width"] - REF["f1_w"]) / 2), u(REF["pad"]), u(REF["f1_w"]), u(REF["f1_h"])],
    "lock": [u((REF["width"] - REF["lock_w"]) / 2), u(REF["lock_y"]), u(REF["lock_w"]), u(REF["lock_h"])],
    "rule_y": u(REF["rule_y"]), "rule_h": u(REF["rule_h"]), "band_y": u(REF["band_y"]), "cell_inset": u(REF["cell_inset"]),
    "block_gap": u(REF["block_gap"]), "fia_h2": u(REF["fia_h2"]), "fia_h3": u(REF["fia_h3"]), "band_h": u(REF["band_h"]), "lap_base": u(REF["lap_base"]),
    "cap_lapword": u(REF["cap_lapword"]), "cap_lapnum": u(REF["cap_lapnum"]),
    "sprint_cap": u(REF["sprint_cap"]), "sprint_base": u(REF["sprint_base"]),
    "gainbar": u(REF["gain_h"]), "gain_base": u(REF["gain_base"]), "cap_gainword": u(REF["cap_gainword"]),
    "block2": u(REF["block2"]), "block3": u(REF["block3"]),
    "fia_box": [u(REF["fia_x0"]), u(REF["fia_w"])],
    "block_text_x": u(REF["block_text_x"]), "cap_block": u(REF["cap_block"]),
    "base2": [u(b) for b in REF["base2"]], "base3": [u(b) for b in REF["base3"]],
    "chq_h": u(REF["chq_h"]),
    "colours": {
        # sampled on the captures (the tower is slightly translucent; these are the colours over a dark scene)
        "hdr_top": [68, 67, 81], "hdr_bottom": [50, 48, 60], "rule": [76, 76, 90],
        "band": [24, 23, 35],
        "row": [38, 37, 44],
        "pos": [22, 21, 30],
        "alpha": [0.97, 0.90, 0.74],
        "p1": "#E0120F",
        "ko": "#8E0B12",
        "purple": "#C35BDC",
        "green": "#2BD15F",
        "gap": "#F2F2F4",
        "label": "#BDBAC4",
        "lapword": "#D4D3DA",
        "inpit": "#5ED6B4",       # mint, as on the 2026 feed
        "sc": "#ECD024",
        "red": "#E10600",
        "clear": "#1E9E3E",
        "arrow_up": "#32BF36",
        "arrow_dn": "#E8102A",
        "tyres": {"S": "#E8303A", "M": "#F0D23A", "H": "#E6E6E6", "I": "#3CB043", "W": "#2E7CD6"},
    },
}

# (0.4.0 drew the public build's FIA badge from the game's ms_fia_logo.tif; Swofty's profile build refused it)


# ------------------------------------------------------------------------------------------------- sprites
def px(units):
    v = int(math.ceil(units * PX / 4.0)) * 4
    return max(4, v)


def hexrgba(h, a=255):
    h = h.lstrip("#")
    return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16), a if len(h) < 8 else int(h[6:8], 16))


def save(img, path):
    assert img.size[0] % 4 == 0 and img.size[1] % 4 == 0, (path, img.size)
    img.save(path)


def flat(w, h, rgba):
    return Image.new("RGBA", (px(w), px(h)), rgba)


def triangle(w, h, up, colour):
    img = flat(w, h, (0, 0, 0, 0))
    n, m = img.size
    big = img.resize((n * 4, m * 4))
    d = ImageDraw.Draw(big)
    N, M = big.size
    pts = [(N / 2, 0), (N, M), (0, M)] if up else [(0, 0), (N, 0), (N / 2, M)]
    d.polygon(pts, fill=hexrgba(colour))
    return big.resize((n, m), Image.LANCZOS)


def chequer(w, h):
    img = Image.new("RGBA", (px(w), px(h)), (255, 255, 255, 255))
    d = ImageDraw.Draw(img)
    q = img.size[1] / 4
    for yy in range(4):
        for xx in range(int(img.size[0] / q) + 1):
            if (xx + yy) % 2:
                d.rectangle([xx * q, yy * q, (xx + 1) * q - 1, (yy + 1) * q - 1], fill=(12, 12, 12, 255))
    return img


def make_art(art: Path):
    art.mkdir(parents=True, exist_ok=True)
    L = LAYOUT
    C = L["colours"]
    W, R, H = L["width"], L["row"], L["header"]
    sprites = {}

    def add(name, img, bounds):
        save(img, art / f"{name}.png")
        sprites[name] = {"png": f"art/{name}.png", "bounds": [round(b, 3) for b in bounds]}

    for i, a in enumerate(C["alpha"]):
        A = int(255 * a)
        # header: a lighter slate block (logo and lockup), a dark rule, the dark LAP band
        img = flat(W, H, (0, 0, 0, 0))
        d = ImageDraw.Draw(img)
        sy = img.size[1] / H
        top_end = int(L["rule_y"] * sy)
        for y in range(top_end):
            t = y / max(1, top_end - 1)
            c = [int(C["hdr_top"][k] + (C["hdr_bottom"][k] - C["hdr_top"][k]) * t) for k in range(3)]
            d.line([(0, y), (img.size[0], y)], fill=(*c, A))
        d.rectangle([0, top_end, img.size[0], int(L["band_y"] * sy)], fill=(*C["rule"], A))
        d.rectangle([0, int(L["band_y"] * sy), img.size[0], img.size[1]], fill=(*C["band"], A))
        add(f"hdr{i}", img, [0, 0, W, H])
        # rows: the darker position column and the row
        img = flat(W, R, (*C["row"], A))
        d = ImageDraw.Draw(img)
        pw = int(L["pos_w"] * img.size[0] / W)
        ins = int(round(L["cell_inset"] * img.size[1] / R))
        d.rectangle([0, ins, pw - 1, img.size[1] - ins - 1], fill=(*C["pos"], A))
        add(f"row{i}", img, [0, 0, W, R])
        # the "GAINED / LOST" line extends the LAP band
        add(f"gainbg{i}", flat(W, L["gainbar"], (*C["band"], A)), [0, 0, W, L["gainbar"]])

    add("me", flat(W - L["pos_w"], R, (255, 255, 255, 26)), [L["pos_w"], 0, W, R])
    ci = L["cell_inset"]
    add("p1", flat(L["pos_w"], R - 2 * ci, hexrgba(C["p1"])), [0, ci, L["pos_w"], R - ci])
    add("ko", flat(L["pos_w"], R - 2 * ci, hexrgba(C["ko"])), [0, ci, L["pos_w"], R - ci])
    add("koline", flat(W, 0.8, hexrgba(C["p1"])), [0, 0, W, 0.8])
    add("sep", flat(W - 8, 0.6, (255, 255, 255, 70)), [4, 0, W - 4, 0.6])
    cw, chh = u(12.0), u(9.0)
    img = flat(cw, chh, (0, 0, 0, 0))
    ImageDraw.Draw(img).rounded_rectangle([0, 0, img.size[0] - 1, img.size[1] - 1], radius=img.size[1] // 4,
                                          fill=(255, 255, 255, 255))
    lx = L["logo_x"]
    add("teamchip", img, [lx - cw / 2, R / 2 - chh / 2, lx + cw / 2, R / 2 + chh / 2])
    add("dim", flat(W, R, (8, 8, 14, 150)), [0, 0, W, R])
    bx = L["pos_w"] + 0.6
    add("bar", flat(1.0, R - 6, (255, 255, 255, 255)), [bx, 3.0, bx + 1.0, R - 3.0])

    # position change arrows (in place of the number) and the gained / lost marks
    x = L["pos_x"]
    aw, ah = u(10.0), u(6.5)
    add("arr_up", triangle(aw, ah, True, C["arrow_up"]), [x - aw / 2, R / 2 - ah / 2, x + aw / 2, R / 2 + ah / 2])
    add("arr_dn", triangle(aw, ah, False, C["arrow_dn"]), [x - aw / 2, R / 2 - ah / 2, x + aw / 2, R / 2 + ah / 2])
    gx = L["gain_x"]
    gw, gh = u(8.0), u(5.5)
    add("gup", triangle(gw, gh, True, C["arrow_up"]), [gx - gw / 2, R / 2 - gh / 2, gx + gw / 2, R / 2 + gh / 2])
    add("gdn", triangle(gw, gh, False, C["arrow_dn"]), [gx - gw / 2, R / 2 - gh / 2, gx + gw / 2, R / 2 + gh / 2])
    add("gz", flat(u(7.0), 0.8, (150, 148, 160, 255)), [gx - u(3.5), R / 2 - 0.4, gx + u(3.5), R / 2 + 0.4])

    # outside chips (side stack): P box, penalty, fastest lap, chequered, tyre age
    ch = L["chip"]
    top = (R - ch) / 2
    add("pitbox", flat(ch, ch, (255, 255, 255, 255)), [0, top, ch, top + ch])
    # a penalty: the broadcast's red square with a white "!" (sampled: #C8201F)
    n = px(ch) * 4
    img = Image.new("RGBA", (n, n), hexrgba("#C8201F"))
    d = ImageDraw.Draw(img)
    bw = n * 0.16
    d.rectangle([n / 2 - bw / 2, n * 0.16, n / 2 + bw / 2, n * 0.62], fill=(255, 255, 255, 255))
    d.rectangle([n / 2 - bw / 2, n * 0.72, n / 2 + bw / 2, n * 0.86], fill=(255, 255, 255, 255))
    add("penbox", img.resize((px(ch), px(ch)), Image.LANCZOS), [0, top, ch, top + ch])
    add("agebox", flat(ch, ch, (24, 23, 35, 235)), [0, top, ch, top + ch])
    n = px(ch) * 4
    img = Image.new("RGBA", (n, n), hexrgba(C["purple"]))
    d = ImageDraw.Draw(img)
    c, r = n / 2, n * 0.27
    cy = c + n * 0.05
    d.ellipse([c - r, cy - r, c + r, cy + r], outline=(255, 255, 255, 255), width=int(n * 0.075))
    d.rectangle([c - n * 0.07, n * 0.1, c + n * 0.07, n * 0.17], fill=(255, 255, 255, 255))
    d.line([(c, cy), (c + r * 0.45, cy - r * 0.45)], fill=(255, 255, 255, 255), width=int(n * 0.07))
    add("flbox", img.resize((px(ch), px(ch)), Image.LANCZOS), [0, top, ch, top + ch])
    add("chqbox", chequer(ch * 1.3, ch), [0, top, ch * 1.3, top + ch])

    # header flag blocks (two and three lines) and the chequered strip
    for lines in (2, 3):
        h = L[f"block{lines}"]
        for key in ("sc", "red", "clear"):
            img = flat(W, h, hexrgba(C[key]))
            d = ImageDraw.Draw(img)
            d.rectangle([0, 0, img.size[0], int(round(L["block_gap"] * img.size[1] / h)) - 1], fill=(*C["band"], 255))
            add(f"blk{lines}_{key}", img, [0, 0, W, h])
    add("blk_chq", chequer(W, L["chq_h"]), [0, 0, W, L["chq_h"]])
    return sprites


def fit_into(src: Path, w_units, h_units):
    """The logo scaled to fit a w x h box (units), centred, at PX*2 pixels a unit."""
    im = Image.open(src).convert("RGBA")
    bb = im.getbbox()
    if bb:
        im = im.crop(bb)
    W, H = px(w_units) * 2, px(h_units) * 2
    s = min(W / im.size[0], H / im.size[1])
    im = im.resize((max(1, int(im.size[0] * s)), max(1, int(im.size[1] * s))), Image.LANCZOS)
    out = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    out.paste(im, ((W - im.size[0]) // 2, (H - im.size[1]) // 2), im)
    return out


def private_art(art: Path, logos: Path, sprites: dict):
    """Private build: the F1 logo, the FIA lockup's badge, the FIA badge of the flag blocks and the broadcast's team
    icons (tv_<team>.png from tools/fetch_assets.py). Returns the team indices that have an icon."""
    L = LAYOUT
    found = []

    def add(name, img, bounds):
        save(img, art / f"{name}.png")
        sprites[name] = {"png": f"art/{name}.png", "bounds": [round(b, 3) for b in bounds]}

    f1 = logos / "f1.png"
    if f1.exists():
        x, y, w, h = L["f1"]
        add("f1logo", fit_into(f1, w, h), [x, y, x + w, y + h])
    fw = logos / "fia_white.png"
    fb = logos / "fia_black.png"
    if fw.exists():
        x, y, w, h = L["lock"]
        sw = u(REF["fia_small_w"])
        add("fia_lock", fit_into(fw, sw, h), [x, y, x + sw, y + h])
    for name, src in (("fia_k", fb), ("fia_w", fw)):
        if src.exists():
            x0, w = L["fia_box"]
            for lines in (2, 3):
                hb = L[f"block{lines}"]
                g = L["block_gap"]
                h = L[f"fia_h{lines}"]
                cy = g + (hb - g) / 2
                add(f"{name}{lines}", fit_into(src, w, h), [x0, cy - h / 2, x0 + w, cy + h / 2])
    R = L["row"]
    for i, t in enumerate(TEAMS):
        src = logos / f"tv_{t}.png"
        if not src.exists():
            continue
        w, h = L["logo_w"], L["logo_h"]
        x = L["logo_x"]
        add(f"tm_{t}", fit_into(src, w, h), [x - w / 2, R / 2 - h / 2, x + w / 2, R / 2 + h / 2])
        found.append(i)
    return found


# ------------------------------------------------------------------------------------------------- record
def vm(name, neg=False, default=None):
    b = {"view_model": P + name}
    if neg:
        b["not"] = True
    if default is not None:
        b["default"] = default
    return b


# Measured in game (0.4.1 screenshots): the game draws a text's capitals 0.45 cap heights lower than the
# composer's baseline. Every text of the record is raised by that much (the HTML preview draws true baselines).
GAME_TEXT_DY = 0.45


def text(id_, style, align, box, cap, at, value=None, literal=None, colour=None, visible=None, wrap=None, case=None):
    at = [at[0], round(at[1] - GAME_TEXT_DY * cap, 3)]
    e = {"kind": "text", "id": id_, "style": style, "align": align, "box": box, "cap_height": cap, "at": at}
    if value is not None:
        e["value"] = value
    if literal is not None:
        e["literal"] = literal
    if colour:
        e["colour"] = colour
    if visible:
        e["visible"] = visible
    if wrap:
        e["wrap"] = wrap
    if case:
        e["case"] = case
    return e


def sprite(id_, name, layer, visible=None, at=None, tint=None):
    e = {"kind": "sprite", "id": id_, "sprite": name, "layer": layer}
    if at:
        e["at"] = at
    if visible:
        e["visible"] = visible
    if tint:
        e["tint"] = tint
    return e


def group(id_, children, visible=None, at=None, repeat=None):
    e = {"kind": "group", "id": id_, "children": children}
    if visible:
        e["visible"] = visible
    if at:
        e["at"] = at
    if repeat:
        e["repeat"] = repeat
    return e


def header(flavour, sprites):
    L = LAYOUT
    C = L["colours"]
    W = L["width"]
    bgs = [sprite(f"hb{i}", f"hdr{i}", 20, visible=vm(f"bg{i}")) for i in range(3)]
    if "f1logo" in sprites:
        logo = sprite("f1", "f1logo", 26)
    else:
        x, y, w, h = L["f1"]
        logo = {"kind": "unit", "id": "f1", "unit": "series_logo", "at": [x, y], "size": w,
                "texture": {"screen": "series_logo"}}
    # under the logo: "SPRINT" in sprint sessions; the FIA Formula 1 World Championship lockup otherwise (private)
    subs = [group("spr", [text("sprt", "OSD_34_bold", "centre", W, L["sprint_cap"], [W / 2, L["sprint_base"]],
                               literal="SPRINT")], visible=vm("sprint"))]
    if "fia_lock" in sprites:
        lx, ly, lw, lh = L["lock"]
        tx = lx + u(REF["fia_small_w"]) + u(1.5)
        cap = u(3.4)
        subs.append(group("lock", [
            sprite("lockf", "fia_lock", 26),
            text("lock1", "OSD_19_regular", "left", u(60), cap, [tx, ly + u(4.6)], literal="FIA Formula 1",
                 colour="#E8E8EEff", case="none"),
            text("lock2", "OSD_19_regular", "left", u(60), cap, [tx, ly + u(9.8)], literal="World Championship",
                 colour="#E8E8EEff", case="none"),
        ], visible=vm("sprint", True, True)))
    y = L["lap_base"]
    lw = L["cap_lapword"]
    ln = L["cap_lapnum"]
    # "LAP 26 / 71": one line per digit count of the lap (ld1..ld3), so "LAP", the number and "/ 71" keep the
    # broadcast's spacing; the game's own centring (eCentre) centres the whole line
    digit = u(8.4)            # one bold digit at the lap number's size (measured in game)
    def lap_line(nd):
        wn = digit * nd
        x_num = u(6.0)
        x_sl = x_num + wn + u(5.0)
        x_tot = x_sl + u(8.0)
        return group(f"ld{nd}", [{"kind": "centre", "id": f"lc{nd}", "at": [W / 2, y], "children": [
            text(f"lw{nd}", "OSD_19_regular", "right", u(30), lw, [0, 0], literal="LAP", colour=C["lapword"] + "ff"),
            text(f"ln{nd}", "OSD_24_bold", "left", u(30), ln, [x_num, 0], value=vm("lap")),
            text(f"ls{nd}", "OSD_19_regular", "left", u(8), lw, [x_sl, 0], literal="/", colour=C["lapword"] + "ff"),
            group(f"lt{nd}", [text(f"ltn{nd}", "OSD_19_regular", "left", u(24), lw, [x_tot, 0],
                                   value={"path": "#session.number_of_laps", "ty": "si32"},
                                   colour=C["lapword"] + "ff")], visible=vm("demo", True, True)),
            group(f"ltd{nd}", [text(f"ltdn{nd}", "OSD_19_regular", "left", u(24), lw, [x_tot, 0], value=vm("dlaps"),
                                    colour=C["lapword"] + "ff")], visible=vm("demo")),
        ]}], visible=vm(f"ld{nd}"))
    laps = group("laps", [lap_line(1), lap_line(2), lap_line(3)], visible=vm("form", True, True))
    form = group("formg", [text("formt", "OSD_24_bold", "centre", W, ln * 0.9, [W / 2, y], literal="FORMATION LAP")],
                 visible=vm("form"))
    race = group("race", [laps, form], visible=vm("race"))
    timed = group("timed", [
        {"kind": "centre", "id": "tc", "at": [W / 2, y], "children": [
            text("sess", "OSD_24_bold", "right", u(60), ln, [-u(3.0), 0], value=vm("sess")),
            group("clk", [text("clkn", "OSD_24_regular", "left", u(50), ln, [u(3.0), 0],
                               value={"session": "time_remaining_str"})], visible=vm("demo", True, True)),
            group("clkd", [text("clkdn", "OSD_24_regular", "left", u(50), ln, [u(3.0), 0], value=vm("dclk"))],
                  visible=vm("demo")),
        ]}], visible=vm("timed"))
    top = group("top", bgs + [logo] + subs + [race, timed])

    tx = L["block_text_x"]
    cap = L["cap_block"]

    def block(id_, colour_key, lines, ink="#0E0E10ff", value=None, fia="k"):
        """A flag block: the FIA badge at the left, two or three lines of heavy text."""
        n = len(lines)
        hb = L[f"block{n}"]
        kids = [sprite(id_ + "_bg", f"blk{n}_{colour_key}", 21)]
        x = tx
        if f"fia_{fia}{n}" in sprites:
            kids.append(sprite(id_ + "_f", f"fia_{fia}{n}", 23))
        else:
            # public build: no FIA badge (no mark is shipped, and a sprite drawn from a game texture stops
            # Swofty's profile build, DECISIONES.md D37): the text starts at the block's left margin
            x = u(8.0)
        for k, (line, base) in enumerate(zip(lines, L[f"base{n}"])):
            e = text(f"{id_}_t{k}", "OSD_24_bold", "left", W - x - 2, cap if line is not None else cap * 0.8,
                     [x, base], literal=line, colour=ink)
            if line is None:
                e.pop("literal", None)
                e["value"] = value
            kids.append(e)
        return group(id_, kids, visible=vm("c_" + id_))

    def band(id_, label, leaf):
        """A line of the dark LAP band (as GAINED / LOST): the view the data column shows."""
        return group(id_, [sprite(f"{id_}_bg{i}", f"gainbg{i}", 21, visible=vm(f"bg{i}")) for i in range(3)] +
                     [text(f"{id_}_t", "OSD_24_bold", "centre", W - u(10), L["cap_gainword"], [W / 2, L["gain_base"]],
                           literal=label, wrap="clampwidth")], visible=vm(leaf))

    # the 2026 world feed's flag blocks under the LAP band
    cards = {"kind": "stack", "id": "cards", "dir": "down", "gap": 0, "children": [
        group("gain", [sprite(f"gain_bg{i}", f"gainbg{i}", 21, visible=vm(f"bg{i}")) for i in range(3)] +
              [text("gain_t", "OSD_24_bold", "centre", W, L["cap_gainword"], [W / 2, L["gain_base"]],
                    literal="GAINED / LOST")], visible=vm("gained")),
        group("pits", [sprite(f"pits_bg{i}", f"gainbg{i}", 21, visible=vm(f"bg{i}")) for i in range(3)] +
              [text("pits_t", "OSD_24_bold", "centre", W, L["cap_gainword"], [W / 2, L["gain_base"]],
                    literal="PIT STOPS")], visible=vm("pstops")),
        band("tyv", "TYRES", "tyrev"),
        band("sty", "STARTING TYRES", "stty"),

        block("red", "red", ["RED", "FLAG"], "#FFFFFFff", fia="w"),
        block("sc", "sc", ["SAFETY", "CAR"]),
        block("vsc", "sc", ["VIRTUAL", "SAFETY", "CAR"]),
        block("vscend", "sc", ["VSC", "ENDING"]),
        block("scending", "sc", ["SC", "ENDING"]),
        block("clear", "clear", ["TRACK", "CLEAR"], "#FFFFFFff", fia="w"),
        block("yel", "sc", ["YELLOW", "FLAG", None], value=vm("ysec")),
    ]}
    return top, cards


def row(flavour, logos_found):
    L = LAYOUT
    C = L["colours"]
    R = L["row"]
    ty = L["text_y"]
    r = "r{NN}_"
    k = []
    k += [sprite(f"bg{i}", f"row{i}", 20, visible=vm(f"bg{i}")) for i in range(3)]
    k += [sprite("me", "me", 22, visible=vm(r + "me"))]
    k += [sprite("sep", "sep", 23, visible=vm(r + "sep"))]
    k += [sprite("p1", "p1", 24, visible=vm(r + "p1")), sprite("ko", "ko", 24, visible=vm(r + "ko")),
          sprite("kol", "koline", 25, visible=vm(r + "kol"))]
    # position: the arrow replaces the number while it shows
    k += [group("pa", [group("pb", [
        text("pos", "OSD_24_regular", "centre", L["pos_w"], L["cap_pos"], [L["pos_x"], ty], value=vm(r + "pos"))
    ], visible=vm(r + "dn", True, True))], visible=vm(r + "up", True, True))]
    k += [sprite("au", "arr_up", 26, visible=vm(r + "up")), sprite("ad", "arr_dn", 26, visible=vm(r + "dn"))]

    # team logo: the broadcast's icons (private build) by one-hot, else the game's own badge (colour or white)
    for i in logos_found:
        k.append(sprite(f"tm{i:02d}", f"tm_{TEAMS[i]}", 27, visible=vm(r + f"tm{i:02d}")))
    k.append({"kind": "unit", "id": "lg", "unit": "team_logo", "at": [L["logo_x"], R / 2], "size": L["unit_logo"],
              "visible": vm(r + "bdg"), "texture": vm(r + "badge")})

    # driver name: the game's own per-car values, one slot per vehicle id (changed only while the row shows)
    k.append(group("v", [
        group("cd", [text("code", "OSD_24_bold", "left", L["name_box"], L["cap_name"], [L["name_x"], ty],
                          value={"session_car": "driver.name_code", "slot_var": "V"}, wrap="clampwidth",
                          case="upper")], visible=vm("abbr")),
        group("sn", [text("sur", "OSD_24_bold", "left", L["name_box"], L["cap_name"], [L["name_x"], ty],
                          value={"session_car": "driver.second_name", "slot_var": "V"}, wrap="clampwidth",
                          case="upper")], visible=vm("abbr", True, True)),
        group("cb", [sprite("bar", "bar", 28, tint={"session_car": "nub_colour", "slot_var": "V",
                                                      "default": "#00000000"})], visible=vm("colbar")),
        group("nb", [sprite("chip", "teamchip", 27, tint={"session_car": "nub_colour", "slot_var": "V",
                                                           "default": "#00000000"})], visible=vm(r + "nob")),
    ], visible=vm(r + "c{VV}"), repeat={"count": SLOTS, "var": "V"}))
    k.append(group("nd", [text("dn", "OSD_24_bold", "left", L["name_box"], L["cap_name"], [L["name_x"], ty],
                               value=vm(r + "nm"), wrap="clampwidth", case="upper")], visible=vm("demo")))

    # data column: white (gap / time), purple, green, grey label (Interval / Leader), red (IN PIT)
    gx = L["gap_right"]
    k.append(group("gw", [text("g", "OSD_24_regular", "right", u(52), L["cap_gap"], [gx, ty],
                               value=vm(r + "gap"), colour=C["gap"] + "ff", case="none", wrap="clampwidth")],
                   visible=vm(r + "gsp", True, True)))
    k.append(text("gp", "OSD_24_bold", "right", L["gap_box"], L["cap_gap"], [gx, ty], value=vm(r + "gap"),
                  colour=C["purple"] + "ff", visible=vm(r + "gpu")))
    k.append(text("gg", "OSD_24_bold", "right", L["gap_box"], L["cap_gap"], [gx, ty], value=vm(r + "gap"),
                  colour=C["green"] + "ff", visible=vm(r + "gpg")))
    k.append(text("gl", "OSD_24_regular", "right", u(40), L["cap_label"], [gx, ty], value=vm(r + "gap"),
                  colour=C["label"] + "ff", visible=vm(r + "glb"), case="none", wrap="clampwidth"))
    k.append(text("gi", "OSD_24_bold", "right", u(46), u(6.6), [L["tyre_x"] + u(4), ty], value=vm(r + "gap"),
                  colour=C["inpit"] + "ff", visible=vm(r + "gip"), wrap="clampwidth"))

    # places gained / lost: the mark, then the white number (the gap text)
    k += [sprite("gu", "gup", 27, visible=vm(r + "gu")), sprite("gd", "gdn", 27, visible=vm(r + "gd")),
          sprite("g0", "gz", 27, visible=vm(r + "g0"))]

    # tyre: the compound's letter in its colour
    for t, col in C["tyres"].items():
        k.append(text(f"t{t}", "OSD_24_bold", "centre", u(10), L["cap_tyre"], [L["tyre_x"], ty], literal=t,
                      colour=col + "ff", visible=vm(r + f"t{t}")))

    # outside chips, laid out left to right by the game's stacker
    ch = L["chip"]
    side = {"kind": "stack", "id": "side", "dir": "right", "gap": u(1.5), "at": [L["side_x"], 0], "children": [
        group("pit", [sprite("pb", "pitbox", 30),
                      text("pt", "OSD_24_bold", "centre", ch, u(7.0), [ch / 2, R / 2 + u(3.5)], literal="P",
                           colour="#101018ff")], visible=vm(r + "pit")),
        group("pen", [sprite("nb", "penbox", 30)], visible=vm(r + "pen")),
        group("fl", [sprite("fb", "flbox", 30)], visible=vm(r + "fl")),
        group("chq", [sprite("cb", "chqbox", 30)], visible=vm(r + "chq")),
        group("age", [sprite("ab", "agebox", 30),
                      text("at", "OSD_24_regular", "centre", ch, u(5.4), [ch / 2, R / 2 + u(2.7)],
                           value=vm(r + "tage"))], visible=vm(r + "tag")),
    ]}
    k.append(side)
    k.append(sprite("dim", "dim", 45, visible=vm(r + "dim")))
    # a change of car on the row: the content fades out, the DLL swaps it while hidden, and it fades back in
    # (rNN_fx false -> true). The background stays. Off with the Animations setting.
    static_ids = {"bg0", "bg1", "bg2", "me", "sep", "p1", "ko", "kol", "pa", "au", "ad"}
    bg = [e for e in k if e["id"] in static_ids]
    content = [e for e in k if e["id"] not in static_ids]
    rc = group("rc", content)
    rc["motion"] = {"id": "fx", "keys": [[0, 0.0], [0.16, 1.0]], "curve": "generic_fade_on",
                    "play_on": vm(r + "fx")}
    return group("r", bg + [rc], visible=vm(r + "on"), repeat={"count": ROWS, "var": "N", "pitch": [0, R]})


def preview_leaves():
    """The HUD editor's static five-row copy."""
    pv = {P + "bg1": True, P + "timed": True, P + "sess": "P1", P + "demo": True, P + "dclk": "33:56",
          P + "abbr": True}
    names = ["RUS", "VER", "HAD", "ALB", "HAM"]
    gaps = ["1:19.832", "+0.214", "+0.377", "+0.402", "+0.518"]
    for i in range(5):
        n = f"{P}r{i:02d}_"
        pv[n + "on"] = True
        pv[n + "pos"] = str(i + 1)
        pv[n + "nm"] = names[i]
        pv[n + "gap"] = gaps[i]
    pv[P + "r00_p1"] = True
    pv[P + "r01_pit"] = True
    pv[P + "r03_me"] = True
    return pv


def unique_ids(elements):
    """Element ids are unique across a record: prefix a repeated id with its parent's."""
    seen = set()

    def walk(items, parent):
        for e in items:
            i = e["id"]
            if i in seen:
                i = f"{parent}_{i}"
                n = 2
                while i in seen:
                    i = f"{parent}_{e['id']}{n}"
                    n += 1
                e["id"] = i
            seen.add(i)
            walk(e.get("children", []), i)

    walk(elements, "x")
    return elements


def record(flavour, sprites, logos_found):
    top, cards = header(flavour, sprites)
    rows = group("rows", [row(flavour, logos_found)])
    tower = group("tower", [{"kind": "stack", "id": "stack", "dir": "down", "gap": 0,
                             "children": [top, cards, rows]}], at=[0, LAYOUT["offset_y"]])
    # More than 22 rows showing (23-24: a My Team grid, "Rows: All") makes a visibly taller tower than a normal
    # 20-22 car field (hidden rows cost no space, but these are drawn). Slide the whole tower further up while
    # that many are on screen; not measured in game, so tune extra_up in LAYOUT if it still runs under something
    # else or off the bottom.
    tower["motion"] = {"id": "wide", "keys": [[0, [0, 0]], [0.4, [0, -LAYOUT["extra_up"]]]],
                       "curve": "generic_fade_on", "play_on": vm("wide")}
    elements = unique_ids([tower])
    rec = {
        "$comment": "TV Timing Tower: generated by tools/hudgen.py - edit the generator, not this file.",
        "prefix": "tvtt",
        "screen": "standings",
        "show": vm("show"),
        # replays and the spectator screen: off (not testable here; see DECISIONES.md D17)
        "sprites": sprites,
        "elements": elements,
        # as Project Halo's tower (seen in game): the DLL keeps the clock leaf registered ("" while not timed) so
        # the node exists, and the clone of the game's formatter writes the time into it from `timed`
        "clock": {"output": P + "clock", "start": vm("timed")},
        "stock_nodes": [
            {"template": "hud_lap_counter.osd_visibility_option",
             "scene": "eaid://ui_package/art/osd/components/lap_counter/lap_counter.idf",
             "hide": ["car_category_logo", "session_icon", "centre_lap_value", "separator_bg", "separator_bg_top",
                      "centre_formation"],
             "wrap": ["edge_base", "edge_border", "lap_bg", "lockup"],
             "while": vm("show")},
            {"template": "hud_left_notification_dynamic.osd_visibility_option.left_notification.left_message",
             "hide": ["boundless"], "while": vm("show")},
        ],
        "editor": {"rows": 5, "preview": preview_leaves()},
    }
    return {"hud.element": {"tower": rec}}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out", type=Path)
    ap.add_argument("--flavour", choices=["private", "public"], required=True)
    ap.add_argument("--logos", type=Path, default=Path("assets/private/logos"))
    a = ap.parse_args()
    out = a.out
    art = out / "art"
    if art.exists():
        shutil.rmtree(art)
    sprites = make_art(art)
    found = []
    if a.flavour == "private" and a.logos.exists():
        found = private_art(art, a.logos, sprites)
    # every sprite must be a PNG of the addon: Swofty 1.9.14's profile build reads each sprite's file and stops with
    # "os error 3" on one drawn from a game texture (`texture`), although `swofty hud compose` accepts it
    assert all("png" in s for s in sprites.values()), "texture sprites break the profile build"
    (out / "registry").mkdir(parents=True, exist_ok=True)
    rec = record(a.flavour, sprites, found)
    (out / "registry" / "tower.json").write_text(json.dumps(rec, indent=1), encoding="utf-8")
    meta = dict(LAYOUT, flavour=a.flavour, teams=TEAMS, logos=[TEAMS[i] for i in found],
                f1logo="f1logo" in sprites, sprites=sprites)
    (out / "layout.json").write_text(json.dumps(meta, indent=1), encoding="utf-8")
    print(f"hudgen: {a.flavour}: {len(sprites)} sprites, {len(found)} team icons, f1 logo {'f1logo' in sprites}")


if __name__ == "__main__":
    main()
