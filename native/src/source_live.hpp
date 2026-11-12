// SPDX-License-Identifier: MIT
// TV Timing Tower - the game's live state (swofty::live, read from memory by Swofty's core, no UDP) turned into
// a model Snapshot. Every value the core marks as not read stays at the model's "unknown" default.
#pragma once
#include <array>
#include <optional>
#include <string>
#include <vector>

#include <swofty/live.hpp>

#include "lap_clock.hpp"
#include "model.hpp"

namespace tvt {

namespace live = swofty::live;

class LiveSource {
public:
    enum class Why { ok, no_core, no_session, no_order, time_trial };

    void reset()
    {
        clock_.reset();
        stints_.reset();
        badges_.fill(Badge{});
        last_type_ = -1;
        last_gen_ = ~0u;
    }

    // Fills `snap` from the live state. `dt` is the frame time; `now` the wall clock (badge refresh).
    Why read(Snapshot &snap, double dt, double now)
    {
        snap = Snapshot{};
        if (!live::available())
            return Why::no_core;
        std::optional<live::Session> so = live::session();
        if (!so || !live::has(*so, SWOFTY_LIVE_SHAVE_TYPE))
            return Why::no_session;
        const live::Session &s = *so;
        if (live::game_mode(s) == live::GameMode::time_trial || s.type == 18)
            return Why::time_trial;
        if (!live::has(s, SWOFTY_LIVE_SHAVE_ORDER) || s.classified == 0)
            return Why::no_order;

        if (int(s.type) != last_type_ || s.generation != last_gen_) {
            if (int(s.type) != last_type_) {
                clock_.reset();
                stints_.reset();
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
        return snap.cars.empty() ? Why::no_order : Why::ok;
    }

    const LapClock &clock() const { return clock_; }

private:
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
};

} // namespace tvt
