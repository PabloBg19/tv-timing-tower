// SPDX-License-Identifier: MIT
// TV Timing Tower - the HUD leaves (view_model_data.hud_tvt_*) and the View -> leaves mapping.
// The record in tools/hudgen.py binds exactly these names; native/test checks the two stay in step.
#pragma once
#include <array>
#include <string>
#include <vector>

#include <swofty/hud_leaves.hpp>

#include "model.hpp"

namespace tvt {

namespace hud = swofty::hud;

inline constexpr const char *kPrefix = "view_model_data.hud_tvt_";
inline constexpr int kSlideBits = 5;

struct RowLeaves {
    hud::BoolLeaf on, sep, me, p1, ko, kol, dim, up, dn;
    hud::TextLeaf pos, gap;
    hud::BoolLeaf gpu, gpg, glb, gip, gsp, gch, gq;
    hud::BoolLeaf tS, tM, tH, tI, tW;
    hud::BoolLeaf sS, sM, sH, sI, sW; // STARTING TYRES: the compound's name
    hud::TextLeaf tage;
    hud::BoolLeaf tag, pit, pen;
    hud::TextLeaf pent;
    hud::BoolLeaf fl, chq, gu, gd, g0, fx, nob;
    hud::TextLeaf nm, badge, tn;
    hud::BoolLeaf bdg;
    hud::OneHot tm, car;
    hud::BoolLeaf sd[5], su[5]; // the row's slides (hudgen SLIDE_BITS): 1, 2, 4, 8, 16 rows down / up
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
        champ = L.boolean("champ");
        cons = L.boolean("cons");

        form = L.boolean("form");
        wide = L.boolean("wide");
        for (int b = 0; b < 2; b++) {
            BannerLeaves &x = ban[b];
            std::string k = b == 0 ? "bnl" : "bnf";
            x.on = L.boolean(k);
            x.car = L.one_hot(k + "_c", kSlots, 2);
            x.tm = L.one_hot(k + "_tm", kTeams, 2);
            x.badge = L.text(k + "_badge", 96);
            x.bdg = L.boolean(k + "_bdg");
            x.nm = L.text(k + "_nm", 24); // demo: the simulated driver's name
            if (b == 1)
                x.time = L.text(k + "_t", 12);
        }
        {
            CardLeaves &x = qc;
            x.on = L.boolean("qc");
            x.car = L.one_hot("qc_c", kSlots, 2);
            x.tm = L.one_hot("qc_tm", kTeams, 2);
            x.badge = L.text("qc_badge", 96);
            x.bdg = L.boolean("qc_bdg");
            x.nm = L.text("qc_nm", 24);
            x.pos = L.text("qc_pos", 4);
            x.ty = L.one_hot("qc_ty", 5, 0);
            x.big = L.text("qc_t", 12);
            x.bc = L.one_hot("qc_bc", 5, 0); // the big figure's colour: white, grey, green, yellow, purple
            x.tt = L.text("qc_tt", 12);
            x.tcar = L.one_hot("qc_n", kSlots, 2);
            x.tnm = L.text("qc_tnm", 24);
            for (int i = 0; i < 3; i++) {
                x.sec[i] = L.one_hot("qc_s" + std::to_string(i + 1) + "_", 5, 0);
                x.lng[i] = L.boolean("qc_l" + std::to_string(i + 1)); // "SECTOR n" in place of "Sn"
            }
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
            r.gch = l.boolean("gch");
            r.gsp = l.boolean("gsp");
            r.gq = l.boolean("gq");
            r.tS = l.boolean("tS");
            r.tM = l.boolean("tM");
            r.tH = l.boolean("tH");
            r.tI = l.boolean("tI");
            r.tW = l.boolean("tW");
            r.sS = l.boolean("sS");
            r.sM = l.boolean("sM");
            r.sH = l.boolean("sH");
            r.sI = l.boolean("sI");
            r.sW = l.boolean("sW");
            r.tage = l.text("tage", 4);
            r.tag = l.boolean("tag");
            r.pit = l.boolean("pit");
            r.pen = l.boolean("pen");
            r.pent = l.text("pent", 6);
            r.fl = l.boolean("fl");
            r.chq = l.boolean("chq");
            r.nm = l.text("nm", 24);
            r.tn = l.text("tn", 20); // the constructors' table: the team's name
            r.badge = l.text("badge", 96);
            r.bdg = l.boolean("bdg");
            r.gu = l.boolean("gu");
            r.gd = l.boolean("gd");
            r.g0 = l.boolean("g0");
            r.fx = l.boolean("fx");
            r.nob = l.boolean("nob");
            for (int b = 0; b < kSlideBits; b++) {
                r.sd[b] = l.boolean("sd" + std::to_string(b));
                r.su[b] = l.boolean("su" + std::to_string(b));
            }
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
        L.set(champ, h.on && (h.champ || h.cons));
        L.set(cons, h.on && h.cons);

        L.set(form, h.on && h.race && h.formation);
        L.set(wide, h.on && h.wide);
        // the banner: its name / logo slots sit straight under an always-shown group, so they change while
        // visible (see the rows below); hidden, every slot is off
        for (int b = 0; b < 2; b++) {
            BannerLeaves &x = ban[b];
            const bool mine = h.on && h.banner == (b == 0 ? banner_leader : banner_fastest);
            hud::Leaves::Atomic group(L);
            L.set(x.on, mine);
            const bool icon = mine && cfg.logos == Config::Logos::tv && h.ban_team >= 0 && has_team(h.ban_team);
            L.set(x.car, mine && !h.demo && h.ban_vid >= 0 && h.ban_vid < kSlots ? h.ban_vid : -1);
            L.set(x.tm, icon ? h.ban_team : -1);
            L.set(x.badge, mine && !icon ? h.ban_badge : std::string());
            L.set(x.bdg, mine && !icon && !h.ban_badge.empty());
            L.set(x.nm, mine && h.demo ? h.ban_name : std::string());
            if (b == 1)
                L.set(x.time, mine ? h.ban_time : std::string());
        }

        // the qualifying card: as the banner, its slots sit straight under an always-shown group and change while
        // the card shows; hidden, every slot is off
        {
            CardLeaves &x = qc;
            const QCard &k = v.card;
            const bool on = h.on && k.on;
            hud::Leaves::Atomic group(L);
            L.set(x.on, on);
            const bool icon = on && cfg.logos == Config::Logos::tv && k.team >= 0 && has_team(k.team);
            L.set(x.car, on && !h.demo && k.vid >= 0 && k.vid < kSlots ? k.vid : -1);
            L.set(x.tm, icon ? k.team : -1);
            L.set(x.badge, on && !icon ? k.badge : std::string());
            L.set(x.bdg, on && !icon && !k.badge.empty());
            L.set(x.nm, on && h.demo ? k.name : std::string());
            L.set(x.pos, on ? k.pos : std::string());
            static const char kTy[] = "SMHIW";
            int ty = -1;
            for (int i = 0; i < 5; i++)
                if (on && k.tyre == kTy[i])
                    ty = i;
            L.set(x.ty, ty);
            L.set(x.big, on ? k.big : std::string());
            L.set(x.bc, on ? k.big_col : -1);
            L.set(x.tt, on ? k.target : std::string());
            const bool tgt = on && !k.target.empty();
            L.set(x.tcar, tgt && !h.demo && k.target_vid >= 0 && k.target_vid < kSlots ? k.target_vid : -1);
            L.set(x.tnm, tgt && h.demo ? k.target_name : std::string());
            for (int i = 0; i < 3; i++) {
                L.set(x.sec[i], on ? k.sec[i] : -1);
                L.set(x.lng[i], on && k.sec_long[i]);
            }
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
        // where each car was shown before this change (its slide starts there)
        std::array<int, 64> was_at;
        was_at.fill(-1);
        for (int i = 0; i < kRows; i++)
            if (was_on_[i] && shown_vid_[i] >= 0 && shown_vid_[i] < int(was_at.size()))
                was_at[size_t(shown_vid_[i])] = i;

        for (int i = 0; i < kRows; i++) {
            const Row &r = v.rows[size_t(i)];
            RowLeaves &x = rows[i];
            hud::Leaves::Atomic group(L);
            L.set(x.on, r.on);
            if (r.on && was_on_[i] && shown_vid_[i] != r.vid && (hold_until_[i] > 0 ||
                                                                 (fade_now && now >= next_fade_[i]))) {
                // a new car on this row: fade the old content out while the slides move it to where the new car was
                // shown (unseen), keep it for the slide's length, then swap the car and turn the slides off: the new
                // content glides from there to its place as it fades in. A car from off the tower only fades.
                if (hold_until_[i] <= 0) {
                    L.set(x.fx, false);
                    const int from = r.vid >= 0 && r.vid < int(was_at.size()) ? was_at[size_t(r.vid)] : -1;
                    const int d = from >= 0 ? from - i : 0; // > 0: the car comes up from below
                    const int n = d < 0 ? -d : d;
                    for (int b = 0; b < kSlideBits; b++) {
                        L.set(x.sd[b], d > 0 && (n >> b & 1));
                        L.set(x.su[b], d < 0 && (n >> b & 1));
                    }
                    hold_until_[i] = now + (n ? kSlideHold : kFadeHold);
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
                for (int b = 0; b < kSlideBits; b++) { // the slides back to rest: the content glides home
                    L.set(x.sd[b], false);
                    L.set(x.su[b], false);
                }
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
            L.set(x.gch, r.gap_champ);
            L.set(x.gsp, r.gap_purple || r.gap_green || r.gap_label || r.gap_inpit || r.gap_champ);
            // a lap time (or a gap over a minute) in the narrow column: practice, qualifying, the fastest lap flash
            L.set(x.gq, h.timed || r.gap.find(':') != std::string::npos);
            L.set(x.tS, r.tyre == 'S');
            L.set(x.tM, r.tyre == 'M');
            L.set(x.tH, r.tyre == 'H');
            L.set(x.tI, r.tyre == 'I');
            L.set(x.tW, r.tyre == 'W');
            L.set(x.sS, r.start_tyre == 'S');
            L.set(x.sM, r.start_tyre == 'M');
            L.set(x.sH, r.start_tyre == 'H');
            L.set(x.sI, r.start_tyre == 'I');
            L.set(x.sW, r.start_tyre == 'W');
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
            L.set(x.tn, r.team_label);
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
    hud::BoolLeaf show, on, timed, race, demo, abbr;
    hud::OneHot bg;
    hud::TextLeaf sess, lap, dlaps, dclk;
    hud::BoolLeaf c_chq, c_red, c_sc, c_vsc, c_vscend, c_scending, c_clear, c_yel;
    hud::TextLeaf ysec;
    hud::TextLeaf clock;
    hud::OneHot ld;
    hud::BoolLeaf gained, sprint, pstops, form, tyrev, stty, wide, champ, cons;
    hud::Leaves::Rows<RowLeaves> rows;
    struct BannerLeaves {
        hud::BoolLeaf on, bdg;
        hud::OneHot car, tm;
        hud::TextLeaf badge, nm, time;
    };
    BannerLeaves ban[2]; // 0 NEW RACE LEADER, 1 FASTEST LAP
    struct CardLeaves {
        hud::BoolLeaf on, bdg;
        hud::BoolLeaf lng[3];
        hud::OneHot car, tm, ty, bc, tcar;
        hud::OneHot sec[3];
        hud::TextLeaf badge, nm, pos, big, tt, tnm;
    };
    CardLeaves qc; // the qualifying card

    static constexpr double kFadeHold = 0.16; // the row's fade out (generic_fade_on) before the swap
    static constexpr int kMaxFades = 12;      // more rows changing at once: no animation (a start, a restart)
    static constexpr double kSlideHold = 0.36; // a slide's length (hudgen SLIDE_S) and a little

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
