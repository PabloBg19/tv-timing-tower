#!/usr/bin/env python3
"""TV Timing Tower - writes the hud.element record and its sprites for one flavour.

    python tools/hudgen.py <out_addon_dir> --flavour private|public [--logos assets/private/logos]

Geometry lives in LAYOUT below (HUD units of the standings screen: the stock board is 300 wide and its rows
21.635 apart). The same LAYOUT is exported to <out>/layout.json for the HTML preview, so the preview and the
game draw from one set of numbers.
"""
import argparse
import json
import math
import shutil
import sys
from pathlib import Path

from PIL import Image, ImageDraw

P = "view_model_data.hud_tvt_"   # leaf prefix (must match native/src/leaves.hpp)
ROWS = 22
SLOTS = 24
TEAMS = ["mercedes", "ferrari", "redbull", "mclaren", "astonmartin", "alpine",
         "williams", "racingbulls", "haas", "sauber", "audi", "cadillac"]
PX = 4  # sprite pixels per HUD unit

LAYOUT = {
    # 0.2: shorter rows so 22 cars fit on screen, the tower moved up into the space of the (hidden) lap counter
    "offset_y": -40.0,       # the whole tower, from the stock board's origin
    "width": 186.0,
    "row": 18.0,
    "header": 38.0,
    "card": 26.0,            # flag block: FIA badge + two lines (2026 world feed)
    "gainbar": 11.0,         # "GAINED / LOST" bar under the header
    "pos_x": 9.0,
    "logo_x": 25.0,
    "logo_size": 12.5,
    "bar_x": 17.6,
    "name_x": 33.5,
    "name_box": 78.0,
    "gap_right": 168.0,
    "gap_box": 48.0,
    "tyre_x": 177.5,
    "text_y": 12.6,          # baseline of the row's texts
    "cap_name": 7.0,
    "cap_pos": 7.0,
    "cap_gap": 6.6,
    "cap_label": 5.0,
    "cap_tyre": 6.4,
    "side_x": 188.0,         # outside items (P box, penalty, fastest lap, flag)
    "chip": 12.0,
    "colours": {
        "bg": [14, 17, 36],
        "hdr": [10, 12, 27],
        "alpha": [0.95, 0.84, 0.66],
        "p1": "#E10600",
        "ko": "#8E0B12",
        "purple": "#B138DD",
        "green": "#2BD15F",
        "gap": "#E8E8EC",
        "label": "#AEB3C9",
        "sc": "#FFD12E",
        "red": "#E10600",
        "clear": "#1E9E3E",
        "tyres": {"S": "#E8002D", "M": "#FFD12E", "H": "#F0F0EC", "I": "#43B02A", "W": "#0072C6"},
    },
}


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


def make_art(art: Path):
    art.mkdir(parents=True, exist_ok=True)
    L = LAYOUT
    C = L["colours"]
    W, R, H = L["width"], L["row"], L["header"]
    sprites = {}

    def add(name, img, bounds):
        save(img, art / f"{name}.png")
        sprites[name] = {"png": f"art/{name}.png", "bounds": [round(b, 3) for b in bounds]}

    # row and header backgrounds, three opacities; a soft vertical shade keeps the column one surface
    for i, a in enumerate(C["alpha"]):
        img = flat(W, R, (0, 0, 0, 0))
        d = ImageDraw.Draw(img)
        for y in range(img.size[1]):
            t = y / max(1, img.size[1] - 1)
            c = [int(v * (1.0 + 0.10 * (0.5 - t))) for v in C["bg"]]
            d.line([(0, y), (img.size[0], y)], fill=(*c, int(255 * a)))
        add(f"row{i}", img, [0, 0, W, R])
        img = flat(W, H, (0, 0, 0, 0))
        d = ImageDraw.Draw(img)
        for y in range(img.size[1]):
            t = y / max(1, img.size[1] - 1)
            c = [int(v * (1.15 - 0.25 * t)) for v in C["hdr"]]
            d.line([(0, y), (img.size[0], y)], fill=(*c, int(255 * min(1.0, a + 0.03))))
        # thin red rule under the logo, as the broadcast header
        add(f"hdr{i}", img, [0, 0, W, H])

    add("hdr_rule", flat(W - 24, 0.6, hexrgba("#FFFFFF", 38)), [12, 21.6, W - 12, 22.2])
    add("me", flat(W, R, (255, 255, 255, 34)), [0, 0, W, R])
    add("me_edge", flat(1.2, R, (255, 255, 255, 235)), [0, 0, 1.2, R])
    bh = R - 4.0
    add("p1", flat(15, bh, hexrgba(C["p1"])), [1.5, 2.0, 16.5, 2.0 + bh])
    add("ko", flat(15, bh, hexrgba(C["ko"])), [1.5, 2.0, 16.5, 2.0 + bh])
    add("koline", flat(W, 0.8, hexrgba(C["p1"])), [0, 0, W, 0.8])
    add("sep", flat(W - 8, 0.6, (255, 255, 255, 70)), [4, 0, W - 4, 0.6])
    add("dim", flat(W, R, (6, 7, 14, 150)), [0, 0, W, R])
    add("bar", flat(1.2, R - 6, (255, 255, 255, 255)), [L["bar_x"], 3.0, L["bar_x"] + 1.2, R - 3.0])

    # position arrows
    for name, up, col in (("arr_up", True, C["green"]), ("arr_dn", False, "#E8002D")):
        img = flat(8, 8, (0, 0, 0, 0))
        d = ImageDraw.Draw(img)
        s = img.size[0]
        pts = [(s / 2, s * 0.12), (s * 0.92, s * 0.85), (s * 0.08, s * 0.85)] if up else \
              [(s * 0.08, s * 0.15), (s * 0.92, s * 0.15), (s / 2, s * 0.88)]
        d.polygon(pts, fill=hexrgba(col))
        x = L["pos_x"]
        add(name, img, [x - 4, R / 2 - 4, x + 4, R / 2 + 4])

    # places gained / lost: green and red triangles, a grey dash, left of the number
    gx = L["gap_right"] - 15.0
    for name, up, col in (("gup", True, C["green"]), ("gdn", False, "#E8002D")):
        img = flat(6, 6, (0, 0, 0, 0))
        d = ImageDraw.Draw(img)
        n = img.size[0]
        pts = [(n / 2, n * 0.1), (n * 0.95, n * 0.9), (n * 0.05, n * 0.9)] if up else \
              [(n * 0.05, n * 0.1), (n * 0.95, n * 0.1), (n / 2, n * 0.9)]
        d.polygon(pts, fill=hexrgba(col))
        add(name, img, [gx - 3, R / 2 - 3.2, gx + 3, R / 2 + 2.8])
    add("gz", flat(5, 0.9, (170, 175, 195, 255)), [gx - 2.5, R / 2 - 0.6, gx + 2.5, R / 2 + 0.3])
    add("gainbg", flat(W, L["gainbar"], (46, 51, 72, 235)), [0, 0, W, L["gainbar"]])

    # outside chips: positioned by the side stack, bounds from the chip's own origin
    ch = L["chip"]
    top = (R - ch) / 2
    add("pitbox", flat(ch, ch, (255, 255, 255, 255)), [0, top, ch, top + ch])
    add("penbox", flat(19, ch, hexrgba("#F5B400")), [0, top, 19, top + ch])
    add("agebox", flat(ch, ch, (20, 24, 44, 235)), [0, top, ch, top + ch])
    # fastest lap: purple chip with a stopwatch
    n = px(ch) * 2
    img = Image.new("RGBA", (n, n), hexrgba(C["purple"]))
    d = ImageDraw.Draw(img)
    c, r = n / 2, n * 0.28
    d.ellipse([c - r, c - r + n * 0.05, c + r, c + r + n * 0.05], outline=(255, 255, 255, 255), width=int(n * 0.07))
    d.rectangle([c - n * 0.06, n * 0.12, c + n * 0.06, n * 0.2], fill=(255, 255, 255, 255))
    d.line([(c, c + n * 0.05), (c, c - r * 0.55 + n * 0.05)], fill=(255, 255, 255, 255), width=int(n * 0.06))
    add("flbox", img.resize((px(ch), px(ch)), Image.LANCZOS), [0, top, ch, top + ch])
    # chequered chip
    img = Image.new("RGBA", (px(16), px(ch)), (255, 255, 255, 255))
    d = ImageDraw.Draw(img)
    q = img.size[1] / 4
    for yy in range(4):
        for xx in range(int(img.size[0] / q) + 1):
            if (xx + yy) % 2:
                d.rectangle([xx * q, yy * q, (xx + 1) * q - 1, (yy + 1) * q - 1], fill=(12, 12, 12, 255))
    add("chqbox", img, [0, top, 16, top + ch])

    # header flag blocks; the FIA badge sits on a navy square at the left, as on the 2026 world feed
    cd = L["card"]
    add("fiabox", flat(18, 18, (16, 35, 105, 255)), [5, (cd - 18) / 2, 23, (cd + 18) / 2])
    add("card_sc", flat(W, cd, hexrgba(C["sc"])), [0, 0, W, cd])
    add("card_red", flat(W, cd, hexrgba(C["red"])), [0, 0, W, cd])
    add("card_clear", flat(W, cd, hexrgba(C["clear"])), [0, 0, W, cd])
    img = Image.new("RGBA", (px(W), px(cd)), (255, 255, 255, 255))
    d = ImageDraw.Draw(img)
    q = img.size[1] / 4
    for yy in range(4):
        for xx in range(int(img.size[0] / q) + 1):
            if (xx + yy) % 2:
                d.rectangle([xx * q, yy * q, (xx + 1) * q - 1, (yy + 1) * q - 1], fill=(10, 10, 10, 255))
    add("card_chq", img, [0, 0, W, cd])
    return sprites


def fit_logo(src: Path, size_px: int):
    im = Image.open(src).convert("RGBA")
    bbox = im.getbbox()
    if bbox:
        im = im.crop(bbox)
    w, h = im.size
    s = (size_px * 0.92) / max(w, h)
    im = im.resize((max(1, int(w * s)), max(1, int(h * s))), Image.LANCZOS)
    out = Image.new("RGBA", (size_px, size_px), (0, 0, 0, 0))
    out.paste(im, ((size_px - im.size[0]) // 2, (size_px - im.size[1]) // 2), im)
    return out


def private_logos(art: Path, logos: Path, sprites: dict):
    """The F1 logo for the header from assets/private/logos (private build). Team logos are the game's own since
    0.2 (they follow logo mods); the HTML preview reads assets/private/logos itself."""
    L = LAYOUT
    f1 = logos / "f1.png"
    if f1.exists():
        im = Image.open(f1).convert("RGBA")
        bb = im.getbbox()
        im = im.crop(bb) if bb else im
        w, h = 40.0, 40.0 * im.size[1] / im.size[0]
        if h > 10.5:
            w, h = w * 10.5 / h, 10.5
        out = im.resize((px(w) * 2, px(h) * 2), Image.LANCZOS)
        save(out, art / "f1logo.png")
        x0 = (L["width"] - w) / 2
        sprites["f1logo"] = {"png": "art/f1logo.png", "bounds": [x0, 3.0, x0 + w, 3.0 + h]}


# ------------------------------------------------------------------------------------------------- record
def vm(name, neg=False, default=None):
    b = {"view_model": P + name}
    if neg:
        b["not"] = True
    if default is not None:
        b["default"] = default
    return b


def text(id_, style, align, box, cap, at, value=None, literal=None, colour=None, visible=None, wrap=None, case=None):
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


FIA_TEX = "eaid://ui_package/art/osd/textures/message_system/ms_fia_logo.tif"


def header(flavour, has_f1):
    L = LAYOUT
    W = L["width"]
    bgs = [sprite(f"hb{i}", f"hdr{i}", 20, visible=vm(f"bg{i}")) for i in range(3)]
    if has_f1:
        logo = sprite("f1", "f1logo", 26)
    else:
        logo = {"kind": "unit", "id": "f1", "unit": "series_logo", "at": [W / 2 - 14.5, 3.0], "size": 29.0,
                "texture": {"screen": "series_logo"}}
    # the line under the logo: "SPRINT" in sprint sessions; the championship's name otherwise (private build)
    subs = [group("spr", [text("sprt", "OSD_24_bold", "centre", 60, 4.6, [W / 2, 19.6], literal="SPRINT")],
                  visible=vm("sprint"))]
    if flavour == "private":
        subs.append(group("fiaw", [text("fiat", "OSD_19_regular", "centre", 120, 2.9, [W / 2, 19.2],
                                        literal="FIA Formula 1 World Championship", colour="#d8dbe6ff")],
                          visible=vm("sprint", True, True)))
    y = 32.0
    # "LAP 23 / 57": the number right-aligned against the slash, "LAP" a fixed space before the widest number
    race = group("race", [
        {"kind": "centre", "id": "lc", "at": [W / 2, y], "children": [
            text("lw", "OSD_19_regular", "right", 22, 5.8, [-15.5, 0], literal="LAP"),
            text("ln", "OSD_24_bold", "right", 16, 7.2, [3.0, 0], value=vm("lap")),
            text("ls", "OSD_19_regular", "centre", 6, 5.8, [6.5, 0], literal="/"),
            group("lt", [text("ltn", "OSD_19_regular", "left", 16, 5.8, [9.6, 0],
                              value={"path": "#session.number_of_laps", "ty": "si32"})], visible=vm("demo", True, True)),
            group("ltd", [text("ltdn", "OSD_19_regular", "left", 16, 5.8, [9.6, 0], value=vm("dlaps"))],
                  visible=vm("demo")),
        ]}], visible=vm("race"))
    timed = group("timed", [
        text("sess", "OSD_24_bold", "right", 60, 7.2, [W / 2 - 3.0, y], value=vm("sess")),
        group("clk", [text("clkn", "OSD_24_regular", "left", 40, 7.2, [W / 2 + 3.0, y],
                           value={"session": "time_remaining_str"})], visible=vm("demo", True, True)),
        group("clkd", [text("clkdn", "OSD_24_regular", "left", 40, 7.2, [W / 2 + 3.0, y], value=vm("dclk"))],
              visible=vm("demo")),
    ], visible=vm("timed"))
    top = group("top", bgs + [sprite("rule", "hdr_rule", 24), logo] + subs + [race, timed])

    cd = L["card"]
    x = 29.0

    def block(id_, spr, lines, colour="#101010ff", value=None):
        """A flag block: the FIA badge and one or two lines, left-aligned (the second may be a leaf)."""
        kids = [sprite(id_ + "_bg", spr, 21), sprite(id_ + "_fb", "fiabox", 22),
                sprite(id_ + "_fl", "fia", 23, at=[14.0, cd / 2])]
        if len(lines) == 1 and value is None:
            kids.append(text(id_ + "_t", "OSD_24_bold", "left", W - x - 4, 7.2, [x, cd / 2 + 3.6],
                             literal=lines[0], colour=colour))
        else:
            kids.append(text(id_ + "_t", "OSD_24_bold", "left", W - x - 4, 7.2, [x, 11.6], literal=lines[0],
                             colour=colour))
            if value is not None:
                kids.append(text(id_ + "_u", "OSD_24_bold", "left", W - x - 4, 7.2, [x, 21.6], value=value,
                                 colour=colour))
            else:
                kids.append(text(id_ + "_u", "OSD_24_bold", "left", W - x - 4, 7.2, [x, 21.6], literal=lines[1],
                                 colour=colour))
        return group(id_, kids, visible=vm("c_" + id_))

    # the broadcast's flag states (FOM world feed 2026): a coloured block under the header with the FIA badge
    cards = {"kind": "stack", "id": "cards", "dir": "down", "gap": 0, "children": [
        group("chq", [sprite("chq_bg", "card_chq", 21)], visible=vm("c_chq")),
        block("red", "card_red", ["RED FLAG"], "#ffffffff"),
        block("scend", "card_sc", ["SAFETY CAR", "IN THIS LAP"]),
        block("sc", "card_sc", ["SAFETY", "CAR"]),
        block("vsc", "card_sc", ["VIRTUAL", "SAFETY CAR"]),
        block("vscend", "card_sc", ["VSC", "ENDING"]),
        block("clear", "card_clear", ["TRACK CLEAR"], "#ffffffff"),
        block("yel", "card_sc", ["YELLOW FLAG"], value=vm("ysec")),
        group("gain", [sprite("gain_bg", "gainbg", 21),
                       text("gain_t", "OSD_24_bold", "centre", W - 8, 5.0, [W / 2, L["gainbar"] / 2 + 2.5],
                            literal="GAINED / LOST")], visible=vm("gained")),
    ]}
    return top, cards


def row(flavour):
    L = LAYOUT
    C = L["colours"]
    R = L["row"]
    ty = L["text_y"]
    r = "r{NN}_"
    k = []
    k += [sprite(f"bg{i}", f"row{i}", 20, visible=vm(f"bg{i}")) for i in range(3)]
    k += [sprite("me", "me", 22, visible=vm(r + "me")), sprite("mee", "me_edge", 23, visible=vm(r + "me"))]
    k += [sprite("sep", "sep", 23, visible=vm(r + "sep"))]
    k += [sprite("p1", "p1", 24, visible=vm(r + "p1")), sprite("ko", "ko", 24, visible=vm(r + "ko")),
          sprite("kol", "koline", 25, visible=vm(r + "kol"))]
    # position: the arrow replaces the number while it shows
    k += [group("pa", [group("pb", [
        text("pos", "OSD_24_bold", "centre", 16, L["cap_pos"], [L["pos_x"], ty], value=vm(r + "pos"))
    ], visible=vm(r + "dn", True, True))], visible=vm(r + "up", True, True))]
    k += [sprite("au", "arr_up", 26, visible=vm(r + "up")), sprite("ad", "arr_dn", 26, visible=vm(r + "dn"))]

    # team logo: always the game's own badge (broadcast set in colour, or the HUD set), so logo mods show too
    k.append({"kind": "unit", "id": "lg", "unit": "team_logo", "at": [L["logo_x"], R / 2], "size": L["logo_size"],
              "visible": vm(r + "bdg"), "texture": vm(r + "badge")})

    # driver name: the game's own per-car values, one slot per vehicle id. No parent of these switches turns on
    # and off with demo mode: the DLL clears the slots (c = -1) in demo, while the row shows.
    k.append(group("v", [
        group("cd", [text("code", "OSD_24_bold", "left", L["name_box"], L["cap_name"], [L["name_x"], ty],
                          value={"session_car": "driver.name_code", "slot_var": "V"}, wrap="clampwidth",
                          case="upper")], visible=vm("abbr")),
        group("sn", [text("sur", "OSD_24_bold", "left", L["name_box"], L["cap_name"], [L["name_x"], ty],
                          value={"session_car": "driver.second_name", "slot_var": "V"}, wrap="clampwidth",
                          case="upper")], visible=vm("abbr", True, True)),
        group("cb", [sprite("bar", "bar", 28, tint={"session_car": "nub_colour", "slot_var": "V",
                                                      "default": "#00000000"})], visible=vm("colbar")),
    ], visible=vm(r + "c{VV}"), repeat={"count": SLOTS, "var": "V"}))
    k.append(group("nd", [text("dn", "OSD_24_bold", "left", L["name_box"], L["cap_name"], [L["name_x"], ty],
                               value=vm(r + "nm"), wrap="clampwidth", case="upper")], visible=vm("demo")))

    # data column: normal, purple (fastest / session best), green (personal best), label (INTERVAL / LEADER)
    gx = L["gap_right"]
    k.append(group("gw", [text("g", "OSD_24_regular", "right", L["gap_box"], L["cap_gap"], [gx, ty],
                               value=vm(r + "gap"), colour=C["gap"] + "ff")],
                   visible=vm(r + "gsp", True, True)))
    k.append(text("gp", "OSD_24_bold", "right", L["gap_box"], L["cap_gap"], [gx, ty], value=vm(r + "gap"),
                  colour=C["purple"] + "ff", visible=vm(r + "gpu")))
    k.append(text("gg", "OSD_24_bold", "right", L["gap_box"], L["cap_gap"], [gx, ty], value=vm(r + "gap"),
                  colour=C["green"] + "ff", visible=vm(r + "gpg")))
    k.append(text("gl", "OSD_19_regular", "right", L["gap_box"], L["cap_label"], [gx, ty - 0.5],
                  value=vm(r + "gap"), colour=C["label"] + "ff", visible=vm(r + "glb")))

    # places gained / lost (the number is the white gap text)
    k += [sprite("gu", "gup", 27, visible=vm(r + "gu")), sprite("gd", "gdn", 27, visible=vm(r + "gd")),
          sprite("g0", "gz", 27, visible=vm(r + "g0"))]

    # tyre: the compound's letter in its colour, as on the 2026 broadcast (no ring)
    for t, col in C["tyres"].items():
        k.append(text(f"t{t}", "OSD_24_bold", "centre", 10, L["cap_tyre"], [L["tyre_x"], ty], literal=t,
                      colour=col + "ff", visible=vm(r + f"t{t}")))

    # outside chips, laid out left to right by the game's stacker
    ch = L["chip"]
    side = {"kind": "stack", "id": "side", "dir": "right", "gap": 1.2, "at": [L["side_x"], 0], "children": [
        group("pit", [sprite("pb", "pitbox", 30),
                      text("pt", "OSD_24_bold", "centre", ch, 6.4, [ch / 2, R / 2 + 3.2], literal="P",
                           colour="#101018ff")], visible=vm(r + "pit")),
        group("pen", [sprite("nb", "penbox", 30),
                      text("nt", "OSD_19_regular", "centre", 19, 5.2, [9.5, R / 2 + 2.6], value=vm(r + "pent"),
                           colour="#101010ff")], visible=vm(r + "pen")),
        group("fl", [sprite("fb", "flbox", 30)], visible=vm(r + "fl")),
        group("chq", [sprite("cb", "chqbox", 30)], visible=vm(r + "chq")),
        group("age", [sprite("ab", "agebox", 30),
                      text("at", "OSD_19_regular", "centre", ch, 5.2, [ch / 2, R / 2 + 2.6], value=vm(r + "tage"))],
              visible=vm(r + "tag")),
    ]}
    k.append(side)
    k.append(sprite("dim", "dim", 45, visible=vm(r + "dim")))
    return group("r", k, visible=vm(r + "on"), repeat={"count": ROWS, "var": "N", "pitch": [0, R]})


def preview_leaves():
    """The HUD editor's static five-row copy."""
    pv = {P + "bg1": True, P + "timed": True, P + "sess": "P1", P + "demo": True, P + "dclk": "33:56"}
    names = ["RUSSELL", "VERSTAPPEN", "HADJAR", "ALBON", "HAMILTON"]
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


def record(flavour, sprites, has_f1):
    top, cards = header(flavour, has_f1)
    rows = group("rows", [row(flavour)])
    elements = unique_ids([group("tower", [{"kind": "stack", "id": "stack", "dir": "down", "gap": 0,
                                            "children": [top, cards, rows]}], at=[0, LAYOUT["offset_y"]])])
    rec = {
        "$comment": "TV Timing Tower: generated by tools/hudgen.py - edit the generator, not this file.",
        "prefix": "tvtt",
        "screen": "standings",
        "show": vm("show"),
        # replays and the spectator screen: off in 0.1.0 (not testable here; see DECISIONES.md D17)
        "sprites": sprites,
        "elements": elements,
        "clock": {"output": P + "clock", "start": vm("clk")},
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
    sprites["fia"] = {"texture": FIA_TEX, "fit": [15.0, 10.0]}
    has_f1 = False
    if a.flavour == "private" and a.logos.exists():
        private_logos(art, a.logos, sprites)
        has_f1 = "f1logo" in sprites
    (out / "registry").mkdir(parents=True, exist_ok=True)
    rec = record(a.flavour, sprites, has_f1)
    (out / "registry" / "tower.json").write_text(json.dumps(rec, indent=1), encoding="utf-8")
    meta = dict(LAYOUT, flavour=a.flavour, teams=TEAMS, f1logo=has_f1, sprites=sprites)
    (out / "layout.json").write_text(json.dumps(meta, indent=1), encoding="utf-8")
    print(f"hudgen: {a.flavour}: {len(sprites)} sprites, f1 logo {has_f1}")


if __name__ == "__main__":
    main()
