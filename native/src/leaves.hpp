// SPDX-License-Identifier: MIT
// TV Timing Tower - the HUD leaves (view_model_data.hud_tvt_*) and the View -> leaves mapping.
// The record in tools/hudgen.py binds exactly these names; native/test checks the two stay in step.
#pragma once
#include <string>
#include <vector>

#include <swofty/hud_leaves.hpp>

#include "model.hpp"

namespace tvt {

namespace hud = swofty::hud;

inline constexpr const char *kPrefix = "view_model_data.hud_tvt_";

struct RowLeaves {
    hud::BoolLeaf on, sep, me, p1, ko, kol, dim, up, dn;
    hud::TextLeaf pos, gap;
    hud::BoolLeaf gpu, gpg, glb, gip, gsp;
    hud::BoolLeaf tS, tM, tH, tI, tW;
    hud::TextLeaf tage;
    hud::BoolLeaf tag, pit, pen;
    hud::TextLeaf pent;
    hud::BoolLeaf fl, chq, gu, gd, g0;
    hud::TextLeaf nm, badge;
    hud::BoolLeaf bdg;
    hud::OneHot tm, car;
};

class Publisher {
public:
    // private_teams: the team indices the record has a broadcast icon sprite for (native/tvt_logos.txt)
    explicit Publisher(std::vector<int> private_teams = {}) : teams_(std::move(private_teams))
    {
        // the first three change together: show first so the stock board and ours swap in one tick
        show = L.boolean("show");
        on = L.boolean("on");
        timed = L.boolean("timed");
        race = L.boolean("race");
        demo = L.boolean("demo");
        abbr = L.boolean("abbr");
        colbar = L.boolean("colbar");
        bg = L.one_hot("bg", 3);
        sess = L.text("sess", 16);
        lap = L.text("lap", 8);
        dlaps = L.text("dlaps", 8);
        dclk = L.text("dclk", 12);
        c_chq = L.boolean("c_chq");
        c_red = L.boolean("c_red");
        c_scend = L.boolean("c_scend");
        c_sc = L.boolean("c_sc");
        c_vsc = L.boolean("c_vsc");
        c_vscend = L.boolean("c_vscend");
        c_clear = L.boolean("c_clear");
        c_yel = L.boolean("c_yel");
        ysec = L.text("ysec", 24);
        clk = L.boolean("clk");
        sprint = L.boolean("sprint");
        gained = L.boolean("gained");
        rows = L.rows<RowLeaves>("r", kRows, 2, [](hud::Leaves &l, int) {
            RowLeaves r;
            r.on = l.boolean("on");
            r.sep = l.boolean("sep");
            r.me = l.boolean("me");
            r.p1 = l.boolean("p1");
            r.ko = l.boolean("ko");
            r.kol = l.boolean("kol");
            r.dim = l.boolean("dim");
            r.up = l.boolean("up");
            r.dn = l.boolean("dn");
            r.pos = l.text("pos", 4);
            r.gap = l.text("gap", 16);
            r.gpu = l.boolean("gpu");
            r.gpg = l.boolean("gpg");
            r.glb = l.boolean("glb");
            r.gip = l.boolean("gip");
            r.gsp = l.boolean("gsp");
            r.tS = l.boolean("tS");
            r.tM = l.boolean("tM");
            r.tH = l.boolean("tH");
            r.tI = l.boolean("tI");
            r.tW = l.boolean("tW");
            r.tage = l.text("tage", 4);
            r.tag = l.boolean("tag");
            r.pit = l.boolean("pit");
            r.pen = l.boolean("pen");
            r.pent = l.text("pent", 6);
            r.fl = l.boolean("fl");
            r.chq = l.boolean("chq");
            r.nm = l.text("nm", 24);
            r.badge = l.text("badge", 96);
            r.bdg = l.boolean("bdg");
            r.gu = l.boolean("gu");
            r.gd = l.boolean("gd");
            r.g0 = l.boolean("g0");
            r.tm = l.one_hot("tm", kTeams, 2);
            r.car = l.one_hot("c", kSlots, 2);
            return r;
        });
    }

    // Tower off: our leaves say so, the stock board comes back. The rows keep their leaves: under a hidden node the
    // game drops visibility changes, so rows change only while the tower shows (see publish).
    void off()
    {
        L.set(show, false);
        L.set(on, false);
        L.flush();
    }

    // The game re-formats the session clock each time `clk` goes from false to true (the record's clock start), so
    // the caller pulses it a few times a second while a timed session shows.
    void clock_pulse(bool on)
    {
        L.set(clk, on);
    }

    void publish(const View &v, const Config &cfg)
    {
        const Header &h = v.h;
        L.set(show, h.on);
        L.set(on, h.on);
        L.set(timed, h.on && h.timed);
        L.set(race, h.on && h.race);
        L.set(demo, h.demo);
        L.set(abbr, h.abbr);
        L.set(colbar, cfg.colbar);
        L.set(bg, cfg.bg < 0 || cfg.bg > 2 ? 1 : cfg.bg);
        L.set(sess, h.sess);
        L.set(lap, h.lap);
        L.set(dlaps, h.laps_total);
        L.set(dclk, h.clock);
        L.set(c_chq, h.card == card_chq);
        L.set(c_red, h.card == card_red);
        L.set(c_scend, h.card == card_sc_end);
        L.set(c_sc, h.card == card_sc);
        L.set(c_vsc, h.card == card_vsc);
        L.set(c_vscend, h.card == card_vsc_end);
        L.set(c_clear, h.card == card_clear);
        L.set(c_yel, h.card == card_yellow);
        L.set(ysec, h.ysec);
        L.set(sprint, !h.sub.empty());
        L.set(gained, h.gained);

        for (int i = 0; i < kRows; i++) {
            const Row &r = v.rows[size_t(i)];
            RowLeaves &x = rows[i];
            hud::Leaves::Atomic group(L);
            L.set(x.on, r.on);
            if (!r.on) {
                // The per-car name slots keep their value while the row is hidden: the game does not apply a
                // visibility change under a hidden node, so a slot cleared now could show again with the row
                // (two names in one row). They change only while the row shows.
                L.set(x.bdg, false);
                L.set(x.pit, false);
                L.set(x.pen, false);
                L.set(x.fl, false);
                L.set(x.chq, false);
                L.set(x.tag, false);
                continue;
            }
            L.set(x.sep, r.sep);
            L.set(x.me, r.me);
            L.set(x.p1, r.p1 && !r.ko);
            L.set(x.ko, r.ko);
            L.set(x.kol, r.ko_line);
            L.set(x.dim, r.out || r.elim);
            L.set(x.up, r.up);
            L.set(x.dn, r.dn);
            L.set(x.pos, r.pos);
            L.set(x.gap, r.gap);
            L.set(x.gpu, r.gap_purple);
            L.set(x.gpg, r.gap_green);
            L.set(x.glb, r.gap_label);
            L.set(x.gip, r.gap_inpit);
            L.set(x.gsp, r.gap_purple || r.gap_green || r.gap_label || r.gap_inpit);
            L.set(x.tS, r.tyre == 'S');
            L.set(x.tM, r.tyre == 'M');
            L.set(x.tH, r.tyre == 'H');
            L.set(x.tI, r.tyre == 'I');
            L.set(x.tW, r.tyre == 'W');
            L.set(x.tage, r.tyre_age);
            L.set(x.tag, !r.tyre_age.empty());
            L.set(x.pit, r.pit);
            L.set(x.pen, r.pen);
            L.set(x.pent, r.pen_text);
            L.set(x.fl, r.fl);
            L.set(x.chq, r.chq);
            L.set(x.gu, r.gain_on && r.gain > 0);
            L.set(x.gd, r.gain_on && r.gain < 0);
            L.set(x.g0, r.gain_on && r.gain == 0);
            L.set(x.nm, r.name);
            bool icon = cfg.logos == Config::Logos::tv && r.team >= 0 && has_team(r.team);
            L.set(x.tm, icon ? r.team : -1);
            L.set(x.badge, icon ? std::string() : r.badge);
            L.set(x.bdg, !icon && !r.badge.empty());
            L.set(x.car, r.vid >= 0 && r.vid < kSlots && !h.demo ? r.vid : -1);
        }
        L.flush();
    }

    hud::Leaves L{kPrefix};
    hud::BoolLeaf show, on, timed, race, demo, abbr, colbar;
    hud::OneHot bg;
    hud::TextLeaf sess, lap, dlaps, dclk;
    hud::BoolLeaf c_chq, c_red, c_scend, c_sc, c_vsc, c_vscend, c_clear, c_yel;
    hud::TextLeaf ysec;
    hud::BoolLeaf clk, gained, sprint;
    hud::Leaves::Rows<RowLeaves> rows;

private:
    bool has_team(int t) const
    {
        for (int x : teams_)
            if (x == t)
                return true;
        return false;
    }
    std::vector<int> teams_;
};

} // namespace tvt
