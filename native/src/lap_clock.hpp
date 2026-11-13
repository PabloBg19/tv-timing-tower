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

    // What happened to the line crossings since the reset, for the log (why a session has no times)
    struct Stats {
        int crossings = 0, timed = 0;
        int first = 0, dirty = 0, pit = 0, warn = 0, range = 0; // crossings not timed, by reason
        int resets = 0, jumps = 0, wraps = 0, skips = 0;        // car resets, flashbacks, early line wraps, counter jumps
    };
    const Stats &stats() const { return stats_; }

    void reset()
    {
        cars_.fill(Car{});
        sim_ = 0;
        pending_ = 0;
        fastest_vid_ = -1;
        fastest_ms_ = -1;
        stats_ = Stats{};
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
            // a car going back: its own reset (back to the garage, a teleport to the pit lane) ... but not the lap
            // distance wrapping at the line a moment before the game's counter goes up
            if (s.lap < c.lap || (s.lap == c.lap && s.lap_m < c.lap_m - 40.f && !line_wrap(c, s, track_m))) {
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
            stats_.jumps++;
            for (Car &c : cars_)
                c.cross_t = -1;
        }
        for (const LapSample &s : cars)
            if (s.vid < kCars && back[s.vid] && cars_[s.vid].seen) { // that car's lap in progress is unusable
                if (!jumped)
                    stats_.resets++;
                cars_[s.vid].cross_t = -1;
                cars_[s.vid].det_t = -1;
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
            // A crossing of the line shows as the game's counter going up and/or the lap distance wrapping from the
            // end of the lap to its start. In qualifying and practice the counter does not move at all (seen in game:
            // 10 wraps, 0 counter steps), in races both happen, a few frames apart in either order. The first of
            // the two is the crossing; the other, within 20 s, is the same one.
            const bool counter = s.lap == c.lap + 1 && !jumped && !back[s.vid];
            const bool wrap = !counter && s.lap == c.lap && line_wrap(c, s, track_m) && !jumped;
            if (counter || wrap) {
                double cross;
                if (wrap) {
                    double before = std::fmax(0.0, double(track_m) - c.lap_m);
                    double after = std::fmax(0.0, double(s.lap_m));
                    double f = before + after > 0.01 ? before / (before + after) : 1.0;
                    cross = c.t + f * (sim_ - c.t);
                    stats_.wraps++;
                } else {
                    double f = 1.0;
                    if (track_m > 100.f) {
                        double before = std::fmax(0.0, double(track_m) - c.lap_m);
                        double after = std::fmax(0.0, double(s.lap_m));
                        if (before + after > 0.01)
                            f = before / (before + after);
                    }
                    cross = c.t + f * (sim_ - c.t);
                    if (track_m > 100.f && s.lap_m > track_m * 0.8f && s.lap_m > c.lap_m && sim_ > c.t) {
                        // the counter went up before the distance wrapped: the line is still ahead, at this speed
                        double v = double(s.lap_m - c.lap_m) / (sim_ - c.t);
                        cross = sim_ + double(track_m - s.lap_m) / v;
                    }
                }
                if (c.det_t < 0 || sim_ - c.det_t >= 20.0) {
                    c.det_t = sim_;
                    cross_at(c, s, cross);
                }
            } else if (s.lap != c.lap) {
                c.cross_t = -1; // a counter jump we did not see happen
                c.det_t = -1;
                c.dirty = true;
                c.warn0 = s.warnings;
                if (!jumped && !back[s.vid])
                    stats_.skips++;
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
    struct Car;
    // one crossing of the line at sim time `cross`: closes the lap in progress (timed when clean) and opens the next
    void cross_at(Car &c, const LapSample &s, double cross);
    // the car went from the end of a lap to its start with the counter unchanged
    static bool line_wrap(const Car &c, const LapSample &s, float track_m)
    {
        return track_m > 100.f && s.lap == c.lap && c.lap_m > track_m * 0.8f && s.lap_m < track_m * 0.2f;
    }

    struct Car {
        bool seen = false;
        int lap = 0;
        float lap_m = 0;
        double t = 0;          // sim time of the last sample
        double cross_t = -1;   // sim time of the last crossing, -1 unknown
        double det_t = -1;     // sim time the last crossing was seen (the counter and the wrap both report it)
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
    Stats stats_;
};

inline void LapClock::cross_at(Car &c, const LapSample &s, double cross)
{
    bool valid = c.cross_t >= 0 && !c.dirty && !s.in_pit && s.warnings == c.warn0;
    stats_.crossings++;
    if (c.cross_t < 0)
        stats_.first++;
    else if (s.in_pit)
        stats_.pit++;
    else if (c.dirty)
        stats_.dirty++;
    else if (s.warnings != c.warn0)
        stats_.warn++;
    if (c.cross_t >= 0) {
        int ms = int(std::lround((cross - c.cross_t) * 1000.0));
        if (valid && (ms < 15000 || ms > 600000))
            stats_.range++;
        if (ms < 15000 || ms > 600000)
            valid = false;
        if (valid) {
            stats_.timed++;
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
}

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
