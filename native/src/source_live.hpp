// SPDX-License-Identifier: MIT
// TV Timing Tower - the game's live state (swofty::live, read from memory by Swofty's core, no UDP) turned into
// a model Snapshot. Every value the core marks as not read stays at the model's "unknown" default.
#pragma once
#include <array>
#include <optional>
#include <string>
#include <vector>

#include <cstring>
#include <unordered_map>
#include <unordered_set>

#include <swofty/career.hpp>
#include <swofty/live.hpp>
#include <swofty/mem.hpp>

#include "guarded_read.hpp"
#include "lap_clock.hpp"
#include "laptimes.hpp"
#include "qcard.hpp"
#include "model.hpp"
#include "standings.hpp"

namespace tvt {

namespace live = swofty::live;

class LiveSource {
public:
    enum class Why { ok, no_core, no_session, no_order, time_trial };

    void reset()
    {
        clock_.reset();
        stints_.reset();
        my_.reset();
        rd.invalidate();
        badges_.fill(Badge{});
        last_type_ = -1;
        last_gen_ = ~0u;
        champ_reset();
    }

    // The drivers' championship of a career, for the formation lap (standings.hpp): read once a race session, retried
    // every 2 s a few times; note() changes once with what came of it, for the log
    const std::string &champ_note() const { return champ_note_; }
    // what came of reading the game's own lap times (laptimes.hpp), for the log
    const std::string &times_note() const { return lt_note_; }

    // Fills `snap` from the live state. `dt` is the frame time; `now` the wall clock (badge refresh).
    Why read(Snapshot &snap, double dt, double now)
    {
        snap = Snapshot{};
        if (!live::available()) {
            rd.invalidate();
            return Why::no_core;
        }
        std::optional<live::Session> so = live::session();
        if (!so || !live::has(*so, SWOFTY_LIVE_SHAVE_TYPE)) {
            rd.invalidate();
            return Why::no_session;
        }
        const live::Session &s = *so;
        if (live::game_mode(s) == live::GameMode::time_trial || s.type == 18)
            return Why::time_trial;
        if (!live::has(s, SWOFTY_LIVE_SHAVE_ORDER) || s.classified == 0)
            return Why::no_order;

        if (int(s.type) != last_type_ || s.generation != last_gen_) {
            rd.invalidate();
            if (int(s.type) != last_type_) {
                clock_.reset();
                stints_.reset();
                my_.reset();
                champ_reset();
                lt_cars_.fill(LtCar{});
            }
            badges_.fill(Badge{});
            last_type_ = int(s.type);
            last_gen_ = s.generation;
        }

        SessionIn &si = snap.s;
        si.type = int(s.type);
        si.racers = int(s.racers);
        si.track_m = live::has(s, SWOFTY_LIVE_SHAVE_TRACK_LENGTH) ? s.track_length_m : 0.f;
        switch (live::safety_car(s)) {
        case live::SafetyCar::safety_car: si.sc = 1; break;
        case live::SafetyCar::virtual_safety_car: si.sc = 2; break;
        case live::SafetyCar::formation: si.sc = 0; si.formation = true; break;
        default: si.sc = 0; break;
        }
        si.red = live::has(s, SWOFTY_LIVE_SHAVE_RED_FLAG) && s.red_flag != 0;
        si.yellow = live::has(s, SWOFTY_LIVE_SHAVE_YELLOW) ? s.yellow_sectors : 0;
        si.chequered = s.chequered != 0;

        const int me = s.subject >= 0 ? s.subject : s.player;
        const uint32_t n = s.classified < live::max_cars ? s.classified : live::max_cars;

        // lap clock over every classified car
        samples_.clear();
        std::array<std::optional<live::Car>, live::max_cars> cars{};
        for (uint32_t i = 0; i < n; i++) {
            uint32_t vid = s.order[i];
            if (vid >= live::max_cars)
                continue;
            cars[vid] = live::car(vid);
            const auto &c = cars[vid];
            if (c && live::has(*c, SWOFTY_LIVE_HAVE_PROGRESS)) {
                LapSample ls;
                ls.vid = vid;
                ls.lap = c->lap;
                ls.lap_m = c->lap_m;
                ls.in_pit = live::in_pit(*c) || (live::pit_status(*c).value_or(0) != 0);
                ls.warnings = live::has(*c, SWOFTY_LIVE_HAVE_PENALTIES) ? c->track_limit_warnings : 0;
                samples_.push_back(ls);
            }
        }
        clock_.step(dt, si.track_m, samples_);
        si.fastest_vid = clock_.fastest_vid();
        si.fastest_ms = clock_.fastest_ms();

        const bool race = kind_of(si.type) == Kind::race;
        int floor_ms = 0;
        for (uint32_t i = 0; i < n; i++) {
            uint32_t vid = s.order[i];
            if (vid >= live::max_cars || !cars[vid])
                continue;
            const live::Car &c = *cars[vid];
            CarIn ci;
            ci.vid = vid;
            ci.pos = live::has(c, SWOFTY_LIVE_HAVE_CLASS) && c.position > 0 ? c.position : int(i) + 1;
            ci.status = live::has(c, SWOFTY_LIVE_HAVE_CLASS) ? c.result_status : 2;
            ci.in_pit = live::in_pit(c) || live::pit_status(c).value_or(0) != 0;
            if (live::has(c, SWOFTY_LIVE_HAVE_PROGRESS)) {
                ci.lap = c.lap;
                ci.total_m = c.total_m;
                ci.have_total = true;
            }
            if (live::has(c, SWOFTY_LIVE_HAVE_TYRE))
                ci.tyre = c.tyre_letter;
            ci.speed_kmh = live::speed_kmh(c).value_or(-1.f);
            auto stops = live::pit_stops(c);
            ci.pit_stops = stops ? int(*stops) : -1;
            if (live::has(c, SWOFTY_LIVE_HAVE_PROGRESS))
                ci.tyre_age = stints_.age(vid, c.lap, stops.value_or(0), ci.in_pit);
            if (live::has(c, SWOFTY_LIVE_HAVE_PENALTIES)) {
                ci.pen_s = c.time_penalty_s;
                ci.drive_through = c.drive_through != 0;
                ci.stop_go = c.stop_go != 0;
            }
            ci.gap_ahead_ms = live::gap_ahead_ms(c).value_or(-1);
            ci.gap_leader_ms = live::gap_leader_ms(c).value_or(-1);
            ci.me = int(vid) == me;
            ci.badge = badge(vid, now);
            ci.last_ms = clock_.last_ms(vid);
            if (race) {
                ci.best_ms = clock_.best_ms(vid);
            } else {
                // prefer a time that doesn't put this car ahead of one the game ranks higher (see
                // LapClock::best_at_least), but showing a gap beats showing nothing: fall back to the car's own
                // best lap (even if it doesn't fit that order) rather than leaving the row blank
                int coherent = clock_.best_at_least(vid, floor_ms);
                ci.best_ms = coherent > 0 ? coherent : clock_.best_ms(vid);
                if (ci.best_ms > floor_ms)
                    floor_ms = ci.best_ms; // a fallback faster than the floor doesn't lower it for the cars below
            }
            snap.cars.push_back(std::move(ci));
        }

        // the safety car coming in: the SC vehicle (not a racer) has entered the pit lane
        if (si.sc == 1) {
            uint32_t total = s.cars < live::max_cars ? s.cars : live::max_cars;
            for (uint32_t v = 0; v < total; v++) {
                auto c = (v < live::max_cars && cars[v]) ? cars[v] : live::car(v);
                if (c && !live::is_racer(*c) && (live::in_pit(*c) || live::pit_status(*c).value_or(0) != 0)) {
                    si.sc_in = true;
                    break;
                }
            }
        }
        if (race && !snap.cars.empty())
            champ_step(snap, now);
        // every session: the game's own lap times (simulated laps included) in place of the addon's clock
        if (!snap.cars.empty())
            game_times(snap, now, me);
        // races: the fastest lap is the game's (its fastest valid lap, as its own best lap toast), not the clock's
        if (race && snap.s.game_times) {
            si.fastest_vid = -1;
            si.fastest_ms = -1;
            for (const CarIn &c : snap.cars)
                if (c.best_ms > 0 && (si.fastest_ms < 0 || c.best_ms < si.fastest_ms)) {
                    si.fastest_ms = c.best_ms;
                    si.fastest_vid = int(c.vid);
                }
        }
        // qualifying: the player's lap for the card
        if (!race && me >= 0 && me < int(live::max_cars) && cars[size_t(me)] &&
            live::has(*cars[size_t(me)], SWOFTY_LIVE_HAVE_PROGRESS)) {
            const live::Car &c = *cars[size_t(me)];
            MySample ms;
            ms.ok = true;
            ms.vid = uint32_t(me);
            ms.sim = clock_.sim_time();
            ms.cross = clock_.cross_t(uint32_t(me));
            ms.lap_m = c.lap_m;
            ms.in_pit = live::in_pit(c) || live::pit_status(c).value_or(0) != 0;
            ms.sector = live::has(c, SWOFTY_LIVE_HAVE_SECTOR) ? int(c.sector) : -1;
            if (live::has(s, SWOFTY_LIVE_SHAVE_SECTORS) && s.sector_count >= 3 && si.track_m > 100.f) {
                for (int k = 0; k < 2; k++) {
                    const float e = s.sector_end_m[k];
                    ms.end[k] = e > 50.f && e < si.track_m - 50.f ? e : -1.f;
                }
                if (ms.end[0] < 0 || ms.end[1] <= ms.end[0])
                    ms.end[0] = ms.end[1] = -1.f;
            }
            if (lt_state_ == 1 && lt_system_ && size_t(me) < lt_cars_.size()) {
                const laptimes::Car &g = lt_cars_[size_t(me)].car;
                ms.game_sec[0] = g.cur_sec[0];
                ms.game_sec[1] = g.cur_sec[1];
                ms.game_last_ms = g.last_ms;
            }
            snap.my = my_.step(ms);
        } else {
            my_.reset();
        }
        return snap.cars.empty() ? Why::no_order : Why::ok;
    }

    const LapClock &clock() const { return clock_; }

private:
    // ---- the game's lap times (laptimes.hpp): the system is looked up once a second until found; each tick reads
    // every car's lap count (one byte) and its laps again when that changes or every 2 s
    struct LtCar {
        int laps = -2;
        double next = 0;
        laptimes::Car car;
    };

    // guarded reads with a region cache (guarded_read.hpp), dropped on every session change
    GuardedReader rd;

    void game_times(Snapshot &snap, double now, int me)
    {
        if (lt_state_ == 2)
            return;
        const uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
        if (lt_state_ == 0) {
            // only the game build these addresses come from
            uint32_t stamp = 0;
            uintptr_t nt = 0;
            int32_t lfanew = 0;
            if (!base || !rd(base + 0x3c, &lfanew, 4) || !rd(base + uintptr_t(lfanew) + 8, &stamp, 4) ||
                stamp != laptimes::kPeStamp) {
                lt_state_ = 2;
                lt_note_ = "lap times: not this game build (stamp " + std::to_string(stamp) + "): no times";
                return;
            }
            (void)nt;
            lt_state_ = 1;
        }
        if (!lt_system_ || now >= lt_check_) {
            if (now < lt_search_ && !lt_system_)
                return;
            lt_search_ = now + 1.0;
            lt_check_ = now + 5.0;
            std::string why;
            uintptr_t sys = laptimes::find_system(rd, base, why);
            if (sys && lt_name_.empty()) {
                // the key's class name, when the game has one there: another name is another system (the key is
                // wrong for this game), no name leaves it to the data's own checks (sectors adding up to the lap)
                lt_name_ = laptimes::key_name(rd, base);
                if (lt_name_.empty())
                    lt_name_ = "?";
                else if (lt_name_ != laptimes::kSystemName) {
                    lt_state_ = 2;
                    lt_note_ = "lap times: the registry key is '" + lt_name_ + "', not " + laptimes::kSystemName + ": no times";
                    return;
                }
            }
            if (sys != lt_system_)
                lt_cars_.fill(LtCar{});
            lt_system_ = sys;
            if (!sys) {
                lt_note_ = "lap times: " + why;
                return;
            }
        }
        int with = 0;
        for (CarIn &c : snap.cars) {
            if (c.vid >= lt_cars_.size())
                continue;
            LtCar &lc = lt_cars_[c.vid];
            const int n = laptimes::lap_count(rd, lt_system_, c.vid);
            if (n != lc.laps || now >= lc.next) {
                std::string why;
                laptimes::Car car;
                if (!laptimes::read_car(rd, lt_system_, c.vid, car, why)) {
                    // a record that doesn't look right: no game times this session (the addon's own clock stays)
                    lt_note_ = "lap times: " + why + " (car " + std::to_string(c.vid) + "): no times";
                    lt_state_ = 2;
                    return;
                }
                lc.car = car;
                lc.laps = n;
                // the player's car: its lap in progress feeds the qualifying card's splits
                lc.next = now + (int(c.vid) == me ? 0.1 : 2.0);
            }
            c.best_ms = lc.car.best_ms;
            c.last_ms = lc.car.last_ms;
            for (int k = 0; k < 3; k++) {
                c.best_sec[k] = lc.car.best_sec[k];
                c.pb_sec[k] = lc.car.pb_sec[k];
            }
            with += lc.car.best_ms > 0;
        }
        snap.s.game_times = true;
        lt_note_ = "lap times: the game's, " + std::to_string(with) + " cars with a time";
    }

    MyLapTracker my_;
    int lt_state_ = 0; // 0 to check, 1 this build, 2 off (another build, or something didn't look right)
    uintptr_t lt_system_ = 0;
    double lt_search_ = 0, lt_check_ = 0;
    std::string lt_name_, lt_note_;
    std::array<LtCar, 32> lt_cars_{};

    void champ_reset()
    {
        champ_.clear();
        champ_state_ = 0;
        champ_tries_ = 0;
        champ_next_ = 0;
    }

    void champ_step(Snapshot &snap, double now)
    {
        if (champ_state_ == 1) {
            snap.champ = champ_;
            return;
        }
        if (champ_state_ == 2 || now < champ_next_)
            return;
        champ_next_ = now + 2.0;
        uintptr_t e = reinterpret_cast<uintptr_t>(swofty::career::entity());
        if (!e) { // not a career (a Grand Prix): no championship
            champ_state_ = 2;
            champ_note_ = "championship: not a career";
            return;
        }
        // every read guarded: a wrong pointer gives false, never a fault
        auto rd = [](uintptr_t at, void *dst, size_t n) {
            if (!at || !n || !swofty::mem::readable(at, n))
                return false;
            std::memcpy(dst, reinterpret_cast<const void *>(at), n);
            return true;
        };
        std::vector<ChampEntry> table;
        std::unordered_map<uint32_t, int> cars;
        std::unordered_set<uint32_t> known;
        long human = -1;
        std::string why;
        bool ok = standings::read_table(rd, e, table, why) && standings::read_cars(rd, e, cars, why, &known, &human);
        if (ok)
            ok = champ_map(table, cars, known, snap, why);
        if (ok) {
            champ_ = std::move(table);
            champ_state_ = 1;
            snap.champ = champ_;
            size_t on = 0;
            std::string off;
            for (const ChampEntry &t : champ_) {
                on += t.vid >= 0;
                if (t.vid < 0 && off.size() < 120)
                    off += (off.empty() ? "" : ", ") + std::to_string(t.stat);
            }
            champ_note_ = "championship: " + std::to_string(champ_.size()) + " drivers, leader " +
                          std::to_string(champ_.front().points) + " pts, " + std::to_string(on) + " on track" +
                          (off.empty() ? "" : " (not here: stat " + off + ")") +
                          (human >= 0 ? ", player stat " + std::to_string(human) : "");
        } else if (++champ_tries_ >= 10) {
            champ_state_ = 2;
            champ_note_ = "championship: not shown (" + why + ")";
        }
    }

    // each driver's car; most of this session's cars must be found, or the mapping is not this grid. The player's
    // car left without a driver takes the one table entry that no entrant row knows (a driver the player created
    // whose row the table names another way)
    static bool champ_map(std::vector<ChampEntry> &table, const std::unordered_map<uint32_t, int> &cars,
                          const std::unordered_set<uint32_t> &known, const Snapshot &snap, std::string &why)
    {
        std::array<bool, 32> here{};
        for (const CarIn &c : snap.cars)
            if (c.vid < here.size())
                here[c.vid] = true;
        size_t mapped = 0;
        for (ChampEntry &t : table) {
            auto it = cars.find(t.stat);
            t.vid = it != cars.end() && it->second >= 0 && it->second < 32 && here[size_t(it->second)] ? it->second : -1;
            mapped += t.vid >= 0;
        }
        int me = -1;
        for (const CarIn &c : snap.cars)
            if (c.me)
                me = int(c.vid);
        bool me_mapped = false;
        for (const ChampEntry &t : table)
            me_mapped = me_mapped || (me >= 0 && t.vid == me);
        if (me >= 0 && !me_mapped) {
            ChampEntry *only = nullptr;
            int unknown = 0;
            for (ChampEntry &t : table)
                if (t.vid < 0 && !known.count(t.stat)) {
                    only = &t;
                    unknown++;
                }
            if (unknown == 1) {
                only->vid = me;
                mapped++;
            }
        }
        if (mapped * 10 < snap.cars.size() * 8) {
            why = "only " + std::to_string(mapped) + " of " + std::to_string(snap.cars.size()) + " cars found in it";
            return false;
        }
        return true;
    }

    struct Badge {
        std::string key;
        double next = 0;
    };

    // Team badges change only with the car set; ask again every 2 s while a car has none.
    const std::string &badge(uint32_t vid, double now)
    {
        Badge &b = badges_[vid];
        if (now >= b.next) {
            auto k = live::team_badge(vid);
            if (k)
                b.key = *k;
            b.next = now + (b.key.empty() ? 2.0 : 30.0);
        }
        return b.key;
    }

    LapClock clock_;
    Stints stints_;
    std::vector<LapSample> samples_;
    std::array<Badge, live::max_cars> badges_{};
    int last_type_ = -1;
    uint32_t last_gen_ = ~0u;
    std::vector<ChampEntry> champ_;
    int champ_state_ = 0, champ_tries_ = 0; // 0 to read, 1 read, 2 not available this session
    double champ_next_ = 0;
    std::string champ_note_;
};

} // namespace tvt
