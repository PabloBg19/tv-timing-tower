// SPDX-License-Identifier: MIT
// TV Timing Tower - lap times measured from the live progress (the core's live state has no lap times yet).
// A lap is timed between two observed crossings of the line (the game's lap counter going up), each crossing
// placed between two samples by distance. A clock that stops while the game is paused keeps pauses out of
// the times. A lap is not counted when the car used the pit lane during it, when its track-limit warnings went
// up during it (the game invalidates those laps), or when the session jumped (flashback, restart).
#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace tvt {

struct LapSample {
    uint32_t vid = 0;
    int lap = 0;          // the game's counter
    float lap_m = 0;      // metres into the lap
    bool in_pit = false;
    int warnings = 0;     // track-limit warnings
};

class LapClock {
public:
    static constexpr int kCars = 32;
    static constexpr int kMaxLaps = 96;

    void reset()
    {
        cars_.fill(Car{});
        sim_ = 0;
        pending_ = 0;
        fastest_vid_ = -1;
        fastest_ms_ = -1;
        epoch_++;
    }

    // dt: seconds since the last call. Call once per new snapshot of the game.
    void step(double dt, float track_m, const std::vector<LapSample> &cars)
    {
        bool moved = false;
        int jumps = 0, seen = 0;
        std::array<bool, kCars> back{};
        for (const LapSample &s : cars) {
            if (s.vid >= kCars)
                continue;
            const Car &c = cars_[s.vid];
            if (!c.seen)
                continue;
            seen++;
            if (s.lap != c.lap || std::fabs(s.lap_m - c.lap_m) > 0.001f)
                moved = true;
            // a car going back: its own reset (back to the garage, a teleport to the pit lane) ...
            if (s.lap < c.lap || (s.lap == c.lap && s.lap_m < c.lap_m - 40.f)) {
                back[s.vid] = true;
                jumps++;
            }
        }
        // ... or, when most of the field goes back at once, a flashback or a restart
        const bool jumped = seen >= 4 && jumps * 2 > seen;
        if (moved) {
            sim_ += (pending_ <= 0.35 ? pending_ : 0.0) + dt; // short stalls are frames between physics updates
            pending_ = 0;
        } else {
            pending_ += dt; // paused, or every car stopped: the clock waits
        }
        if (jumped) { // a flashback or a restart: every lap in progress is unusable
            for (Car &c : cars_)
                c.cross_t = -1;
        }
        for (const LapSample &s : cars)
            if (s.vid < kCars && back[s.vid] && cars_[s.vid].seen) { // that car's lap in progress is unusable
                cars_[s.vid].cross_t = -1;
                cars_[s.vid].dirty = true;
            }

        for (const LapSample &s : cars) {
            if (s.vid >= kCars)
                continue;
            Car &c = cars_[s.vid];
            if (!c.seen) {
                c.seen = true;
                c.lap = s.lap;
                c.lap_m = s.lap_m;
                c.t = sim_;
                c.warn0 = s.warnings;
                c.dirty = true; // the lap we join in the middle is never timed
                continue;
            }
            if (s.lap == c.lap + 1 && !jumped && !back[s.vid]) {
                double f = 1.0;
                if (track_m > 100.f) {
                    double before = std::fmax(0.0, double(track_m) - c.lap_m);
                    double after = std::fmax(0.0, double(s.lap_m));
                    if (before + after > 0.01)
                        f = before / (before + after);
                }
                double cross = c.t + f * (sim_ - c.t);
                bool valid = c.cross_t >= 0 && !c.dirty && !s.in_pit && s.warnings == c.warn0;
                if (c.cross_t >= 0) {
                    int ms = int(std::lround((cross - c.cross_t) * 1000.0));
                    if (ms < 15000 || ms > 600000)
                        valid = false;
                    if (valid) {
                        c.last_ms = ms;
                        if (c.n < kMaxLaps)
                            c.laps[size_t(c.n++)] = ms;
                        if (c.best_ms < 0 || ms < c.best_ms)
                            c.best_ms = ms;
                        if (fastest_ms_ < 0 || ms < fastest_ms_) {
                            fastest_ms_ = ms;
                            fastest_vid_ = int(s.vid);
                        }
                    } else {
                        c.last_ms = -1;
                    }
                }
                c.cross_t = cross;
                c.dirty = s.in_pit;
                c.warn0 = s.warnings;
            } else if (s.lap != c.lap) {
                c.cross_t = -1; // a counter jump we did not see happen
                c.dirty = true;
                c.warn0 = s.warnings;
            }
            if (s.in_pit)
                c.dirty = true;
            if (s.warnings != c.warn0 && s.lap == c.lap)
                c.dirty = true;
            c.lap = s.lap;
            c.lap_m = s.lap_m;
            c.t = sim_;
        }
    }

    int best_ms(uint32_t vid) const { return vid < kCars ? cars_[vid].best_ms : -1; }
    int last_ms(uint32_t vid) const { return vid < kCars ? cars_[vid].last_ms : -1; }
    int fastest_vid() const { return fastest_vid_; }
    int fastest_ms() const { return fastest_ms_; }
    double sim_time() const { return sim_; }

    // The fastest of the car's valid laps that is not faster than `floor_ms`, -1 for none. The tower uses it so a
    // lap the game threw out (and we could not tell) never puts a car's time ahead of a car the game ranks higher.
    int best_at_least(uint32_t vid, int floor_ms) const
    {
        if (vid >= kCars)
            return -1;
        const Car &c = cars_[vid];
        int best = -1;
        for (int i = 0; i < c.n; i++) {
            int t = c.laps[size_t(i)];
            if (t >= floor_ms && (best < 0 || t < best))
                best = t;
        }
        return best;
    }

private:
    struct Car {
        bool seen = false;
        int lap = 0;
        float lap_m = 0;
        double t = 0;          // sim time of the last sample
        double cross_t = -1;   // sim time of the last crossing, -1 unknown
        bool dirty = true;
        int warn0 = 0;
        int last_ms = -1, best_ms = -1;
        int n = 0;
        std::array<int, kMaxLaps> laps{};
    };
    std::array<Car, kCars> cars_{};
    double sim_ = 0, pending_ = 0;
    int fastest_vid_ = -1, fastest_ms_ = -1;
    uint32_t epoch_ = 0;
};

// Laps on the current set of tyres, counted from the last pit stop seen (or the start of the session).
class Stints {
public:
    void reset() { cars_.fill(Car{}); }

    int age(uint32_t vid, int lap, int stops, bool in_pit)
    {
        if (vid >= cars_.size())
            return -1;
        Car &c = cars_[vid];
        if (!c.seen) {
            c = Car{true, stops, lap, in_pit};
        }
        if (stops > c.stops || (c.was_pit && !in_pit && stops < 0)) {
            c.start_lap = lap;
            c.stops = stops;
        }
        c.was_pit = in_pit;
        int a = lap - c.start_lap;
        return a < 0 ? 0 : a;
    }

private:
    struct Car {
        bool seen = false;
        int stops = 0;
        int start_lap = 0;
        bool was_pit = false;
    };
    std::array<Car, 32> cars_{};
};

} // namespace tvt
