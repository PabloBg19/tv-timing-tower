// SPDX-License-Identifier: MIT
// Host tests: the model, the lap clock, the live adapter (over Swofty's fake live state) and the leaves (over
// Swofty's fake HUD core), and that every leaf the record binds is one the DLL declares.
//   tools/build.py test
#include <cassert>
#include <cstdio>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <string>

#include <swofty/fake_hud.hpp>
#include <swofty/fake_live.hpp>

#include "../src/demo.hpp"
#include "../src/lap_clock.hpp"
#include "../src/leaves.hpp"
#include "../src/model.hpp"
#include "../src/source_live.hpp"

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
    CHECK(fmt_laps_down(1) == "+1 LAP");
    CHECK(fmt_laps_down(3) == "+3 LAPS");
    CHECK(fmt_clock(2036) == "33:56");
    CHECK(std::string(session_label(6)) == "Q2");
    CHECK(std::string(session_label(11)) == "SQ2");
    CHECK(knockout_cut(1, 22) == 16 && knockout_cut(1, 20) == 15 && knockout_cut(2, 22) == 10);
    CHECK(team_index("ferrari_2026") == 1);
    CHECK(team_index("red_bull_2026") == 2);
    CHECK(team_index("racing_bulls_2026") == 7);
    CHECK(team_index("rb_2025") == 7);
    CHECK(team_index("kick_sauber_2025") == 9);
    CHECK(team_index("audi_2026") == 10);
    CHECK(team_index("cadillac_2026") == 11);
    CHECK(team_index("aston_martin_2026") == 4);
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
    CHECK(v.rows[0].p1 && v.rows[0].gap == "INTERVAL" && v.rows[0].gap_label);
    CHECK(v.rows[1].gap.rfind("+", 0) == 0);
    CHECK(v.rows[19].gap == "+1 LAP" || v.rows[19].gap.rfind("+", 0) == 0);
    CHECK(v.rows[8].pen && v.rows[8].pen_text == "+5s");
    // a pass: P6 and P7 swap after 6 s and get arrows
    t.update(demo::make(demo::race, 7.0), cfg, 7.0);
    const View &v2 = t.view();
    CHECK(v2.rows[5].up && v2.rows[6].dn);
    t.update(demo::make(demo::race, 11.5), cfg, 11.5);
    CHECK(!t.view().rows[5].up);
    // the fastest lap flashes purple then stays as the icon
    CHECK(t.view().rows[1].fl);
    CHECK(t.view().h.card == card_yellow && t.view().h.ysec == "S2");
    CHECK(t.view().rows[21].gap == "DNF" && t.view().rows[21].out);
}

static void test_leader_mode_and_rows()
{
    Tower t;
    Config cfg;
    cfg.gap = Config::Gap::leader;
    const View &v = t.update(demo::make(demo::race, 1.0), cfg, 1.0);
    CHECK(v.h.lead && v.rows[0].gap == "LEADER");
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
    Tower t;
    Config cfg;
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
    // Q2: the six from Q1 are dimmed, the knockout zone is P11-P16
    Tower t2;
    const View &q2 = t2.update(demo::make(demo::quali_q2, 0.5), cfg, 0.5);
    CHECK(q2.rows[17].elim && !q2.rows[17].ko);
    CHECK(q2.rows[10].ko_line && q2.rows[15].ko && !q2.rows[16].ko);
    // personal best flash (green) for the player after 5 s
    Tower t3;
    t3.update(demo::make(demo::quali_q1, 4.0), cfg, 4.0);
    const View &q = t3.update(demo::make(demo::quali_q1, 5.5), cfg, 5.5);
    bool green = false;
    for (const Row &r : q.rows)
        green |= r.gap_green || r.gap_purple;
    CHECK(green);
}

static void test_cards()
{
    Config cfg;
    {
        Tower t;
        CHECK(t.update(demo::make(demo::safety_car, 1), cfg, 1).h.card == card_sc);
        CHECK(t.update(demo::make(demo::safety_car, 9), cfg, 9).h.card == card_sc_end);
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
        CHECK(v.h.card == card_red && v.rows[3].gap == "PIT");
    }
    {
        Tower t;
        const View &v = t.update(demo::make(demo::finish, 5), cfg, 5);
        CHECK(v.h.card == card_chq && v.rows[0].chq && v.rows[21].gap == "DNF" && v.rows[20].gap == "DSQ");
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
    CHECK(c.fastest_vid() == 0);
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
        fake.set_badge(v, v == 0 ? "ferrari_2026" : "haas_2026");
    }
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
    CHECK(v.rows[1].gap == "+1.2" && v.rows[2].gap == "PIT" && v.rows[1].me && v.h.lap == "5");
    CHECK(v.rows[0].team == 1);

    fake.session().type = 18; // Time Trial: the game's own board
    CHECK(src.read(snap, 0.016, 0.1) == LiveSource::Why::time_trial);
    fake.set_available(false);
    CHECK(src.read(snap, 0.016, 0.2) == LiveSource::Why::no_core);
}

static void test_leaves_and_record()
{
    swofty::fake::Hud core;
    Publisher pub({1, 8});
    Tower t;
    Config cfg;
    const View &v = t.update(demo::make(demo::race, 1.0), cfg, 1.0);
    pub.publish(v, cfg);
    CHECK(core.refused() == 0);
    CHECK(core.number("view_model_data.hud_tvt_show") == 1);
    CHECK(core.text("view_model_data.hud_tvt_r00_pos") == "1");
    CHECK(core.number("view_model_data.hud_tvt_bg1") == 1);
    // demo rows carry their own names; live rows use the game's per-car slots instead
    CHECK(core.text("view_model_data.hud_tvt_r00_nm") == "RUSSELL");
    // RUSSELL (Mercedes) has no private logo in this list: the game's badge shows; LECLERC (Ferrari) has one
    CHECK(core.number("view_model_data.hud_tvt_r00_bdg") == 1);
    CHECK(core.number("view_model_data.hud_tvt_r02_tm01") == 1 && core.number("view_model_data.hud_tvt_r02_bdg") == 0);
    pub.off();
    CHECK(core.number("view_model_data.hud_tvt_show") == 0);

    // every leaf the record binds must be declared, with the record's {NN}/{VV} expanded
    std::set<std::string> declared;
    for (auto &p : pub.L.paths())
        declared.insert(p);
    for (const char *rec : {"build/public/addon/registry/tower.json", "build/private/addon/registry/tower.json"}) {
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

int main()
{
    test_format();
    test_race_rows();
    test_leader_mode_and_rows();
    test_quali();
    test_cards();
    test_lap_clock();
    test_live_adapter();
    test_leaves_and_record();
    if (failures) {
        std::printf("tower tests: %d failed\n", failures);
        return 1;
    }
    std::printf("tower tests: ok\n");
    return 0;
}
