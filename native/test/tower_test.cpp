// SPDX-License-Identifier: MIT
// Host tests: the model, the lap clock, the live adapter (over Swofty's fake live state) and the leaves (over
// Swofty's fake HUD core), and that every leaf the record binds is one the DLL declares.
//   tools/build.py test
#include <cassert>
#include <cstdio>
#include <fstream>
#include <regex>
#include <chrono>
#include <set>
#include <sstream>
#include <string>

#include <swofty/fake_hud.hpp>
#include <swofty/fake_live.hpp>

#include "../src/demo.hpp"
#include "../src/lap_clock.hpp"
#include "../src/laptimes.hpp"
#include "../src/qcard.hpp"
#include "../src/leaves.hpp"
#include "../src/model.hpp"
#include "../src/source_live.hpp"
#include "../src/standings.hpp"
#include <map>

using namespace tvt;

static int failures = 0;
#define CHECK(c)                                                                                                       \
    do {                                                                                                               \
        if (!(c)) {                                                                                                    \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c);                                                   \
            failures++;                                                                                                \
        }                                                                                                              \
    } while (0)

static void test_format()
{
    CHECK(fmt_gap_race(999) == "+0.9");   // truncated, as the 2026 broadcast
    CHECK(fmt_gap_race(12345) == "+12.3");
    CHECK(fmt_gap_race(65333) == "+1:05.3");
    CHECK(fmt_gap_ms(234) == "+0.234");
    CHECK(fmt_lap(87345) == "1:27.345");
    CHECK(fmt_laps_down(1) == "+1 Lap");
    CHECK(fmt_laps_down(3) == "+3 Laps");
    CHECK(fmt_clock(2036) == "33:56");
    CHECK(std::string(session_label(6)) == "Q2");
    CHECK(std::string(session_label(11)) == "SQ2");
    CHECK(knockout_cut(1, 22) == 16 && knockout_cut(1, 20) == 15 && knockout_cut(2, 22) == 10);
    CHECK(team_index("ferrari_2026") == 1);
    CHECK(team_index("red_bull_2026") == 2);
    CHECK(team_index("racing_bulls_2026") == 7);
    // other seasons keep the game's own badge (the broadcast icons are 2026's): no Kick icon on an old Alfa Romeo
    CHECK(team_index("rb_2025") == -1);
    CHECK(team_index("kick_sauber_2025") == -1);
    CHECK(team_index("alfa_romeo") == -1 && team_index("alfa_romeo_23") == -1 && team_index("ferrari") == -1);
    CHECK(team_index("audi_2026") == 10);
    CHECK(team_index("cadillac_2026") == 11);
    CHECK(team_index("aston_martin_2026") == 4);
    // the 2026 keys the game really uses (in-game logs)
    CHECK(team_index("alphatauri_26") == 7);
    CHECK(team_index("alfa_romeo_26") == 10);
    CHECK(team_index("gm_cadillac_26") == 11);
    CHECK(team_index("red_bull_26") == 2);
    CHECK(team_index("") == -1);
}

static void test_race_rows()
{
    Tower t;
    Config cfg;
    cfg.gap = Config::Gap::interval;
    Snapshot s = demo::make(demo::race, 1.0);
    const View &v = t.update(s, cfg, 1.0);
    CHECK(v.h.on && v.h.race && !v.h.timed);
    CHECK(v.h.lap == "22");
    CHECK(v.h.laps_total == "57");
    CHECK(v.shown == 22);
    CHECK(v.rows[0].p1 && v.rows[0].gap == "Interval" && v.rows[0].gap_label);
    CHECK(v.h.abbr && !v.h.bare); // gaps shown: codes
    CHECK(v.rows[1].gap.rfind("+", 0) == 0);
    CHECK(v.rows[19].gap == "+1 Lap" || v.rows[19].gap.rfind("+", 0) == 0);
    CHECK(v.rows[8].pen && v.rows[8].pen_text == "+5s");
    // a pass: P6 and P7 swap after 6 s and get arrows
    t.update(demo::make(demo::race, 7.0), cfg, 7.0);
    const View &v2 = t.view();
    CHECK(v2.rows[5].up && v2.rows[6].dn);
    t.update(demo::make(demo::race, 11.5), cfg, 11.5);
    CHECK(!t.view().rows[5].up);
    // the race's fastest lap: the demo shows it (as the game's own times do); the addon's clock alone does not
    CHECK(t.view().rows[1].fl);
    {
        Tower tn;
        Snapshot a = demo::make(demo::race, 7.0), b = demo::make(demo::race, 11.5);
        a.s.demo = b.s.demo = false;
        tn.update(a, cfg, 7.0);
        tn.update(b, cfg, 11.5);
        CHECK(!tn.view().rows[1].fl);
    }
    {
        Config fc = cfg;
        fc.race_fastest = true;
        Tower tf;
        tf.update(demo::make(demo::race, 7.0), fc, 7.0);
        tf.update(demo::make(demo::race, 11.5), fc, 11.5);
        CHECK(tf.view().rows[1].fl);
    }
    CHECK(t.view().h.card == card_yellow && t.view().h.ysec == "SECTOR 2");
    CHECK(t.view().rows[21].gap == "Out" && t.view().rows[21].out);
}

static void test_leader_mode_and_rows()
{
    Tower t;
    Config cfg;
    cfg.gap = Config::Gap::leader;
    const View &v = t.update(demo::make(demo::race, 1.0), cfg, 1.0);
    CHECK(v.h.lead && v.rows[0].gap == "Leader");
    cfg.rows = Config::Rows::top10;
    const View &v2 = t.update(demo::make(demo::race, 1.0), cfg, 1.1);
    CHECK(v2.shown == 10); // the player (ALBON) is in the top 10 in this scenario
    cfg.rows = Config::Rows::compact;
    const View &v3 = t.update(demo::make(demo::race, 1.0), cfg, 1.2);
    CHECK(v3.shown == 6 && v3.rows[3].sep);
    // auto mode flips to the leader when the leader starts a lap that is a multiple of 5
    Tower t2;
    Config a;
    Snapshot s = demo::make(demo::race, 0.0);
    for (auto &c : s.cars)
        c.lap = 23;
    t2.update(s, a, 0.0);
    for (auto &c : s.cars)
        c.lap = 25;
    CHECK(t2.update(s, a, 1.0).h.lead);
    CHECK(!t2.update(s, a, 10.0).h.lead);
}

static void test_quali()
{
    // off by default (0.12): no times in practice and qualifying, full surnames, no flashes
    {
        Tower t0;
        Config off;
        Snapshot q0 = demo::make(demo::quali_q1, 0.5);
        q0.s.game_times = false; // the addon's own clock only
        const View &v0 = t0.update(q0, off, 0.5);
        bool any = false;
        for (int i = 0; i < v0.shown; i++)
            any = any || !v0.rows[i].gap.empty();
        CHECK(!any && !v0.h.abbr && v0.rows[16].ko);
        Tower tp0;
        Snapshot pr0 = demo::make(demo::practice, 0.5);
        pr0.s.game_times = false;
        const View &p0 = tp0.update(pr0, off, 0.5);
        any = false;
        for (int i = 0; i < p0.shown; i++)
            any = any || !p0.rows[i].gap.empty();
        CHECK(!any);
    }
    Tower t;
    Config cfg;
    cfg.timed_times = true; // the timing logic, kept for when the core gives the game's own lap times
    const View &v = t.update(demo::make(demo::quali_q1, 0.5), cfg, 0.5);
    CHECK(v.h.timed && v.h.sess == "Q1");
    CHECK(v.rows[0].gap.find(':') != std::string::npos); // leader shows the lap time
    CHECK(v.rows[1].gap.rfind("+0.", 0) == 0);
    int ko = 0, lines = 0;
    for (const Row &r : v.rows) {
        ko += r.ko;
        lines += r.ko_line;
    }
    CHECK(ko == 6 && lines == 1);
    CHECK(v.rows[16].ko_line);
    // Q2: the six knocked out in Q1 leave the tower (as on TV); the knockout zone is P11-P16
    Tower t2;
    const View &q2 = t2.update(demo::make(demo::quali_q2, 0.5), cfg, 0.5);
    CHECK(q2.shown == 16 && !q2.rows[16].on);
    CHECK(q2.rows[10].ko_line && q2.rows[15].ko && q2.rows[15].pos == "16");
    // top 10 + me in Q2 with the player knocked out: ten rows, no player
    Config top = cfg;
    top.rows = Config::Rows::top10;
    Tower t2b;
    CHECK(t2b.update(demo::make(demo::quali_q2, 0.5), top, 0.5).shown == 10);
    // personal best flash (green) for the player after 5 s
    Tower t3;
    t3.update(demo::make(demo::quali_q1, 4.0), cfg, 4.0);
    const View &q = t3.update(demo::make(demo::quali_q1, 5.5), cfg, 5.5);
    bool green = false;
    for (const Row &r : q.rows)
        green |= r.gap_green || r.gap_purple;
    CHECK(green);
}

// A plain classified snapshot: n cars at positions 1..n, type `type`, racers `racers` (as the live state may
// report fewer once a knockout round has reduced who's still classified).
static Snapshot make_timed(int type, int n, int racers)
{
    Snapshot s;
    s.s.type = type;
    s.s.racers = racers;
    s.s.track_m = 4500.f;
    for (int i = 0; i < n; i++) {
        CarIn c;
        c.vid = uint32_t(i);
        c.pos = i + 1;
        c.status = 2;
        c.best_ms = 78000 + i * 150;
        s.cars.push_back(c);
    }
    return s;
}

static void test_field_persists_across_quali_segments()
{
    Config cfg;
    Tower t;
    t.update(make_timed(5, 22, 22), cfg, 0.0); // Q1: 22 classified, cut at 16 (10 + (22-10)/2)
    // Q2: the live state may now report only the 16 who went through (not the original 22); the Q1 cut used to
    // decide who left the tower must still come from the field Q1 started with, not this smaller count
    const View &q2 = t.update(make_timed(6, 16, 16), cfg, 1.0);
    CHECK(q2.shown == 16);
    for (int i = 0; i < q2.shown; i++)
        CHECK(!q2.rows[i].elim);
    // the same for sprint qualifying (SQ1 -> SQ2, types 10 -> 11)
    Tower t2;
    t2.update(make_timed(10, 20, 20), cfg, 0.0); // a 20-car SQ1: cut at 15 (10 + (20-10)/2)
    const View &sq2 = t2.update(make_timed(11, 15, 15), cfg, 1.0);
    CHECK(sq2.shown == 15);
    // Q2 seen without Q1 (Q1 skipped, or the game started at Q2) and only Q2's 16 listed: all 16 stay, and the
    // knockout zone is 11-16
    Tower t3;
    t3.update(make_timed(1, 22, 22), cfg, 0.0); // practice first: a different session kind resets the field
    const View &q2d = t3.update(make_timed(6, 16, 16), cfg, 1.0);
    CHECK(q2d.shown == 16);
    CHECK(q2d.rows[10].ko && q2d.rows[10].ko_line && q2d.rows[15].ko && !q2d.rows[15].elim && !q2d.rows[9].ko);
    // the same with the whole field listed: Q1's 6 leave the tower
    Tower t4;
    CHECK(t4.update(make_timed(6, 22, 22), cfg, 0.0).shown == 16);
    // a 24-car My Team Q2 with only its 17 listed
    Tower t5;
    CHECK(t5.update(make_timed(6, 17, 17), cfg, 0.0).shown == 17);
    // Q3 with only its 10, or with Q2's 16 listed
    Tower t6;
    CHECK(t6.update(make_timed(7, 10, 10), cfg, 0.0).shown == 10);
    Tower t7;
    CHECK(t7.update(make_timed(7, 16, 16), cfg, 0.0).shown == 10);
}

static void test_yellow_texts()
{
    Config cfg;
    Snapshot s = demo::make(demo::race, 1.0);
    s.s.yellow = 0x7; // sectors 1, 2 and 3: "SECTORS 1, 2 & 3" ran past the block's edge
    Tower t;
    const View &v = t.update(s, cfg, 1.0);
    CHECK(v.h.card == card_yellow && v.h.ysec == "ALL SECTORS");
    s.s.yellow = 0x5;
    Tower t2;
    const View &v2 = t2.update(s, cfg, 1.0);
    CHECK(v2.h.ysec == "SECTORS 1 & 3");
}

static void test_banners()
{
    Config cfg;
    {
        // demo "banners": P2 takes the lead at 0.5 s but stays within 0.3 s until 3 s, then pulls 0.9 s clear:
        // NEW RACE LEADER from then on for 6 s (not while they fight); a first fastest lap at 3.5 s waits for it and
        // is replaced by the one beating it at 7.5 s, which shows after the leader's banner with its time
        Tower t;
        int leader_at = -1, shown = 0, fastest_at = -1;
        std::string ftime;
        std::string lname;
        int lvid = -1;
        for (int i = 0; i <= 140; i++) {
            double now = i * 0.1;
            const View &v = t.update(demo::make(demo::banners, now), cfg, now);
            if (v.h.banner == banner_leader && leader_at < 0) {
                leader_at = i;
                lvid = v.h.ban_vid;
                lname = v.h.ban_name;
            }
            if (v.h.banner == banner_fastest && fastest_at < 0) {
                fastest_at = i;
                ftime = v.h.ban_time;
            }
            shown += v.h.banner == banner_leader;
        }
        Snapshot s1 = demo::make(demo::banners, 1.0);
        CHECK(leader_at >= 30 && leader_at <= 31);            // once 0.6 s clear of P2, not at the pass (0.5 s)
        CHECK(lvid == int(s1.cars[0].vid) && !lname.empty());
        CHECK(shown >= 58 && shown <= 62);                    // one banner of 6 s
        Snapshot s8 = demo::make(demo::banners, 8.0);
        CHECK(fastest_at >= 95 && fastest_at <= 97 && ftime == fmt_lap(s8.s.fastest_ms));
    }
    {
        // the fastest lap from the addon's own clock (no game times): no banner, no row icon
        Tower t;
        int any = 0, icon = 0;
        for (int i = 0; i <= 140; i++) {
            Snapshot s = demo::make(demo::banners, i * 0.1);
            s.s.demo = false;
            const View &v = t.update(s, cfg, i * 0.1);
            any += v.h.banner == banner_fastest;
            for (const Row &r : v.rows)
                icon += r.fl;
        }
        CHECK(any == 0 && icon == 0);
        // the game's times: both, the first fastest lap too when the race is young
        Tower g;
        any = icon = 0;
        for (int i = 0; i <= 140; i++) {
            Snapshot s = demo::make(demo::banners, i * 0.1);
            s.s.demo = false;
            s.s.game_times = true;
            for (CarIn &c : s.cars)
                c.lap = 2;
            const View &v = g.update(s, cfg, i * 0.1);
            any += v.h.banner == banner_fastest;
            for (const Row &r : v.rows)
                icon += r.fl;
        }
        CHECK(any > 0 && icon > 0);
        // a race joined late: the fastest lap already set is taken in silently, a later one is announced
        Tower late;
        any = 0;
        for (int i = 0; i <= 70; i++) {
            Snapshot s = demo::make(demo::banners, i * 0.1);
            s.s.demo = false;
            s.s.game_times = true;
            for (CarIn &c : s.cars)
                c.lap = 20;
            s.cars[1].gap_ahead_ms = 350; // no leader banner either
            any += late.update(s, cfg, i * 0.1).h.banner == banner_fastest;
        }
        CHECK(any == 0);
    }
    {
        // a lead that changes hands but never gets 0.6 s clear: the banner all the same, 3 s later than a clear one
        // (kLeaderHoldSeconds 4 in front against 1)
        Tower t, tc;
        double close_at = -1, clear_at = -1;
        for (int i = 0; i <= 140; i++) {
            Snapshot s = demo::make(demo::banners, i * 0.1);
            Snapshot c = s;
            s.cars[1].gap_ahead_ms = 350;
            c.cars[1].gap_ahead_ms = 900;
            if (t.update(s, cfg, i * 0.1).h.banner == banner_leader && close_at < 0)
                close_at = i * 0.1;
            if (tc.update(c, cfg, i * 0.1).h.banner == banner_leader && clear_at < 0)
                clear_at = i * 0.1;
        }
        CHECK(clear_at >= 0 && close_at >= clear_at + 3.0);
    }
    {
        // switched off: none
        Config off = cfg;
        off.ban_leader = false;
        off.ban_fastest = false;
        Tower t;
        int any = 0;
        for (int i = 0; i <= 140; i++)
            any += t.update(demo::make(demo::banners, i * 0.1), off, i * 0.1).h.banner != banner_none;
        CHECK(any == 0);
    }
    {
        // the start (the grid, lap 1) and other sessions: none
        Tower t;
        int any = 0;
        for (int i = 0; i <= 60; i++) {
            Snapshot g = demo::make(demo::grid, i * 0.1);
            if (i > 20)
                std::swap(g.cars[0], g.cars[1]);
            any += t.update(g, cfg, i * 0.1).h.banner != banner_none;
        }
        Tower q;
        for (int i = 0; i <= 60; i++)
            any += q.update(demo::make(demo::quali_q1, i * 0.1), cfg, i * 0.1).h.banner != banner_none;
        CHECK(any == 0);
    }
}

static void test_cards()
{
    Config cfg;
    {
        Tower t;
        CHECK(t.update(demo::make(demo::safety_car, 1), cfg, 1).h.card == card_sc);
        CHECK(t.update(demo::make(demo::safety_car, 4), cfg, 4).h.card == card_sc_ending); // SC heading in: SC ENDING
        CHECK(t.update(demo::make(demo::safety_car, 6), cfg, 6).h.card == card_sc_ending); // "SC" / "ENDING"
        CHECK(t.update(demo::make(demo::safety_car, 10), cfg, 10).h.card == card_clear);
        CHECK(t.update(demo::make(demo::safety_car, 14), cfg, 14).h.card == card_none);
    }
    {
        Tower t;
        CHECK(t.update(demo::make(demo::vsc, 1), cfg, 1).h.card == card_vsc);
        CHECK(t.update(demo::make(demo::vsc, 5.5), cfg, 5.5).h.card == card_vsc_end);
        CHECK(t.update(demo::make(demo::vsc, 10), cfg, 10).h.card == card_clear);
        CHECK(t.update(demo::make(demo::vsc, 14), cfg, 14).h.card == card_none);
    }
    {
        Tower t;
        const View &v = t.update(demo::make(demo::red_flag, 1), cfg, 1);
        CHECK(v.h.card == card_red && v.rows[3].gap.empty() && v.h.bare && !v.h.abbr);
    }
    {
        // the game's red flag (not the demo): believed after 2 s in a row once the race is under way
        Tower t;
        Snapshot r = demo::make(demo::red_flag, 1);
        r.s.demo = false;
        for (CarIn &c : r.cars)
            c.lap = 5;
        CHECK(t.update(r, cfg, 1.0).h.card != card_red);
        CHECK(t.update(r, cfg, 2.5).h.card != card_red);
        const View &v = t.update(r, cfg, 3.1);
        CHECK(v.h.card == card_red && v.h.bare);
        // ...and one already up on the grid is ignored (a user: RED FLAG as the race started) until it drops
        Tower g;
        Snapshot s = demo::make(demo::red_flag, 1);
        s.s.demo = false;
        for (CarIn &c : s.cars) {
            c.lap = 0;
            c.have_total = true;
            c.total_m = 0.f;
            c.speed_kmh = 0.f;
        }
        CHECK(g.update(s, cfg, 1.0).h.card != card_red);
        for (CarIn &c : s.cars) {
            c.lap = 2;
            c.speed_kmh = 250.f;
        }
        CHECK(g.update(s, cfg, 10.0).h.card != card_red && !g.update(s, cfg, 20.0).h.bare);
        s.s.red = false;
        g.update(s, cfg, 21.0);
        s.s.red = true;
        g.update(s, cfg, 22.0);
        CHECK(g.update(s, cfg, 24.5).h.card == card_red);
    }
    {
        Tower t;
        const View &v = t.update(demo::make(demo::finish, 5), cfg, 5);
        CHECK(v.h.card == card_none && v.rows[0].chq && v.rows[21].gap == "Out" && v.rows[20].gap == "DSQ");
    }
    {
        Config off;
        off.en_race = false;
        Tower t;
        CHECK(!t.update(demo::make(demo::race, 1), off, 1).h.on);
    }
}

static void test_lap_clock()
{
    LapClock c;
    const float len = 5000.f;
    // one car at 50 m/s: a lap takes 100 s. 60 fps.
    double dt = 1.0 / 60.0;
    float d = 4000.f;
    for (int i = 0; i < 60 * 330; i++) {
        d += 50.f * float(dt);
        int lap = int(d / len);
        float m = d - float(lap) * len;
        c.step(dt, len, {LapSample{0, lap, m, false, 0}});
        if (i == 60 * 150) { // a 20 s pause in the middle of the second lap
            for (int k = 0; k < 60 * 20; k++)
                c.step(dt, len, {LapSample{0, lap, m, false, 0}});
        }
    }
    CHECK(c.best_ms(0) > 99950 && c.best_ms(0) < 100050);
    CHECK(c.fastest_vid() == -1); // the session's fastest needs 8 timed laps to judge against (see below)
    // the fastest lap: 10 cars at ~100 s a lap; one car's bogus 90 s "lap" (10 % under the field) is not the fastest
    {
        LapClock fl;
        std::vector<float> dd(10);
        for (int k = 0; k < 10; k++)
            dd[size_t(k)] = 4000.f - float(k) * 30.f;
        for (int i = 0; i < 60 * 330; i++) {
            std::vector<LapSample> v;
            for (int k = 0; k < 10; k++) {
                float sp = 50.f + float(k) * 0.05f;   // 99.0 .. 100 s laps
                if (k == 9 && i > 60 * 210 && i < 60 * 225)
                    sp = 400.f;                        // a jump forward: one short "lap"
                dd[size_t(k)] += sp * float(dt);
                int l = int(dd[size_t(k)] / len);
                v.push_back(LapSample{uint32_t(k), l, dd[size_t(k)] - float(l) * len, false, 0});
            }
            fl.step(dt, len, v);
        }
        CHECK(fl.fastest_ms() > 98000 && fl.fastest_ms() < 100100);
        CHECK(fl.best_ms(9) > 0 && fl.best_ms(9) < 95000); // the bogus lap is the car's own, not the session's
    }
    // the lap distance wraps at the line a few frames before the game's counter goes up (or after): still timed,
    // from the wrap, and not taken for the car going back
    for (int lag : {-12, 12}) {
        LapClock lw;
        float dw = 4000.f;
        for (int i = 0; i < 60 * 330; i++) {
            dw += 50.f * float(dt);
            float m = std::fmod(dw, len);
            int lap = int(dw / len);
            // the counter moves `lag` frames late (lag > 0) or early (lag < 0) against the distance
            int lap_c = int((dw - 50.f * float(dt) * float(lag)) / len);
            lw.step(dt, len, {LapSample{0, lag > 0 ? lap_c : lap_c, m, false, 0}});
            (void)lap;
        }
        if (!(lw.best_ms(0) > 99950 && lw.best_ms(0) < 100050))
            std::printf("lag %d best %d timed %d resets %d wraps %d first %d skips %d\n", lag, lw.best_ms(0),
                        lw.stats().timed, lw.stats().resets, lw.stats().wraps, lw.stats().first, lw.stats().skips);
        CHECK(lw.best_ms(0) > 99950 && lw.best_ms(0) < 100050);
        CHECK(lw.stats().timed >= 2 && lw.stats().resets == 0);
    }
    // qualifying / practice: the game's lap counter never moves (seen in game: 10 wraps, 0 counter steps); the lap
    // distance wrapping at the line is the crossing. An out lap from the pits is not timed, the laps after it are.
    {
        LapClock qc;
        float dq = 3800.f; // leaving the pit lane near the end of the lap
        for (int i = 0; i < 60 * 330; i++) {
            dq += 50.f * float(dt);
            bool pit = i < 60 * 4;
            qc.step(dt, len, {LapSample{0, 0, std::fmod(dq, len), pit, 0}});
        }
        CHECK(qc.best_ms(0) > 99950 && qc.best_ms(0) < 100050);
        CHECK(qc.stats().timed >= 2 && qc.stats().crossings >= 3 && qc.stats().resets == 0);
    }
    // a race: the counter and the wrap report the same crossing; it is counted once
    {
        LapClock rc;
        float dr = 4000.f;
        for (int i = 0; i < 60 * 330; i++) {
            dr += 50.f * float(dt);
            int lap_c = int((dr - 50.f * float(dt) * 5.f) / len); // the counter 5 frames after the wrap
            rc.step(dt, len, {LapSample{0, lap_c, std::fmod(dr, len), false, 0}});
        }
        CHECK(rc.best_ms(0) > 99950 && rc.best_ms(0) < 100050);
        CHECK(rc.stats().crossings == 4); // 4000 m -> 20500 m: the line at 5, 10, 15 and 20 km, once each
    }
    // a "lap" faster than 75 m/s on average is not a lap (a race once showed a 0:59.0 fastest lap): a car at
    // 100 m/s on a 5000 m track would lap in 50 s; none of it is timed
    {
        LapClock fast;
        float df = 4000.f;
        for (int i = 0; i < 60 * 200; i++) {
            df += 100.f * float(dt);
            int lap = int(df / len);
            fast.step(dt, len, {LapSample{0, lap, df - float(lap) * len, false, 0}});
        }
        CHECK(fast.best_ms(0) < 0 && fast.stats().range > 0);
    }
    // a track-limit warning during a lap voids it
    LapClock w;
    d = 4000.f;
    int warn = 0;
    for (int i = 0; i < 60 * 150; i++) {
        d += 50.f * float(dt);
        int lap = int(d / len);
        float m = d - float(lap) * len;
        if (lap == 1 && m > 2000.f)
            warn = 1;
        w.step(dt, len, {LapSample{0, lap, m, false, warn}});
    }
    CHECK(w.best_ms(0) < 0); // the only lap completed after the first crossing had a warning: void
    // best_at_least
    CHECK(c.best_at_least(0, 0) == c.best_ms(0));
    CHECK(c.best_at_least(0, 200000) == -1);

    // order-coherence fallback (source_live.hpp): our own clock can rank two cars differently from the game by a
    // few ms; a car whose real best lap doesn't fit the floor set by one the game ranks higher must still show
    // its own time (not a blank gap) once best_at_least rejects it
    LapClock oc;
    float oc_d0 = 4000.f, oc_d1 = 4000.f;
    for (int i = 0; i < 60 * 150; i++) { // past the partial joining lap and one full lap each (~120 s)
        oc_d0 += 49.9f * float(dt); // car 0: ~100.2 s/lap
        oc_d1 += 50.1f * float(dt); // car 1: ~99.8 s/lap, genuinely faster
        int l0 = int(oc_d0 / len), l1 = int(oc_d1 / len);
        oc.step(dt, len, {LapSample{0, l0, oc_d0 - float(l0) * len, false, 0},
                          LapSample{1, l1, oc_d1 - float(l1) * len, false, 0}});
    }
    CHECK(oc.best_ms(0) > 0 && oc.best_ms(1) > 0 && oc.best_ms(1) < oc.best_ms(0));
    CHECK(oc.best_at_least(1, oc.best_ms(0)) < 0); // the floor rejects car 1's (faster) time
    int fallback = oc.best_at_least(1, oc.best_ms(0));
    fallback = fallback > 0 ? fallback : oc.best_ms(1); // source_live.hpp's own fallback expression
    CHECK(fallback == oc.best_ms(1) && fallback > 0);

    // practice / qualifying: another car going back to the garage (its distance jumps back) must not void the laps
    // of the others (0.5 voided everyone's lap then, so no time ever showed)
    LapClock q;
    float d0 = 4000.f, d1 = 3000.f;
    for (int i = 0; i < 60 * 260; i++) {
        d0 += 50.f * float(dt);
        d1 += 50.f * float(dt);
        if (i % (60 * 20) == 0)
            d1 -= 1500.f; // car 1 teleports back every 20 s
        int l0 = int(d0 / len), l1 = int(std::max(0.f, d1) / len);
        q.step(dt, len, {LapSample{0, l0, d0 - float(l0) * len, false, 0},
                         LapSample{1, l1, std::max(0.f, d1) - float(l1) * len, false, 0},
                         LapSample{2, 0, 10.f, true, 0}, LapSample{3, 0, 20.f, true, 0}});
    }
    CHECK(q.best_ms(0) > 99950 && q.best_ms(0) < 100050);
    CHECK(q.best_ms(1) < 0); // its own laps are void
    // a flashback (most of the field goes back at once) voids every lap in progress
    LapClock fb;
    float e[4] = {4000.f, 3900.f, 3800.f, 3700.f};
    for (int i = 0; i < 60 * 150; i++) {
        for (float &x : e)
            x += 50.f * float(dt);
        if (i == 60 * 60)
            for (float &x : e)
                x -= 800.f;
        std::vector<LapSample> v;
        for (uint32_t k = 0; k < 4; k++) {
            int l = int(e[k] / len);
            v.push_back(LapSample{k, l, e[k] - float(l) * len, false, 0});
        }
        fb.step(dt, len, v);
    }
    CHECK(fb.best_ms(0) < 0); // the lap with the flashback in it is not timed
}

static void test_session_views()
{
    Config cfg;
    cfg.timed_times = true;
    // qualifying: P1 its time, the others the gap to P1, the knockout zone the gap to the last car through
    Tower t;
    const View &q = t.update(demo::make(demo::quali_q1, 0.5), cfg, 0.5);
    CHECK(q.rows[0].gap.find(':') != std::string::npos);
    const Snapshot qs = demo::make(demo::quali_q1, 0.5);
    int cut_best = qs.cars[15].best_ms, lead_best = qs.cars[0].best_ms;
    CHECK(q.rows[16].ko && q.rows[16].gap == fmt_gap_ms(qs.cars[16].best_ms - cut_best));
    CHECK(!q.rows[14].ko && q.rows[14].gap == fmt_gap_ms(qs.cars[14].best_ms - lead_best));
    // as the broadcast's qualifying tower: "Out Lap" on an out lap, "No Time" with "-" and no tyre without a time
    {
        Tower to;
        Snapshot o = demo::make(demo::quali_q1, 0.5);
        const View &ov = to.update(o, cfg, 0.5);
        int out = 0, none = 0;
        for (int i = 0; i < ov.shown; i++) {
            const Row &r = ov.rows[size_t(i)];
            const CarIn &c = o.cars[size_t(i)];
            if (c.best_ms <= 0) {
                none++;
                CHECK(r.gap.empty() && r.pos == "-" && !r.tyre && !r.p1);
            } else if (c.out_lap && i > 0) {
                out++;
                CHECK(r.gap == "Out Lap" && r.pos != "-");
            }
        }
        CHECK(out == 2 && none == 1);
    }
    // practice: everyone to P1
    Tower tp;
    const View &pv = tp.update(demo::make(demo::practice, 0.5), cfg, 0.5);
    const Snapshot ps = demo::make(demo::practice, 0.5);
    CHECK(pv.rows[3].gap == fmt_gap_ms(ps.cars[3].best_ms - ps.cars[0].best_ms));

    // 24 cars (a career with 12 teams): 24 rows, the knockout cut 10 + 14 / 2 = 17
    Snapshot big = demo::make(demo::race, 1.0);
    for (int k = 22; k < 24; k++) {
        CarIn c = big.cars.back();
        c.vid = uint32_t(k);
        c.pos = k + 1;
        c.badge.clear(); // a team the game draws a rendered badge for (My Team)
        big.cars.push_back(c);
    }
    big.s.racers = 24;
    big.s.demo = false;
    Tower tb;
    const View &bv = tb.update(big, cfg, 1.0);
    CHECK(bv.shown == 24 && bv.rows[23].on && bv.rows[23].vid == 23 && bv.rows[23].badge.empty());
    CHECK(knockout_cut(1, 24) == 17);
    CHECK(bv.h.wide); // 24 rows: slide the tower further up
    // a normal 22-car field (the demo's own race grid) does not trigger it
    Tower tn;
    CHECK(!tn.update(demo::make(demo::race, 1.0), cfg, 1.0).h.wide);

    // pit stops view: fixed mode shows each car's count
    Config pc = cfg;
    pc.gap = Config::Gap::pitstops;
    Snapshot st = demo::make(demo::race, 1.0);
    for (CarIn &c : st.cars)
        c.pit_stops = int(c.vid % 3);
    Tower ts;
    const View &sv = ts.update(st, pc, 1.0);
    CHECK(sv.h.pitstops && sv.rows[2].gap == std::to_string(st.cars[2].vid % 3));
    // a car in the pits always shows IN PIT, even while PIT STOPS or TYRES has the tower (and normally)
    for (Config::Gap mode : {Config::Gap::autoswitch, Config::Gap::pitstops, Config::Gap::tyres}) {
        Snapshot pit = demo::make(demo::race, 1.0);
        pit.cars[5].in_pit = true;
        Config pcfg = cfg;
        pcfg.gap = mode;
        Tower tip;
        const View &iv = tip.update(pit, pcfg, 1.0);
        CHECK(iv.rows[5].gap == "IN PIT" && iv.rows[5].gap_inpit);
    }
    // ... and under the safety car, when the rest of the tower shows the names alone
    {
        Tower tsc;
        const View &scv = tsc.update(demo::make(demo::safety_car, 2.0), cfg, 2.0);
        CHECK(scv.h.bare && scv.rows[4].gap == "IN PIT" && scv.rows[4].gap_inpit && scv.rows[4].tyre == 0);
        CHECK(scv.rows[5].gap.empty() && !scv.rows[5].gap_inpit);
    }
    // starting tyres: on the grid every car stands; codes and the compound's name ("MEDIUM") in its colour, as the
    // broadcast, no letter and no gaps
    Tower tg;
    const Snapshot gs = demo::make(demo::grid, 1.0);
    const View &gv = tg.update(gs, cfg, 1.0);
    CHECK(gv.h.start_tyres && gv.h.bare && gv.h.abbr && gv.rows[0].tyre == 0 && gv.rows[3].gap.empty());
    CHECK(gv.rows[0].start_tyre == gs.cars[0].tyre && gv.rows[0].start_tyre != 0);
    // ... and not once the cars move
    Snapshot mv = demo::make(demo::grid, 1.0);
    for (CarIn &c : mv.cars)
        c.speed_kmh = 80.f;
    Tower tm;
    CHECK(!tm.update(mv, cfg, 1.0).h.start_tyres);
    // tyres view: the leader starts lap 27 (counter 26 -> 27): laps on the set in the data column for 10 s
    Tower tt;
    tt.update(demo::make(demo::tyres, 0.5), cfg, 0.5);
    const Snapshot ty = demo::make(demo::tyres, 1.5);
    const View &tv2 = tt.update(ty, cfg, 1.5);
    CHECK(tv2.h.tyres && tv2.rows[4].gap == std::to_string(ty.cars[4].tyre_age) && tv2.rows[4].tyre != 0);
    CHECK(!tt.update(demo::make(demo::tyres, 12.0), cfg, 12.0).h.tyres);
    // final classification: gaps to the winner from the moment each car took the flag
    Tower tfc;
    for (double t = 0; t <= 6.01; t += 0.1) // past the 5 s purple flash of the fastest lap
        tfc.update(demo::make(demo::finish, t), cfg, t);
    const View &fv = tfc.view();
    CHECK(fv.h.final_cls && fv.rows[0].gap == "Leader" && (fv.rows[1].gap == "+0.8" || fv.rows[1].gap == "+0.9"));
    CHECK(fv.rows[2].gap == "+1.6" || fv.rows[2].gap == "+1.5" || fv.rows[2].gap == "+1.7");
    // formation lap
    Snapshot fm = demo::make(demo::race, 1.0);
    fm.s.formation = true;
    Tower tf;
    CHECK(tf.update(fm, cfg, 1.0).h.formation);
}

static void bench()
{
    // host timing of one tower tick (model + leaves over the fake core): the DLL runs it 30 times a second
    swofty::fake::Hud core;
    Publisher pub({0, 1, 2, 3});
    Tower t;
    Config cfg;
    auto t0 = std::chrono::steady_clock::now();
    const int n = 3000;
    for (int i = 0; i < n; i++) {
        double now = i / 30.0;
        Snapshot s = demo::make(demo::race, std::fmod(now, 14.0));
        s.s.demo = false;
        pub.publish(t.update(s, cfg, now), cfg, now, true);
    }
    double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count() / n;
    std::printf("bench: %.1f us per tower tick (model + leaves, host)\n", us);
    CHECK(us < 400.0);
}

static void test_live_adapter()
{
    swofty::fake::Live fake;
    fake.session().type = 15;
    fake.session().racers = 3;
    fake.session().track_length_m = 5000.f;
    fake.session().have |= SWOFTY_LIVE_SHAVE_TYPE | SWOFTY_LIVE_SHAVE_TRACK_LENGTH;
    for (uint32_t v = 0; v < 3; v++) {
        auto &car = fake.add_car(v, int(v) + 1);
        car.have |= SWOFTY_LIVE_HAVE_CLASS | SWOFTY_LIVE_HAVE_PROGRESS | SWOFTY_LIVE_HAVE_GAP_AHEAD |
                    SWOFTY_LIVE_HAVE_GAP_LEADER | SWOFTY_LIVE_HAVE_TYRE | SWOFTY_LIVE_HAVE_PIT;
        car.result_status = 2;
        car.lap = 4;
        car.lap_m = 1000.f - float(v) * 50.f;
        car.total_m = 4 * 5000.f + car.lap_m;
        car.gap_ahead_ms = v ? 1234 : SWOFTY_LIVE_NO_GAP;
        car.gap_leader_ms = v ? int(v) * 1234 : SWOFTY_LIVE_NO_GAP;
        car.tyre_letter = 'M';
        car.flags |= SWOFTY_LIVE_CAR_RACER;
        if (v != 2)
            fake.set_badge(v, v == 0 ? "ferrari_2026" : "haas_2026");
    }
    // car 2 is My Team: no stock key, the rendered badge's path
    fake.set_badge_texture(2, "eaid://ui_package/art/textures/render_badge_eUserDefined 3");
    fake.car(2).pit_state = 3; // in the pit
    fake.set_order({0, 1, 2});
    fake.set_player(1);
    fake.install();

    LiveSource src;
    Snapshot snap;
    CHECK(src.read(snap, 0.016, 0.0) == LiveSource::Why::ok);
    CHECK(snap.cars.size() == 3 && snap.cars[1].me && snap.cars[0].badge == "ferrari_2026");
    CHECK(snap.cars[2].in_pit && snap.cars[1].gap_ahead_ms == 1234);

    Tower t;
    Config cfg;
    cfg.gap = Config::Gap::interval;
    const View &v = t.update(snap, cfg, 0.0);
    CHECK(v.rows[1].gap == "+1.2" && v.rows[2].gap == "IN PIT" && v.rows[2].gap_inpit && v.rows[1].me && v.h.lap == "5");
    // "Race gap decimals" = Classic: thousandths, as in earlier F1 games
    Config c3 = cfg;
    c3.gap_ms = true;
    Tower t3;
    CHECK(t3.update(snap, c3, 0.0).rows[1].gap == "+1.234");
    CHECK(fmt_gap_race(65432, true) == "+1:05.432" && fmt_gap_race(65432) == "+1:05.4");
    CHECK(v.rows[0].team == 1);
    CHECK(v.rows[0].badge == "#overrides.osd.timing_tower.team_badges_broadcast.ferrari_2026");
    CHECK(snap.cars[2].badge.empty() && v.rows[2].badge == "eaid://ui_package/art/textures/render_badge_eUserDefined 3");
    cfg.logos = Config::Logos::game_white;
    CHECK(t.update(snap, cfg, 0.1).rows[0].badge == "#overrides.osd.timing_tower.team_badges_hud.ferrari_2026");

    fake.session().type = 18; // Time Trial: the game's own board
    CHECK(src.read(snap, 0.016, 0.1) == LiveSource::Why::time_trial);
    fake.set_available(false);
    CHECK(src.read(snap, 0.016, 0.2) == LiveSource::Why::no_core);
}

static void test_leaves_and_record()
{
    swofty::fake::Hud core;
    Publisher pub;
    Tower t;
    Config cfg;
    const View &v = t.update(demo::make(demo::race, 1.0), cfg, 1.0);
    pub.publish(v, cfg);
    CHECK(core.refused() == 0);
    CHECK(core.number("view_model_data.hud_tvt_show") == 1);
    CHECK(core.text("view_model_data.hud_tvt_r00_pos") == "1");
    CHECK(core.number("view_model_data.hud_tvt_bg1") == 1);
    // demo rows carry their own names; live rows use the game's per-car slots instead
    CHECK(core.text("view_model_data.hud_tvt_r00_nm") == "RUS");
    // every row: the game's own colour badge
    CHECK(core.number("view_model_data.hud_tvt_r00_bdg") == 1);
    CHECK(core.text("view_model_data.hud_tvt_r00_badge") == "#overrides.osd.timing_tower.team_badges_broadcast.mercedes_26");
    CHECK(core.number("view_model_data.hud_tvt_abbr") == 1 && core.number("view_model_data.hud_tvt_gained") == 0);
    // demo: no per-car slot on
    CHECK(core.number("view_model_data.hud_tvt_r00_c00") == 0);
    pub.off();
    CHECK(core.number("view_model_data.hud_tvt_show") == 0);


    // live rows: the per-car slot changes only while the row shows (never under a hidden node)
    Snapshot live = demo::make(demo::quali_q2, 0.5);
    live.s.demo = false;
    Tower tl;
    const View &lv = tl.update(live, cfg, 0.5);
    pub.publish(lv, cfg);
    int vid17 = lv.rows[15].vid;
    char k[64];
    std::snprintf(k, sizeof k, "view_model_data.hud_tvt_r15_c%02d", vid17);
    CHECK(core.number(k) == 1);
    // a Q3 view has 10 rows: row 15 hides and keeps its slot (no write under the hidden row)
    Snapshot q3 = live;
    q3.s.type = 7;
    Tower t3q;
    const View &qv = t3q.update(q3, cfg, 1.0);
    CHECK(qv.shown == 10);
    pub.publish(qv, cfg);
    CHECK(core.number("view_model_data.hud_tvt_r15_on") == 0 && core.number(k) == 1);

    // safety car: the SC block, names only (full surnames, no gaps, no tyres)
    Tower tf;
    pub.publish(tf.update(demo::make(demo::safety_car, 1.0), cfg, 1.0), cfg);
    CHECK(core.number("view_model_data.hud_tvt_c_sc") == 1 && core.number("view_model_data.hud_tvt_abbr") == 0);
    CHECK(core.text("view_model_data.hud_tvt_r01_gap").empty() && core.number("view_model_data.hud_tvt_r01_tH") == 0);
    // VSC: codes and intervals stay
    Tower tv;
    const View &vv = tv.update(demo::make(demo::vsc, 1.0), cfg, 1.0);
    CHECK(vv.h.card == card_vsc && vv.h.abbr && vv.rows[1].gap.rfind("+", 0) == 0);
    // places gained / lost as lap 2 starts
    Tower tg;
    const View &gv = tg.update(demo::make(demo::gained, 0.5), cfg, 0.5);
    CHECK(gv.h.gained && gv.rows[1].gain_on && gv.rows[1].gain == 2 && gv.rows[1].gap == "2"); // vid 3 from P4 to P2
    CHECK(gv.rows[2].gain == -1 && gv.rows[0].gain == 0 && gv.rows[0].tyre == 0);
    pub.publish(gv, cfg);
    CHECK(core.number("view_model_data.hud_tvt_gained") == 1 && core.number("view_model_data.hud_tvt_r01_gu") == 1 &&
          core.number("view_model_data.hud_tvt_r02_gd") == 1 && core.number("view_model_data.hud_tvt_r00_g0") == 1);
    CHECK(!tg.update(demo::make(demo::gained, 11.0), cfg, 11.0).h.gained); // 10 s, then intervals
    // a live grid: recorded through the formation lap, frozen at the first change of order
    Tower tl2;
    Snapshot gs = demo::make(demo::race, 0.0);
    gs.s.demo = false;
    for (CarIn &c : gs.cars)
        c.lap = 0;
    gs.s.formation = true;
    tl2.update(gs, cfg, 0.0);
    gs.s.formation = false;
    tl2.update(gs, cfg, 0.1);
    std::swap(gs.cars[0].pos, gs.cars[1].pos);
    std::swap(gs.cars[0], gs.cars[1]);
    tl2.update(gs, cfg, 0.2); // lights out: a pass
    for (CarIn &c : gs.cars)
        c.lap = 1;
    const View &lg = tl2.update(gs, cfg, 0.3);
    CHECK(lg.h.gained && lg.rows[0].gain == 1 && lg.rows[1].gain == -1);
    // private build, logos "tv": the broadcast icon (one-hot) replaces the game's badge for the teams it has
    Publisher pp({0});
    Tower ti;
    Config ctv = cfg;
    ctv.logos = Config::Logos::tv;
    pp.publish(ti.update(demo::make(demo::race, 1.0), ctv, 1.0), ctv);
    CHECK(core.number("view_model_data.hud_tvt_r00_tm00") == 1 && core.number("view_model_data.hud_tvt_r00_bdg") == 0);
    CHECK(core.number("view_model_data.hud_tvt_r01_tm00") == 0 && core.number("view_model_data.hud_tvt_r01_bdg") == 1);
    // the default, "game": the game's badge for every car (it follows logo mods)
    CHECK(Config{}.logos == Config::Logos::game);
    pp.publish(ti.update(demo::make(demo::race, 1.0), cfg, 1.1), cfg);
    CHECK(core.number("view_model_data.hud_tvt_r00_tm00") == 0 && core.number("view_model_data.hud_tvt_r00_bdg") == 1);
    // ...at its team's size: the one-hot of its team
    CHECK(core.number("view_model_data.hud_tvt_r00_bt00") == 1 && core.number("view_model_data.hud_tvt_r00_bt12") == 0);
    // the clock leaf exists (written "" once); the game's formatter owns it afterwards
    CHECK(core.text("view_model_data.hud_tvt_clock").empty());
    // row change fade: the old car stays while the row fades out, then the new one comes in
    Publisher pf;
    Tower ta;
    Snapshot a0 = demo::make(demo::race, 1.0);
    a0.s.demo = false;
    pf.publish(ta.update(a0, cfg, 1.0), cfg, 1.0, true);
    int v0 = ta.view().rows[5].vid;
    std::snprintf(k, sizeof k, "view_model_data.hud_tvt_r05_c%02d", v0);
    CHECK(core.number(k) == 1 && core.number("view_model_data.hud_tvt_r05_fx") == 1);
    Snapshot a1 = demo::make(demo::race, 7.0); // P6 and P7 swapped
    a1.s.demo = false;
    pf.publish(ta.update(a1, cfg, 1.05), cfg, 1.05, true);
    CHECK(core.number("view_model_data.hud_tvt_r05_fx") == 0 && core.number(k) == 1); // fading out, old car
    // the slides: P6's new car was P7 (one row below): row 5 slides down one row unseen, row 6 up one row
    CHECK(core.number("view_model_data.hud_tvt_r05_sd0") == 1 && core.number("view_model_data.hud_tvt_r05_su0") == 0);
    CHECK(core.number("view_model_data.hud_tvt_r06_su0") == 1 && core.number("view_model_data.hud_tvt_r06_sd1") == 0);
    pf.publish(ta.update(a1, cfg, 1.3), cfg, 1.3, true);
    CHECK(core.number("view_model_data.hud_tvt_r05_fx") == 0 && core.number(k) == 1); // still sliding
    pf.publish(ta.update(a1, cfg, 1.45), cfg, 1.45, true);
    CHECK(core.number("view_model_data.hud_tvt_r05_fx") == 1 && core.number(k) == 0); // swapped, fading in
    CHECK(core.number("view_model_data.hud_tvt_r05_sd0") == 0 && core.number("view_model_data.hud_tvt_r06_su0") == 0);
    // race lap 1: intervals show (names only under the safety car / red flag)
    CHECK(core.number("view_model_data.hud_tvt_ld2") == 1 || core.number("view_model_data.hud_tvt_ld1") == 1);

    // every leaf the record binds must be declared, with the record's {NN}/{VV} expanded
    std::set<std::string> declared;
    for (auto &p : pub.L.paths())
        declared.insert(p);
    for (const char *rec : {"build/private/addon/registry/tower.json"}) {
        std::ifstream f(rec);
        if (!f) {
            std::printf("note: %s not built, record check skipped\n", rec);
            continue;
        }
        std::stringstream ss;
        ss << f.rdbuf();
        std::string text = ss.str();
        std::regex re("\"view_model\": \"(view_model_data\\.hud_tvt_[A-Za-z0-9_{}]+)\"");
        int checked = 0;
        for (auto it = std::sregex_iterator(text.begin(), text.end(), re); it != std::sregex_iterator(); ++it) {
            std::string p = (*it)[1];
            p = std::regex_replace(p, std::regex("\\{NN\\}"), "00");
            p = std::regex_replace(p, std::regex("\\{VV\\}"), "00");
            if (p == "view_model_data.hud_tvt_clock")
                continue; // written by the game's own clock formatter, never by the DLL
            if (!declared.count(p)) {
                std::printf("FAIL record %s binds %s, which the DLL does not declare\n", rec, p.c_str());
                failures++;
            }
            checked++;
        }
        std::printf("record %s: %d bindings checked\n", rec, checked);
    }
}

// A fake game memory: blocks at fake addresses, read as the guarded reader would (false outside them)
struct FakeMem {
    std::map<uintptr_t, std::vector<uint8_t>> blocks;
    uintptr_t next = 0x10000000;
    uintptr_t alloc(size_t n)
    {
        uintptr_t at = next;
        blocks[at].assign(n, 0);
        next += (n + 0xfff) & ~uintptr_t(0xfff);
        return at;
    }
    // a block at a fixed address (an image's globals)
    void place(uintptr_t at, size_t n) { blocks[at].assign(n, 0); }
    template <class T> void put(uintptr_t at, T v)
    {
        for (auto &[b, buf] : blocks)
            if (at >= b && at + sizeof v <= b + buf.size()) {
                std::memcpy(buf.data() + (at - b), &v, sizeof v);
                return;
            }
        std::abort();
    }
    bool operator()(uintptr_t at, void *dst, size_t n) const
    {
        for (const auto &[b, buf] : blocks)
            if (at >= b && at + n <= b + buf.size()) {
                std::memcpy(dst, buf.data() + (at - b), n);
                return true;
            }
        return false;
    }
};

// A career with one season: `totals` as the table's last session (drivers 100.. in that order) and, in the entrant
// rows, driver 100+i in car i
static uintptr_t fake_career(FakeMem &m, const std::vector<int> &totals, int cars)
{
    namespace st = standings;
    uintptr_t e = m.alloc(0x1100), champ = m.alloc(0x600), seasons = m.alloc(0x20), season = m.alloc(0x400);
    uintptr_t rounds = m.alloc(2 * st::kRound), sessions = m.alloc(2 * st::kSession), entries = m.alloc(64 * 8);
    uintptr_t rows = m.alloc((cars + 2) * st::kEntrant);
    m.put<uintptr_t>(e + st::kChampionship, champ);
    m.put<uintptr_t>(champ + st::kSeasonsBegin, seasons);
    m.put<uintptr_t>(champ + st::kSeasonsEnd, seasons + 0x10);
    m.put<uint32_t>(champ + st::kSeasonIndex, 0);
    m.put<uintptr_t>(seasons + 8, season);
    uintptr_t table = season + st::kStandings + st::kTable;
    m.put<uintptr_t>(table, rounds);
    m.put<uintptr_t>(table + 8, rounds + 2 * st::kRound); // round 2 not run yet: no sessions
    m.put<uintptr_t>(rounds + 8, sessions);
    m.put<uintptr_t>(rounds + 16, sessions + 2 * st::kSession); // a sprint, then the race: the race is the table
    m.put<uintptr_t>(sessions + st::kSession + 8, entries);
    m.put<uintptr_t>(sessions + st::kSession + 16, entries + totals.size() * 8);
    for (size_t i = 0; i < totals.size(); i++) {
        m.put<uint32_t>(entries + i * 8, uint32_t(100 + i) << 16);
        m.put<int32_t>(entries + i * 8 + 4, totals[i]);
    }
    m.put<uintptr_t>(e + st::kEntrantsBegin, rows);
    m.put<uintptr_t>(e + st::kEntrantsEnd, rows + (cars + 2) * st::kEntrant);
    for (int i = 0; i < cars + 2; i++) {
        uintptr_t r = rows + uintptr_t(i) * st::kEntrant;
        m.put<uint32_t>(r + st::kEntrantDriver, uint32_t(100 + i) << 16);
        m.put<uint8_t>(r + st::kEntrantVid, i < cars ? uint8_t(i) : uint8_t(0xff)); // two reserves without a car
    }
    return e;
}

// the player's own driver (a created one): his row is another kind (low byte set) with the human bit, in car `car`
static void fake_player_row(FakeMem &m, uintptr_t e, int row, int car, uint32_t key)
{
    namespace st = standings;
    uintptr_t rows = 0;
    std::memcpy(&rows, m.blocks.at(e).data() + st::kEntrantsBegin, 8);
    uintptr_t r = rows + uintptr_t(row) * st::kEntrant;
    m.put<uint32_t>(r + st::kEntrantDriver, key);
    m.put<uint32_t>(r + st::kEntrantOrdinal, st::kHumanBit);
    m.put<uint8_t>(r + st::kEntrantVid, uint8_t(car));
}

// The game's lap times (laptimes.hpp) over a fake image: the manager global, the system registry, the lap times system
// and its per-car records with XOR'ed times, as the game lays them out
struct FakeLapTimes {
    FakeMem m;
    uintptr_t base = 0x200000000, system = 0, cars = 0;
    std::vector<uintptr_t> recs;
    static constexpr uint64_t kKey = 0x5a5a5a5a12345678ull;

    explicit FakeLapTimes(int ncars)
    {
        namespace lt = laptimes;
        const uintptr_t gm_at = lt::kGameManager - lt::kImageBase + base, key = lt::kSystemKey - lt::kImageBase + base;
        m.place(gm_at, 8);
        m.place(key, 16);
        uintptr_t mgr = m.alloc(0x100), reg = m.alloc(0x100), slots = m.alloc(16 * 8), info = m.alloc(8), name = m.alloc(64);
        system = m.alloc(0x100);
        cars = m.alloc(8 * ncars);
        m.put<uintptr_t>(gm_at, mgr);
        m.put<uintptr_t>(mgr + lt::kRegistry, reg);
        m.put<uintptr_t>(reg + lt::kSlots, slots);
        m.put<uintptr_t>(reg + lt::kSlots + 8, slots + 16 * 8); // the used bytes sit right after the 8 slots
        m.blocks[slots].resize(16 * 8 + 8, 0);
        for (int i = 0; i < 8; i++) { // other systems, then ours in slot 5
            m.put<uintptr_t>(slots + i * 16, i == 5 ? key : 0x1000 + i * 8);
            m.put<uintptr_t>(slots + i * 16 + 8, i == 5 ? system : 0x2000 + i * 8);
            m.put<uint8_t>(slots + 16 * 8 + i, 1);
        }
        (void)info;
        const char *nm = lt::kSystemName;
        for (size_t i = 0; nm[i]; i++)
            m.put<char>(name + i, nm[i]);
        m.put<uintptr_t>(key + 8, name);
        m.put<uintptr_t>(system + lt::kCars, cars);
        m.put<uintptr_t>(system + lt::kCarsEnd, cars + 8 * ncars);
        for (int c = 0; c < ncars; c++) {
            uintptr_t r = m.alloc(lt::kCarData + lt::kLapCount + 8);
            recs.push_back(r);
            m.put<uintptr_t>(cars + 8 * c, r);
        }
    }
    void put_time(uintptr_t at, float s)
    {
        uint32_t v;
        std::memcpy(&v, &s, 4);
        const uint32_t k = uint32_t(kKey >> (at & 31));
        m.put<uint32_t>(at, v ^ k);
        m.put<uint32_t>(at + 8, k);
    }
    // lap `i` of car `c`: its sectors (seconds) and whether it is valid
    void lap(int c, int i, float s1, float s2, float s3, bool valid)
    {
        namespace lt = laptimes;
        const uintptr_t lap = recs[size_t(c)] + lt::kCarData + lt::kLap0 + uintptr_t(i) * lt::kLapSize;
        put_time(lap + lt::kS1, s1);
        put_time(lap + lt::kS2, s2);
        put_time(lap + lt::kS3, s3);
        put_time(lap + lt::kTime, s1 + s2 + s3);
        const uint64_t q = 0xabcdef0012345670ull | (i & 1); // the flag's key bit varies by lap
        m.put<uint64_t>(lap + lt::kValid + 8, q);
        m.put<uint8_t>(lap + lt::kValid, uint8_t(valid ? (q & 1) : !(q & 1)));
        uint8_t n = 0;
        m(recs[size_t(c)] + lt::kCarData + lt::kLapCount, &n, 1);
        if (i + 1 > n)
            m.put<uint8_t>(recs[size_t(c)] + lt::kCarData + lt::kLapCount, uint8_t(i + 1));
    }
};

// My Team's rendered badge: the path Swofty's live state gives (team_badge_texture) when the car has no stock key
static void test_my_team_badge()
{
    CarIn c;
    c.badge_tex = "eaid://ui_package/art/textures/render_badge_eUserDefined 7";
    CHECK(badge_texture(c, false) == c.badge_tex);
    c.badge = "ferrari_26";
    CHECK(badge_texture(c, true) == "#overrides.osd.timing_tower.team_badges_hud.ferrari_26");
    CHECK(badge_texture(c, false) == "#overrides.osd.timing_tower.team_badges_broadcast.ferrari_26");
}

static void test_game_lap_times()
{
    // the model: qualifying shows the game's times (off by default) as soon as the source says they are the game's
    {
        Tower t;
        Config cfg;
        Snapshot q = demo::make(demo::quali_q1, 0.5);
        q.s.demo = false;
        q.s.game_times = false;
        CHECK(t.update(q, cfg, 0.5).rows[0].gap.empty());
        q.s.game_times = true;
        const View &v = t.update(q, cfg, 0.6);
        CHECK(v.rows[0].gap.find(':') != std::string::npos && v.rows[1].gap.rfind("+", 0) == 0 && v.h.abbr);
    }
    namespace lt = laptimes;
    FakeLapTimes f(4);
    // car 0: an out lap, a valid 1:30.250, an invalid (track limits) 1:29.800, a valid 1:30.100
    f.lap(0, 0, 40.0f, 35.0f, 32.0f, true);
    f.lap(0, 1, 30.0f, 31.25f, 29.0f, true);
    f.lap(0, 2, 29.8f, 31.0f, 29.0f, false);
    f.lap(0, 3, 30.0f, 31.1f, 29.0f, true);
    // car 2: one lap; car 3: none yet; car 1: no record at all
    f.lap(2, 0, 30.5f, 31.5f, 29.5f, true);
    f.m.put<uintptr_t>(f.cars + 8, 0);
    std::string why;
    const uintptr_t gm = lt::kGameManager - lt::kImageBase + f.base;
    uintptr_t key = lt::kSystemKey - lt::kImageBase + f.base;
    const uintptr_t sys = lt::find_system(f.m, gm, key, why);
    CHECK(sys == f.system);
    CHECK(lt::key_name_at(f.m, key) == lt::kSystemName);
    // another game build (an update, EA): the key unknown, found by its class name
    uintptr_t found = 0;
    CHECK(lt::find_system(f.m, gm, found, why) == f.system && found == key);
    lt::Car c;
    CHECK(lt::read_car(f.m, sys, 0, c, why) && c.laps == 4);
    CHECK(c.best_ms == 90100);                  // the fastest VALID lap, not the 89.8 deleted one
    CHECK(c.last_ms == 90100 && c.last_valid);
    CHECK(lt::lap_count(f.m, sys, 0) == 4);
    CHECK(lt::read_car(f.m, sys, 2, c, why) && c.best_ms == 91500 && c.laps == 1);
    CHECK(lt::read_car(f.m, sys, 3, c, why) && c.laps == 0 && c.best_ms < 0);
    CHECK(lt::read_car(f.m, sys, 1, c, why) && c.laps == 0 && c.best_ms < 0); // no record
    CHECK(lt::read_car(f.m, sys, 9, c, why) && c.best_ms < 0);                 // past the list
    // a lap whose sectors don't add up: not the game's layout, give up
    f.put_time(f.recs[2] + lt::kCarData + lt::kLap0 + lt::kTime, 95.0f);
    CHECK(!lt::read_car(f.m, sys, 2, c, why) && !why.empty());
    // no system under the key, or a broken registry: nothing, with a reason
    FakeLapTimes g(2);
    g.m.put<uintptr_t>(g.base + (lt::kGameManager - lt::kImageBase), 0);
    uintptr_t gk = 0;
    CHECK(lt::find_system(g.m, lt::kGameManager - lt::kImageBase + g.base, gk, why) == 0 && why == "no game manager");
    CHECK(lt::find_system(g.m, 0, gk, why) == 0 && why == "no game manager"); // Swofty has no binding
}

static void test_qcard()
{
    namespace lt = laptimes;
    // the game's records: best lap sectors, the best of each sector, the lap in progress
    {
        FakeLapTimes f(2);
        f.lap(0, 0, 30.0f, 31.25f, 29.0f, true);
        f.lap(0, 1, 29.9f, 31.4f, 29.1f, true);   // 90.4: not the best, but the best sector 1
        f.lap(0, 2, 29.5f, 31.0f, 28.5f, false);  // deleted: its sectors don't count
        std::string why;
        uintptr_t key = 0;
        const uintptr_t sys = lt::find_system(f.m, lt::kGameManager - lt::kImageBase + f.base, key, why);
        lt::Car c;
        CHECK(lt::read_car(f.m, sys, 0, c, why) && c.best_ms == 90250);
        CHECK(c.best_sec[0] == 30000 && c.best_sec[1] == 31250 && c.best_sec[2] == 29000);
        CHECK(c.pb_sec[0] == 29900 && c.pb_sec[1] == 31250 && c.pb_sec[2] == 29000);
        CHECK(c.cur_sec[0] < 0 && c.cur_sec[1] < 0);
        // the next lap under way: sector 1 done (no lap time yet)
        const uintptr_t nx = f.recs[0] + lt::kCarData + lt::kLap0 + 3 * lt::kLapSize;
        f.put_time(nx + lt::kS1, 30.2f);
        CHECK(lt::read_car(f.m, sys, 0, c, why) && c.laps == 3 && c.cur_sec[0] == 30200 && c.cur_sec[1] < 0);
        f.put_time(nx + lt::kS2, 31.1f);
        CHECK(lt::read_car(f.m, sys, 0, c, why) && c.cur_sec[1] == 31100);
        // a car with no lap yet, its first under way
        f.put_time(f.recs[1] + lt::kCarData + lt::kLap0 + lt::kS1, 30.6f);
        CHECK(lt::read_car(f.m, sys, 1, c, why) && c.laps == 0 && c.cur_sec[0] == 30600);
    }
    // the tracker: an out lap (no card), the line, the splits placed by distance, the game's figures when they agree
    {
        MyLapTracker tr;
        MySample m;
        m.ok = true;
        m.vid = 3;
        m.end[0] = 1500.f;
        m.end[1] = 3000.f;
        m.in_pit = true;
        m.cross = -1;
        m.sim = 1.0;
        m.lap_m = 200.f;
        CHECK(!tr.step(m).known);
        m.in_pit = false; // out lap: crossings known, but the lap began in the pit lane
        m.cross = 0.5;
        m.sim = 2.0;
        CHECK(!tr.step(m).flying);
        // the line at sim 10.0: a flying lap
        m.cross = 10.0;
        m.sim = 10.1;
        m.lap_m = 5.f;
        MyLap l = tr.step(m);
        CHECK(l.known && l.flying && l.laps == 1 && l.t_ms == 100);
        m.sim = 40.0;
        m.lap_m = 1490.f;
        CHECK(tr.step(m).split[0] < 0);
        m.sim = 40.2;
        m.lap_m = 1510.f; // 1500 halfway: split at 40.1 -> 30.100
        l = tr.step(m);
        CHECK(l.split[0] == 30100 && l.split[1] < 0 && l.split_age[0] > 0.09 && l.split_age[0] < 0.11);
        // the game's sector 1 a few ms off ours: the game's
        m.game_sec[0] = 30087;
        m.sim = 40.3;
        m.lap_m = 1515.f;
        CHECK(tr.step(m).split[0] == 30087);
        // a stale figure far from ours is ignored
        MyLapTracker t2 = tr;
        m.game_sec[1] = 5000;
        m.sim = 72.0;
        m.lap_m = 2990.f;
        tr.step(m);
        m.sim = 72.2;
        m.lap_m = 3010.f;
        l = tr.step(m);
        CHECK(l.split[1] == 72100 - 10000);
        (void)t2;
        // the line again: the lap is done; then the game's lap time replaces ours
        m.game_sec[0] = m.game_sec[1] = -1;
        m.game_last_ms = 88000; // the previous lap's, still in the record
        m.cross = 99.5;
        m.sim = 99.6;
        m.lap_m = 4.f;
        l = tr.step(m);
        CHECK(l.done_ms == 89500 && l.done_split[0] == 30087 && l.flying && l.laps == 2);
        m.game_last_ms = 89512;
        m.sim = 100.0;
        l = tr.step(m);
        CHECK(l.done_ms == 89512 && l.done_age > 0.3 && l.done_age < 0.5);
        // into the pit lane: no flying lap
        m.in_pit = true;
        m.sim = 101.0;
        CHECK(!tr.step(m).flying);
    }
    // the card (the 2026 broadcast's): qualifying only; the time running grey until the first split; at a split
    // its gap to the target (P1 or the cut) big for 4.5 s with SECTOR n; the target's time to the current sector end
    {
        Tower t;
        Config cfg;
        Snapshot q = demo::make(demo::quali_q1, 0.5);
        const View *v = &t.update(q, cfg, 0.5);
        const QCard &k = v->card;
        CHECK(k.on && k.vid == demo::kPlayer && !k.big.empty() && k.big.find(':') == std::string::npos);
        CHECK(k.big_col == big_grey && k.sec[0] == sec_now && k.sec[1] == sec_none && !k.sec_long[0]);
        cfg.qcard = false;
        CHECK(!t.update(q, cfg, 0.6).card.on);
        cfg.qcard = true;
        Snapshot r = demo::make(demo::race, 0.5);
        CHECK(!t.update(r, cfg, 0.7).card.on);

        Tower t2;
        Snapshot s = demo::make(demo::quali_q1, 0.5);
        CarIn *me = nullptr, *p1 = nullptr;
        for (CarIn &c : s.cars) {
            if (c.me)
                me = &c;
            if (c.pos == 1)
                p1 = &c;
        }
        CHECK(me && p1 && me->pos <= 16);
        // sector 1 under way: the target's split 1 on the right ("23.67x", not a lap time)
        v = &t2.update(s, cfg, 0.9);
        CHECK(v->card.target == fmt_lap_short(p1->best_sec[0]) && v->card.target_vid == int(p1->vid));
        // split 1 crossed 0.100 under P1's: "-0.100" purple in place of the time, SECTOR 1, the bar purple
        s.my.split[0] = p1->best_sec[0] - 100;
        s.my.split_age[0] = 1.0;
        s.my.t_ms = s.my.split[0] + 1000;
        v = &t2.update(s, cfg, 1.0);
        CHECK(v->card.big == "-0.100" && v->card.big_col == big_purple && v->card.sec_long[0]);
        CHECK(v->card.target == fmt_lap_short(p1->best_sec[0]) && v->card.sec[0] == sec_purple && v->card.sec[1] == sec_now);
        // the hold over: the time again (white now), the target to the end of sector 2
        s.my.split_age[0] = 5.0;
        s.my.t_ms = s.my.split[0] + 5000;
        v = &t2.update(s, cfg, 1.1);
        CHECK(v->card.big == fmt_run(s.my.t_ms) && v->card.big_col == big_white && !v->card.sec_long[0]);
        CHECK(v->card.target == fmt_lap_short(p1->best_sec[0] + p1->best_sec[1]));
        // slower: yellow; a sector under the player's own best only: green
        s.my.split[0] = me->pb_sec[0] - 1;
        s.my.split_age[0] = 0.5;
        v = &t2.update(s, cfg, 1.2);
        CHECK(v->card.big_col == big_yellow && v->card.big[0] == '+' && v->card.sec[0] == sec_green);
        s.my.split[0] = me->pb_sec[0] + 400;
        CHECK(t2.update(s, cfg, 1.3).card.sec[0] == sec_yellow);
        // sector 3: the target's lap
        s.my.split[0] = me->pb_sec[0];
        s.my.split[1] = s.my.split[0] + me->pb_sec[1];
        s.my.split_age[0] = 30;
        s.my.split_age[1] = 6;
        s.my.t_ms = s.my.split[1] + 6000;
        v = &t2.update(s, cfg, 1.4);
        CHECK(v->card.target == fmt_lap(p1->best_ms) && v->card.sec[2] == sec_now);

        // the knockout zone: the target is the car on the cut (P16 of 22), green when under it
        Tower t3;
        Snapshot z = demo::make(demo::quali_q1, 0.5);
        for (CarIn &c : z.cars)
            if (c.me) {
                c.pos = 19;
                c.best_ms = 99000;
            }
        const CarIn *cutc = nullptr;
        for (const CarIn &c : z.cars)
            if (c.pos == 16 && !c.me)
                cutc = &c;
        CHECK(cutc != nullptr);
        z.my.split[0] = cutc->best_sec[0] - 50;
        z.my.split_age[0] = 0.2;
        z.my.t_ms = z.my.split[0] + 200;
        v = &t3.update(z, cfg, 1.0);
        CHECK(v->card.target_vid == int(cutc->vid) && v->card.big == "-0.050" && v->card.big_col == big_green);
        CHECK(v->card.pos == "19");

        // a lap done: its gap with SECTOR 3 for 4.5 s, then its time, and nothing after 8 s
        Tower t4;
        Snapshot d = demo::make(demo::quali_q1, 0.5);
        d.my.flying = false;
        d.my.t_ms = -1;
        d.my.split[0] = d.my.split[1] = -1;
        d.my.done_ms = 78500;
        d.my.done_split[0] = 23500;
        d.my.done_split[1] = 55000;
        d.my.done_age = 1.0;
        v = &t4.update(d, cfg, 1.0);
        CHECK(v->card.on && v->card.sec_long[2] && (v->card.big[0] == '-' || v->card.big[0] == '+'));
        CHECK(v->card.sec[2] != sec_none && v->card.sec[2] != sec_now);
        d.my.done_age = 6.0;
        v = &t4.update(d, cfg, 1.5);
        CHECK(v->card.big == "1:18.500" && v->card.big_col == big_white && !v->card.sec_long[2]);
        d.my.done_age = 8.5;
        CHECK(!t4.update(d, cfg, 2.0).card.on);
        // a cool-down lap: sector 1 far off the player's best -> no card for the rest of that lap
        d.my.done_ms = -1;
        d.my.done_age = -1;
        d.my.flying = true;
        d.my.laps = 7;
        for (const CarIn &c : d.cars)
            if (c.me)
                d.my.split[0] = c.pb_sec[0] + 5000;
        d.my.split_age[0] = 0.1;
        d.my.t_ms = d.my.split[0] + 100;
        CHECK(!t4.update(d, cfg, 3.0).card.on);
        d.my.split[0] = -1;
        d.my.split_age[0] = -1;
        d.my.t_ms = 1000; // still the same lap (laps unchanged): stays off
        CHECK(!t4.update(d, cfg, 3.1).card.on);
        d.my.laps = 8;
        CHECK(t4.update(d, cfg, 3.2).card.on);
    }
    // the formats
    CHECK(fmt_run(9400) == "9.4" && fmt_run(56690) == "56.6" && fmt_run(81849) == "1:21.8");
    CHECK(fmt_lap_short(30752) == "30.752" && fmt_lap_short(64773) == "1:04.773");
    CHECK(fmt_delta(-457) == "-0.457" && fmt_delta(696) == "+0.696" && fmt_delta(0) == "+0.000");
}

static void test_championship()
{
    namespace st = standings;
    std::vector<int> totals;
    for (int i = 0; i < 20; i++)
        totals.push_back(240 - i * 11);
    {
        FakeMem m;
        uintptr_t e = fake_career(m, totals, 20);
        std::vector<ChampEntry> t;
        std::unordered_map<uint32_t, int> cars;
        std::string why;
        CHECK(st::read_table(m, e, t, why) && t.size() == 20 && t[0].stat == 100 && t[0].points == 240 && t[0].rank == 1);
        CHECK(t[19].stat == 119 && t[19].points == 240 - 19 * 11);
        CHECK(st::read_cars(m, e, cars, why) && cars.size() == 20 && cars[105] == 5 && !cars.count(120));
    }
    {
        // a driver the player created (0.17.0 left him out: his row is not a stock driver's): found, as the player
        FakeMem m;
        uintptr_t e = fake_career(m, totals, 20);
        fake_player_row(m, e, 8, 8, (uint32_t(108) << 16) | 0x5);
        std::unordered_map<uint32_t, int> cars;
        std::unordered_set<uint32_t> known;
        long human = -1;
        std::string why;
        CHECK(st::read_cars(m, e, cars, why, &known, &human) && cars[108] == 8 && human == 108 && cars.size() == 20);
    }
    {
        // not in the game's order (a wrong layout): refused
        FakeMem m;
        std::vector<int> bad = totals;
        std::swap(bad[3], bad[9]);
        uintptr_t e = fake_career(m, bad, 20);
        std::vector<ChampEntry> t;
        std::string why;
        CHECK(!st::read_table(m, e, t, why) && t.empty() && !why.empty());
    }
    {
        // the game rewriting the rows, no entity, pointers to nowhere: refused, never a fault
        FakeMem m;
        uintptr_t e = fake_career(m, totals, 20);
        m.put<uint8_t>(e + st::kEntrantsBusy, 1);
        std::unordered_map<uint32_t, int> cars;
        std::vector<ChampEntry> t;
        std::string why;
        CHECK(!st::read_cars(m, e, cars, why));
        CHECK(!st::read_table(m, 0, t, why));
        m.put<uintptr_t>(e + st::kChampionship, 0xdead0000);
        CHECK(!st::read_table(m, e, t, why) && t.empty());
    }
    {
        // the formation lap: starting tyres first, then the championship in its order with the points, then the names
        Config cfg;
        Tower t;
        Snapshot s = demo::make(demo::race, 1.0);
        for (CarIn &c : s.cars) {
            c.lap = 0;
            c.speed_kmh = 80.f;
        }
        s.s.demo = false;
        s.s.formation = true;
        for (size_t i = 0; i < s.cars.size(); i++) // the championship: the classification backwards
            s.champ.push_back({uint32_t(i), int(200 - i * 9), int(i) + 1, int(s.cars[s.cars.size() - 1 - i].vid)});
        const View &a = t.update(s, cfg, 0.0);
        CHECK(a.h.start_tyres && !a.h.champ);
        const View &b = t.update(s, cfg, 21.0);
        CHECK(b.h.champ && !b.h.start_tyres && !b.h.abbr);
        CHECK(b.rows[0].vid == int(s.cars.back().vid) && b.rows[0].pos == "1" && b.rows[0].p1 && b.rows[0].gap == "200");
        CHECK(b.rows[1].gap == "191" && b.rows[1].tyre == 0 && b.rows[1].start_tyre == 0);
        // then the constructors': each team's drivers' points added up, the best first, the team's name and badge
        const View &cs = t.update(s, cfg, 55.0);
        CHECK(cs.h.cons && !cs.h.champ && !cs.h.start_tyres);
        {
            std::map<int, int> sum;
            for (const ChampEntry &e : s.champ)
                for (const CarIn &c : s.cars)
                    if (int(c.vid) == e.vid)
                        sum[team_index(c.badge)] += e.points;
            int top = 0, teams = int(sum.size());
            for (auto &kv : sum)
                top = std::max(top, kv.second);
            CHECK(cs.shown == teams && cs.rows[0].pos == "1" && cs.rows[0].gap == std::to_string(top));
            CHECK(!cs.rows[0].team_label.empty() && cs.rows[0].vid == -1 && !cs.rows[0].badge.empty());
            for (int i = 1; i < cs.shown; i++)
                CHECK(std::stoi(cs.rows[size_t(i)].gap) <= std::stoi(cs.rows[size_t(i - 1)].gap));
        }
        // a driver with points but no car here (a reserve, a replaced driver): no constructors' table
        {
            Snapshot r = s;
            r.champ.push_back({99u, 4, int(r.champ.size()) + 1, -1});
            Tower tr;
            tr.update(r, cfg, 0.0);
            CHECK(!tr.update(r, cfg, 55.0).h.cons);
        }
        const View &c = t.update(s, cfg, 75.0);
        CHECK(!c.h.champ && !c.h.cons && !c.h.start_tyres && c.rows[0].vid == int(s.cars.front().vid));
        // no table (a Grand Prix, or unreadable): the starting tyres stay for the whole formation lap
        Snapshot g = s;
        g.champ.clear();
        Tower tg;
        tg.update(g, cfg, 0.0);
        CHECK(tg.update(g, cfg, 30.0).h.start_tyres);
        // switched off
        Config off = cfg;
        off.champ_formation = false;
        Tower to;
        to.update(s, off, 0.0);
        CHECK(!to.update(s, off, 25.0).h.champ);
    }
}

int main()
{
    test_format();
    test_race_rows();
    test_leader_mode_and_rows();
    test_quali();
    test_field_persists_across_quali_segments();
    test_cards();
    test_banners();
    test_game_lap_times();
    test_my_team_badge();
    test_qcard();
    test_championship();
    test_yellow_texts();
    test_lap_clock();
    test_session_views();
    test_live_adapter();
    test_leaves_and_record();
    bench();
    if (failures) {
        std::printf("tower tests: %d failed\n", failures);
        return 1;
    }
    std::printf("tower tests: ok\n");
    return 0;
}
