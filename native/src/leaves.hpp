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
    hud::BoolLeaf fl, chq, gu, gd, g0, fx, nob;
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
        c_sc = L.boolean("c_sc");
        c_vsc = L.boolean("c_vsc");
        c_vscend = L.boolean("c_vscend");
        c_scending = L.boolean("c_scending");
        c_clear = L.boolean("c_clear");
        c_yel = L.boolean("c_yel");
        ysec = L.text("ysec", 24);
        clock = L.text("clock", 16);   // written once as "" so the node exists; the game's formatter fills it
        L.set(clock, "");
        ld = L.one_hot("ld", 3, 0, 1); // ld1..ld3: digits of the lap number
        sprint = L.boolean("sprint");
        pstops = L.boolean("pstops");
        tyrev = L.boolean("tyrev");
        stty = L.boolean("stty");

        form = L.boolean("form");
        wide = L.boolean("wide");
        {
            BannerLeaves &x = ban;
            std::string k = "bnl";
            x.on = L.boolean(k);
            x.car = L.one_hot(k + "_c", kSlots, 2);
            x.tm = L.one_hot(k + "_tm", kTeams, 2);
            x.badge = L.text(k + "_badge", 96);
            x.bdg = L.boolean(k + "_bdg");
            x.nm = L.text(k + "_nm", 24); // demo: the simulated driver's name
        }
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
            r.fx = l.boolean("fx");
            r.nob = l.boolean("nob");
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

    // now: seconds (the row change fade's timing). anim: fade a row out and in when its car changes.
    void publish(const View &v, const Config &cfg, double now = 0, bool anim = false)
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
        L.set(ld, h.lap.size() >= 3 ? 2 : h.lap.size() == 2 ? 1 : 0);
        L.set(dlaps, h.laps_total);
        L.set(dclk, h.clock);
        L.set(c_chq, h.card == card_chq);
        L.set(c_red, h.card == card_red);
        L.set(c_sc, h.card == card_sc);
        L.set(c_vsc, h.card == card_vsc);
        L.set(c_vscend, h.card == card_vsc_end);
        L.set(c_scending, h.card == card_sc_ending);
        L.set(c_clear, h.card == card_clear);
        L.set(c_yel, h.card == card_yellow);
        L.set(ysec, h.ysec);
        L.set(sprint, !h.sub.empty());
        L.set(gained, h.gained);
        L.set(pstops, h.pitstops);
        L.set(tyrev, h.tyres);
        L.set(stty, h.start_tyres);

        L.set(form, h.on && h.race && h.formation);
        L.set(wide, h.on && h.wide);
        // the banner: its name / logo slots sit straight under an always-shown group, so they change while
        // visible (see the rows below); hidden, every slot is off
        {
            BannerLeaves &x = ban;
            const bool mine = h.on && h.banner == banner_leader;
            hud::Leaves::Atomic group(L);
            L.set(x.on, mine);
            const bool icon = mine && cfg.logos == Config::Logos::tv && h.ban_team >= 0 && has_team(h.ban_team);
            L.set(x.car, mine && !h.demo && h.ban_vid >= 0 && h.ban_vid < kSlots ? h.ban_vid : -1);
            L.set(x.tm, icon ? h.ban_team : -1);
            L.set(x.badge, mine && !icon ? h.ban_badge : std::string());
            L.set(x.bdg, mine && !icon && !h.ban_badge.empty());
            L.set(x.nm, mine && h.demo ? h.ban_name : std::string());
        }

        // Row change fades: only for a few rows at a time (a start or a restart reshuffles the field: snap) and at
        // most once a second per row
        int changing = 0;
        if (anim)
            for (int i = 0; i < kRows; i++) {
                const Row &r = v.rows[size_t(i)];
                if (r.on && was_on_[i] && shown_vid_[i] != r.vid)
                    changing++;
            }
        const bool fade_now = anim && changing > 0 && changing <= kMaxFades;

        for (int i = 0; i < kRows; i++) {
            const Row &r = v.rows[size_t(i)];
            RowLeaves &x = rows[i];
            hud::Leaves::Atomic group(L);
            L.set(x.on, r.on);
            if (r.on && was_on_[i] && shown_vid_[i] != r.vid && (hold_until_[i] > 0 ||
                                                                 (fade_now && now >= next_fade_[i]))) {
                // a new car on this row: fade the old content out, keep it for kFadeHold, then swap and fade in
                if (hold_until_[i] <= 0) {
                    L.set(x.fx, false);
                    hold_until_[i] = now + kFadeHold;
                    continue;
                }
                if (now < hold_until_[i])
                    continue;
                next_fade_[i] = now + 1.0;
            }
            hold_until_[i] = 0;
            was_on_[i] = r.on;
            if (r.on) {
                shown_vid_[i] = r.vid;
                L.set(x.fx, true); // always back on, also when Animations was switched off during a fade
            }
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
            // no badge from the game (My Team, a custom team): a chip in the team's own colour
            L.set(x.nob, !icon && r.badge.empty() && !h.demo);
            L.set(x.car, r.vid >= 0 && r.vid < kSlots && !h.demo ? r.vid : -1);
        }
        L.flush();
    }

    hud::Leaves L{kPrefix};
    hud::BoolLeaf show, on, timed, race, demo, abbr, colbar;
    hud::OneHot bg;
    hud::TextLeaf sess, lap, dlaps, dclk;
    hud::BoolLeaf c_chq, c_red, c_sc, c_vsc, c_vscend, c_scending, c_clear, c_yel;
    hud::TextLeaf ysec;
    hud::TextLeaf clock;
    hud::OneHot ld;
    hud::BoolLeaf gained, sprint, pstops, form, tyrev, stty, wide;
    hud::Leaves::Rows<RowLeaves> rows;
    struct BannerLeaves {
        hud::BoolLeaf on, bdg;
        hud::OneHot car, tm;
        hud::TextLeaf badge, nm;
    };
    BannerLeaves ban; // NEW RACE LEADER

    static constexpr double kFadeHold = 0.16; // the row's fade out (generic_fade_on) before the swap
    static constexpr int kMaxFades = 4;       // more rows changing at once: no fade (a start, a restart)

private:
    std::array<int, kRows> shown_vid_ = filled(-1);
    std::array<bool, kRows> was_on_{};
    std::array<double, kRows> hold_until_{};
    std::array<double, kRows> next_fade_{};
    static std::array<int, kRows> filled(int v)
    {
        std::array<int, kRows> a{};
        a.fill(v);
        return a;
    }

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
