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
    "logo_x": 29.0, "logo_w": 15.0, "logo_h": 11.0,
    # the game's own badge: a square texture, so a wide logo (Force India, Lotus, Manor) fills only a band of it;
    # the square uses the whole gap between the position box (19) and the name (42)
    "unit_logo": 19.0, "unit_x": 30.5,
    "name_x": 42.0,
    "gap_right": 117.5,
    "gap_right_q": 121.0, "cap_gap_q": 6.2,   # practice / qualifying data column
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

# The broadcast's qualifying card, measured on 1080p captures (px from the card's top-left): 388 x 127, header 43
# with the position box 38 wide, a 3 px team colour line, then the body; at 20 % of the screen's width as on TV
# (205 units: 0.528 a pixel)
QC_S = 205.0 / 388.0


def q(v):
    return round(v * QC_S, 3)


QCARD = {
    "w": q(388), "h": q(127), "hdr": q(43), "line": q(3), "posw": q(38),
    "pos_cap": q(12), "pos_x": q(19), "logo_x": q(62), "name_x": q(85), "name_cap": q(16),
    "tyre_x": q(365), "tyre_d": q(26), "tyre_cap": q(11),
    "time_x": q(15), "time_base": q(82), "time_cap": q(27),
    "delta_base": q(102), "delta_cap": q(10),
    "div_x": q(188), "div_y": [q(55), q(95)],
    "tgt_right": q(373), "tgt_base": q(75), "tgt_cap": q(17), "tn_base": q(95), "tn_cap": q(11),
    "cut_x": [q(228), q(276)], "cut_h": q(15), "cut_cap": q(8), "cutname_x": q(282),
    "lab_base": q(118), "lab_cap": q(9), "lab_dx": q(3),
    "bars": [[q(11), q(129)], [q(135), q(253)], [q(259), q(377)]], "bar_y": [q(123), q(127)],
    # where it sits: right of the tower, its bottom near the screen's at the default HUD place (a 2000 x 1133
    # capture: tower top 97 px, 1.93 px a unit, screen bottom 537 units below the tower's top)
    "x": 8.0, "y": 448.0,
    "colours": {
        "hdr": "#202026", "pos": "#08080A", "body": "#171C22", "body2": "#14181D", "div": "#4A4D52",
        "green": "#3CCF3C", "yellow": "#F2CF0D", "purple": "#C13FDA", "now": "#6B6E73", "later": "#35383D",
        "lab_dim": "#6B6E73", "tname": "#A3A5AA", "cut": "#3D4046", "cut_text": "#C9CBCF",
    },
}

LAYOUT = {
    "offset_y": -40.0,       # the whole tower, from the stock board's origin (into the hidden lap counter's space)
    "extra_up": 24.0,        # added on top of offset_y while more than 22 rows show (D62, not measured in game)
    # the NEW RACE LEADER banner top centre, where the game puts its own centre notification. Measured
    # on a full-screen 2000 x 1125 capture at the default HUD place: the tower's left edge at 41 px, 207 px wide
    # (1.88 px a unit), so the screen's centre is 510 units right of it, and the game's toast starts 27 units
    # above the tower's top. Sized as the broadcast's: 37 % of the screen's width.
    "ban_cx": 510.0, "ban_y": -27.0,
    "ban_h": 38.0, "ban_lw": 364.0, "ban_cap": 12.0,
    "ban_name_w": 138.0, "ban_lab_x": 157.0, "ban_team_w": 49.0, "ban_chev": 12.0,
    "row": ROW_UNITS,
    "width": u(REF["width"]),
    "pos_w": u(REF["pos_w"]), "pos_x": u(REF["pos_x"]),
    "logo_x": u(REF["logo_x"]), "logo_w": u(REF["logo_w"]), "logo_h": u(REF["logo_h"]), "unit_logo": u(REF["unit_logo"]), "unit_x": u(REF["unit_x"]),
    "name_x": u(REF["name_x"]),
    "name_box": u(REF["gap_right"] - REF["name_x"] - 2),
    "gap_right": u(REF["gap_right"]), "gap_box": u(70), "gap_right_q": u(REF["gap_right_q"]), "cap_gap_q": u(REF["cap_gap_q"]),
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
    "champ_h": u(36.0), "champ_b1": u(15.5), "champ_b2": u(29.0),
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
        "ban_dark": [20, 19, 30], "ban_label": "#D8D6E2",
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

    # the qualifying card (QCARD): bounds from the card's top-left
    Q, QC = QCARD, QCARD["colours"]
    qw, qh, qhd, ql = Q["w"], Q["h"], Q["hdr"], Q["line"]
    img = flat(qw, qh, (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    sy = img.size[1] / qh
    sx = img.size[0] / qw
    rad = int(1.4 * sx)
    d.rounded_rectangle([0, 0, img.size[0] - 1, img.size[1] - 1], rad, fill=hexrgba(QC["body"], 240))
    top = int((qhd + ql) * sy)
    for yy in range(top, img.size[1] - rad):
        t = (yy - top) / max(1, img.size[1] - top)
        c0, c1 = hexrgba(QC["body"]), hexrgba(QC["body2"])
        d.line([(0, yy), (img.size[0] - 1, yy)], fill=tuple(int(c0[k] + (c1[k] - c0[k]) * t) for k in range(3)) + (240,))
    d.rectangle([0, rad, img.size[0] - 1, int(qhd * sy)], fill=hexrgba(QC["hdr"], 240))
    d.rounded_rectangle([0, 0, img.size[0] - 1, int(qhd * sy)], rad, fill=hexrgba(QC["hdr"], 240))
    d.rectangle([0, rad, int(Q["posw"] * sx), int(qhd * sy)], fill=hexrgba(QC["pos"], 250))
    d.rounded_rectangle([0, 0, int(Q["posw"] * sx), int(qhd * sy)], rad, fill=hexrgba(QC["pos"], 250))
    d.rectangle([int(Q["posw"] * sx) - rad, 0, int(Q["posw"] * sx), int(qhd * sy)], fill=hexrgba(QC["pos"], 250))
    x0 = int(Q["div_x"] * sx)
    d.rectangle([x0, int(Q["div_y"][0] * sy), x0 + max(1, int(0.5 * sx)), int(Q["div_y"][1] * sy)],
                fill=hexrgba(QC["div"]))
    add("qc_bg", img, [0, 0, qw, qh])
    # the team colour line (tinted per car) and the tyre rings
    add("qc_line", flat(qw, ql, (255, 255, 255, 255)), [0, qhd, qw, qhd + ql])
    td = Q["tyre_d"]
    for t, col in C["tyres"].items():
        img = flat(td, td, (0, 0, 0, 0))
        big = img.resize((img.size[0] * 4, img.size[1] * 4))
        dd = ImageDraw.Draw(big)
        N = big.size[0]
        dd.ellipse([N * 0.04, N * 0.04, N * 0.96, N * 0.96], outline=hexrgba(col), width=int(N * 0.085))
        add(f"qc_ty{t}", big.resize(img.size, Image.LANCZOS),
            [Q["tyre_x"] - td / 2, qhd / 2 - td / 2, Q["tyre_x"] + td / 2, qhd / 2 + td / 2])
    by0, by1 = Q["bar_y"]
    for i, (bx0, bx1) in enumerate(Q["bars"]):
        for st, col in (("later", QC["later"]), ("now", QC["now"]), ("green", QC["green"]),
                        ("yellow", QC["yellow"]), ("purple", QC["purple"])):
            add(f"qc_b{i}{st}", flat(bx1 - bx0, by1 - by0, hexrgba(col)), [bx0, by0, bx1, by1])
    cx0, cx1 = Q["cut_x"]
    cy = Q["tn_base"] - Q["tn_cap"] / 2
    img = flat(cx1 - cx0, Q["cut_h"], (0, 0, 0, 0))
    ImageDraw.Draw(img).rounded_rectangle([0, 0, img.size[0] - 1, img.size[1] - 1], int(0.8 * PX), fill=hexrgba(QC["cut"]))
    add("qc_cutbg", img, [cx0, cy - Q["cut_h"] / 2, cx1, cy + Q["cut_h"] / 2])

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
        # DRIVERS' / CHAMPIONSHIP on two lines (the formation lap)
        add(f"champbg{i}", flat(W, L["champ_h"], (*C["band"], A)), [0, 0, W, L["champ_h"]])

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

    # banners: drawn 4x and scaled down (smooth slanted edges)
    bh, sl = L["ban_h"], L["ban_chev"]

    def poly(w, h, shapes):
        img = flat(w, h, (0, 0, 0, 0))
        big = img.resize((img.size[0] * 4, img.size[1] * 4))
        d = ImageDraw.Draw(big)
        kx, ky = big.size[0] / w, big.size[1] / h
        for pts, fill in shapes:
            if callable(fill):  # a vertical gradient
                mask = Image.new("L", big.size, 0)
                ImageDraw.Draw(mask).polygon([(x * kx, y * ky) for x, y in pts], fill=255)
                grad = Image.new("RGBA", big.size)
                gd = ImageDraw.Draw(grad)
                for y in range(big.size[1]):
                    gd.line([(0, y), (big.size[0], y)], fill=fill(y / max(1, big.size[1] - 1)))
                big.paste(grad, (0, 0), mask)
            else:
                d.polygon([(x * kx, y * ky) for x, y in pts], fill=fill)
        return big.resize(img.size, Image.LANCZOS)

    def shade(rgb, a=242):
        return lambda t: (*[int(c * (1.08 - 0.16 * t)) for c in rgb], a)

    lw, tw = L["ban_lw"], L["ban_team_w"]
    # NEW RACE LEADER: the dark bar, and the team colour block on its left (white, tinted with the car's colour)
    add("ban_l_bg", poly(lw, bh, [([(0, 0), (lw, 0), (lw, bh), (0, bh)], shade(C["ban_dark"]))]), [0, 0, lw, bh])
    # the team block at the right end (where the broadcast puts the driver's face), its left edge slanted
    tb = [lw - tw - sl, 0, lw, bh]
    add("ban_team", poly(tw + sl, bh, [([(sl, 0), (tw + sl, 0), (tw + sl, bh), (0, bh)], (255, 255, 255, 255))]), tb)
    add("ban_team_d", poly(tw + sl, bh, [([(sl, 0), (tw + sl, 0), (tw + sl, bh), (0, bh)], (120, 120, 132, 255))]), tb)
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
        # the banners' larger copy, centred on its own origin
        bw, bh2 = w * 2.0, h * 2.0
        add(f"tmb_{t}", fit_into(src, bw, bh2), [-bw / 2, -bh2 / 2, bw / 2, bh2 / 2])
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
                     [x, base], literal=line, colour=ink, wrap="clampwidth")
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
        group("chp", [sprite(f"chp_bg{i}", f"champbg{i}", 21, visible=vm(f"bg{i}")) for i in range(3)] +
              [text("chp_t1", "OSD_24_bold", "centre", W - u(10), L["cap_gainword"], [W / 2, L["champ_b1"]],
                    literal="DRIVERS'", wrap="clampwidth"),
               text("chp_t2", "OSD_24_bold", "centre", W - u(10), L["cap_gainword"], [W / 2, L["champ_b2"]],
                    literal="CHAMPIONSHIP", wrap="clampwidth")], visible=vm("champ")),

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
    k.append({"kind": "unit", "id": "lg", "unit": "team_logo", "at": [L["unit_x"], R / 2], "size": L["unit_logo"],
              "visible": vm(r + "bdg"), "texture": vm(r + "badge")})

    # driver name: the game's own per-car values, one slot per vehicle id (changed only while the row shows). The texts
    # carry their own visible (no wrapper groups): 24 rows x 30 slots is most of the record, and the game's HUD
    # customisation screen walks the record six times within 65,536 slots (Swofty's walk.rs; 0.17.2 used ~69,200 there
    # and closed the game). A TINTED sprite ignores a visible of its own in game (seen on the 2021 tower: the colour
    # nodes showed it anyway), so the team chip keeps its wrapper group; the optional team colour bar is gone (0.17.4).
    tint = {"session_car": "nub_colour", "slot_var": "V", "default": "#00000000"}
    k.append(group("v", [
        text("code", "OSD_24_bold", "left", L["name_box"], L["cap_name"], [L["name_x"], ty],
             value={"session_car": "driver.name_code", "slot_var": "V"}, wrap="clampwidth", case="upper",
             visible=vm("abbr")),
        text("sur", "OSD_24_bold", "left", L["name_box"], L["cap_name"], [L["name_x"], ty],
             value={"session_car": "driver.second_name", "slot_var": "V"}, wrap="clampwidth", case="upper",
             visible=vm("abbr", True, True)),
        group("nb", [sprite("chip", "teamchip", 27, tint=tint)], visible=vm(r + "nob")),
    ], visible=vm(r + "c{VV}"), repeat={"count": SLOTS, "var": "V"}))
    k.append(group("nd", [text("dn", "OSD_24_bold", "left", L["name_box"], L["cap_name"], [L["name_x"], ty],
                               value=vm(r + "nm"), wrap="clampwidth", case="upper")], visible=vm("demo")))

    # data column: white (gap / time), purple, green, grey label (Interval / Leader), red (IN PIT)
    # race: the gap at the race captures' size. Practice / qualifying: the leader's lap time (1:05.173) ran into the
    # name at that size (0.18.0 in game), so as on the broadcast's qualifying tower (measured on a 2026 Q1 capture:
    # time ending 5.74 rows from the left, the tyre letter at 6.13) the column ends further right and its figures are
    # smaller (our font is wider than the broadcast's narrow one)
    def data(gx, cap):
        return [group("gw", [text("g", "OSD_24_regular", "right", u(52), cap, [gx, ty],
                                  value=vm(r + "gap"), colour=C["gap"] + "ff", case="none", wrap="clampwidth")],
                      visible=vm(r + "gsp", True, True)),
                text("gp", "OSD_24_bold", "right", L["gap_box"], cap, [gx, ty], value=vm(r + "gap"),
                     colour=C["purple"] + "ff", visible=vm(r + "gpu")),
                text("gg", "OSD_24_bold", "right", L["gap_box"], cap, [gx, ty], value=vm(r + "gap"),
                     colour=C["green"] + "ff", visible=vm(r + "gpg"))]
    gx = L["gap_right"]
    k.append(group("dr", data(gx, L["cap_gap"]), visible=vm("timed", True, True)))
    k.append(group("dq", data(L["gap_right_q"], L["cap_gap_q"]), visible=vm("timed")))
    k.append(text("gl", "OSD_24_regular", "right", u(40), L["cap_label"], [gx, ty], value=vm(r + "gap"),
                  colour=C["label"] + "ff", visible=vm(r + "glb"), case="none", wrap="clampwidth"))
    k.append(text("gi", "OSD_24_bold", "right", u(46), u(6.6), [L["tyre_x"] + u(4), ty], value=vm(r + "gap"),
                  colour=C["inpit"] + "ff", visible=vm(r + "gip"), wrap="clampwidth"))
    # the championship's points: right-aligned at the row's right edge and a little smaller than a gap, so a long
    # surname keeps its room (0.17.0 put them at the tyre column and they ran into VERSTAPPEN in game)
    k.append(text("gc", "OSD_24_regular", "right", u(18), u(6.0), [L["width"] - u(1.0), ty],
                  value=vm(r + "gap"), colour=C["gap"] + "ff", visible=vm(r + "gch"), case="none", wrap="clampwidth"))

    # places gained / lost: the mark, then the white number (the gap text)
    k += [sprite("gu", "gup", 27, visible=vm(r + "gu")), sprite("gd", "gdn", 27, visible=vm(r + "gd")),
          sprite("g0", "gz", 27, visible=vm(r + "g0"))]

    # STARTING TYRES: the compound's name in its colour, right-aligned across the data and tyre columns (as IN PIT)
    names = {"S": "SOFT", "M": "MEDIUM", "H": "HARD", "I": "INTER", "W": "WET"}
    for t, col in C["tyres"].items():
        k.append(text(f"s{t}", "OSD_24_bold", "right", u(52), L["cap_gap"], [L["tyre_x"] + u(4), ty],
                      literal=names[t], colour=col + "ff", visible=vm(r + f"s{t}"), wrap="clampwidth"))

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


def banners(logos_found):
    """NEW RACE LEADER, top centre: "LECLERC NEW RACE LEADER" and the team's colour block with its logo where the
    broadcast puts the driver's face. A group (no fade: a motion trigger on our own leaf is not reliable in game,
    and a missed one left the banner invisible) holding the art and label (shown with the leaf) and, straight under
    it, the per-car name / colour slots and the team logo, which the DLL switches only while the banner shows (a
    change under a hidden node is lost). FASTEST LAP was dropped in 0.15 (DECISIONES.md D76)."""
    L = LAYOUT
    C = L["colours"]
    bh, cap = L["ban_h"], L["ban_cap"]
    ty = bh / 2 + cap / 2
    k = "bnl"
    # the surname right-aligned against the label (the game gives no text width)
    logo_cx = L["ban_lw"] - L["ban_team_w"] / 2
    name_x, name_box = L["ban_lab_x"] - 6.0, L["ban_name_w"]
    # layers as the rows': the background under the game's team badge unit, which always draws at 30
    art = [sprite(f"{k}bg", "ban_l_bg", 20),
           group(f"{k}tdg", [sprite(f"{k}td", "ban_team_d", 21)], visible=vm("demo")),
           text(f"{k}lab", "OSD_24_bold", "left", 120.0, cap, [L["ban_lab_x"], ty], literal="NEW RACE LEADER",
                colour=C["ban_label"] + "ff"),
           group(f"{k}dn", [text(f"{k}dnm", "OSD_24_bold", "right", name_box, cap, [name_x, ty],
                                 value=vm(f"{k}_nm"), wrap="clampwidth", case="upper")], visible=vm("demo"))]
    slot = [sprite(f"{k}tc", "ban_team", 21, tint={"session_car": "nub_colour", "slot_var": "V",
                                                    "default": "#00000000"}),
            text(f"{k}sn", "OSD_24_bold", "right", name_box, cap, [name_x, ty],
                 value={"session_car": "driver.second_name", "slot_var": "V"}, wrap="clampwidth", case="upper")]
    cars = group(f"{k}v", slot, visible=vm(f"{k}_c{{VV}}"), repeat={"count": SLOTS, "var": "V"})
    # the team logo: the broadcast icon (private build) by one-hot, else the game's badge
    lg = [group(f"{k}tm{i:02d}g", [sprite(f"{k}tm{i:02d}", f"tmb_{TEAMS[i]}", 27)], at=[logo_cx, bh / 2],
                visible=vm(f"{k}_tm{i:02d}")) for i in logos_found]
    lg.append({"kind": "unit", "id": f"{k}lg", "unit": "team_logo", "at": [logo_cx, bh / 2],
               "size": L["unit_logo"] * 1.7, "visible": vm(f"{k}_bdg"), "texture": vm(f"{k}_badge")})
    g = group(f"{k}g", [group(f"{k}art", art, visible=vm(k)), cars] + lg, at=[-L["ban_lw"] / 2, 0])
    return group("ban", [g], at=[L["ban_cx"], L["offset_y"] + L["ban_y"]])


def qcard(logos_found):
    """The qualifying card (as the broadcast's, captures in DECISIONES.md D90): the player's position, team, surname
    and tyre; the lap time (running in tenths, then the lap in thousandths), the gap at the last split to the target
    lap, the target (P1, or the cut with CUT P15) and S1 S2 S3. Like the banner: the art follows `qc`, and the per
    car name / colour slots sit straight under the always-shown group (they change only while the card shows)."""
    Q, QC = QCARD, QCARD["colours"]
    C = LAYOUT["colours"]
    hd = Q["hdr"]
    base = lambda cap, cy: cy + cap / 2  # a text's baseline for its capitals centred on cy
    art = [sprite("qbg", "qc_bg", 20),
           text("qpos", "OSD_24_bold", "centre", Q["posw"], Q["pos_cap"], [Q["pos_x"], base(Q["pos_cap"], hd / 2)],
                value=vm("qc_pos"), case="none"),
           text("qt", "OSD_24_bold", "left", q(170), Q["time_cap"], [Q["time_x"], Q["time_base"]], value=vm("qc_t"),
                case="none", wrap="clampwidth"),
           text("qtt", "OSD_24_regular", "right", q(170), Q["tgt_cap"], [Q["tgt_right"], Q["tgt_base"]],
                value=vm("qc_tt"), case="none", wrap="clampwidth"),
           group("qcutg", [sprite("qcutb", "qc_cutbg", 21),
                           text("qcutt", "OSD_24_bold", "centre", Q["cut_x"][1] - Q["cut_x"][0], Q["cut_cap"],
                                [(Q["cut_x"][0] + Q["cut_x"][1]) / 2, base(Q["cut_cap"], Q["tn_base"] - Q["tn_cap"] / 2)],
                                value=vm("qc_cutl"), colour=QC["cut_text"] + "ff", case="none")], visible=vm("qc_cut"))]
    for i, (key, col) in enumerate((("g", QC["green"]), ("y", QC["yellow"]), ("p", QC["purple"]))):
        art.append(text(f"qd{key}", "OSD_24_regular", "left", q(120), Q["delta_cap"], [Q["time_x"], Q["delta_base"]],
                        value=vm("qc_d"), colour=col + "ff", visible=vm(f"qc_dc{i + 1}"), case="none"))
    for k, t in enumerate("SMHIW"):
        art.append(sprite(f"qty{t}", f"qc_ty{t}", 22, visible=vm(f"qc_ty{k}")))
        art.append(text(f"qtl{t}", "OSD_24_bold", "centre", Q["tyre_d"], Q["tyre_cap"],
                        [Q["tyre_x"], base(Q["tyre_cap"], hd / 2)], literal=t, colour=C["tyres"][t] + "ff",
                        visible=vm(f"qc_ty{k}")))
    states = [("later", QC["lab_dim"]), ("now", QC["lab_dim"]), ("green", QC["green"]), ("yellow", QC["yellow"]),
              ("purple", QC["purple"])]
    for i, (bx0, _) in enumerate(Q["bars"]):
        art.append(sprite(f"qb{i}l", f"qc_b{i}later", 21))
        for st_i, (st, col) in enumerate(states):
            if st_i > 0:
                art.append(sprite(f"qb{i}{st[0]}", f"qc_b{i}{st}", 22, visible=vm(f"qc_s{i + 1}_{st_i}")))
            art.append(text(f"ql{i}{st[0]}", "OSD_24_regular", "left", q(40), Q["lab_cap"],
                            [bx0 + Q["lab_dx"], Q["lab_base"]], literal=f"S{i + 1}", colour=col + "ff",
                            visible=vm(f"qc_s{i + 1}_{st_i}")))
    # demo: the simulated names
    art.append(group("qdn", [
        text("qdnm", "OSD_24_bold", "left", q(270), Q["name_cap"], [Q["name_x"], base(Q["name_cap"], hd / 2)],
             value=vm("qc_nm"), wrap="clampwidth", case="upper"),
        text("qdtn", "OSD_24_regular", "right", q(150), Q["tn_cap"], [Q["tgt_right"], Q["tn_base"]],
             value=vm("qc_tnm"), colour=QC["tname"] + "ff", wrap="clampwidth", case="upper")], visible=vm("demo")))

    tint = {"session_car": "nub_colour", "slot_var": "V", "default": "#00000000"}
    me = group("qv", [
        text("qn", "OSD_24_bold", "left", q(270), Q["name_cap"], [Q["name_x"], base(Q["name_cap"], hd / 2)],
             value={"session_car": "driver.second_name", "slot_var": "V"}, wrap="clampwidth", case="upper"),
        group("qlg", [sprite("qln", "qc_line", 21, tint=tint)]),
    ], visible=vm("qc_c{VV}"), repeat={"count": SLOTS, "var": "V"})
    tgt = group("qw", [
        text("qwn", "OSD_24_regular", "right", q(150), Q["tn_cap"], [Q["tgt_right"], Q["tn_base"]],
             value={"session_car": "driver.second_name", "slot_var": "V"}, colour=QC["tname"] + "ff",
             wrap="clampwidth", case="upper", visible=vm("qc_ncut")),
        text("qwc", "OSD_24_regular", "left", Q["tgt_right"] - Q["cutname_x"], Q["tn_cap"], [Q["cutname_x"], Q["tn_base"]],
             value={"session_car": "driver.second_name", "slot_var": "V"}, colour=QC["tname"] + "ff",
             wrap="clampwidth", case="upper", visible=vm("qc_cut")),
    ], visible=vm("qc_n{VV}"), repeat={"count": SLOTS, "var": "V"})
    # the team logo: the broadcast icon (private build: the rows' sprite, moved) by one-hot, else the game's badge
    lx, ly = Q["logo_x"], hd / 2
    R = LAYOUT["row"]
    lg = [group(f"qtm{i:02d}g", [sprite(f"qtm{i:02d}", f"tm_{TEAMS[i]}", 27)],
                at=[lx - LAYOUT["logo_x"], ly - R / 2], visible=vm(f"qc_tm{i:02d}")) for i in logos_found]
    lg.append({"kind": "unit", "id": "qlgu", "unit": "team_logo", "at": [lx, ly], "size": LAYOUT["unit_logo"] * 1.1,
               "visible": vm("qc_bdg"), "texture": vm("qc_badge")})
    g = group("qg", [group("qart", art, visible=vm("qc")), me, tgt] + lg)
    return group("qcard", [g], at=[LAYOUT["width"] + Q["x"], LAYOUT["offset_y"] + Q["y"]])


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
    elements = unique_ids([tower, banners(logos_found), qcard(logos_found)])
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
    meta = dict(LAYOUT, qcard=QCARD, flavour=a.flavour, teams=TEAMS, logos=[TEAMS[i] for i in found],
                f1logo="f1logo" in sprites, sprites=sprites)
    (out / "layout.json").write_text(json.dumps(meta, indent=1), encoding="utf-8")
    print(f"hudgen: {a.flavour}: {len(sprites)} sprites, {len(found)} team icons, f1 logo {'f1logo' in sprites}")


if __name__ == "__main__":
    main()
