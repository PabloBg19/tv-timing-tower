// SPDX-License-Identifier: MIT
// TV Timing Tower - the player's lap for the qualifying card (model.hpp Tower::card): the time into the lap and
// the two splits, from the addon's own clock (LapClock: line crossings placed by distance, pauses left out) and the
// sector ends the core gives (SwoftyLiveSession::sector_end_m), each split crossing placed between two samples by
// distance as the line is. The game's own split (laptimes.hpp, the lap in progress) replaces the measured one when
// the two agree within kAgree: the measurement only proves the game's figure is this lap's and not a stale one.
#pragma once
#include <cmath>
#include <cstdint>

#include "model.hpp"

namespace tvt {

struct MySample {
    bool ok = false;          // the player's car was read this frame
    uint32_t vid = 0;
    double sim = 0;           // LapClock::sim_time()
    double cross = -1;        // LapClock::cross_t(vid): its last line crossing, -1 unknown
    float lap_m = 0;
    bool in_pit = false;
    int sector = -1;          // the core's sector index (0..2), -1 unread
    float end[2] = {-1, -1};  // sector 1 and 2 end distances, -1 unknown (then the sector index is used)
    int game_sec[2] = {-1, -1}; // the game's in-progress lap: sector 1 and sector 2 times, -1 none
    int game_last_ms = -1;    // the game's last completed lap of this car, -1 none
};

class MyLapTracker {
public:
    static constexpr int kAgree = 300; // ms: the game's split within this of ours is the same split

    void reset() { *this = MyLapTracker{}; }

    MyLap step(const MySample &s)
    {
        MyLap out;
        if (!s.ok || (vid_ >= 0 && uint32_t(vid_) != s.vid)) {
            const bool same = s.ok;
            reset();
            if (!same)
                return out;
        }
        vid_ = int(s.vid);

        if (s.cross < 0) { // no crossing known (yet, or the lap was thrown out by a reset / flashback)
            cross_ = -1;
            flying_ = false;
        } else if (s.cross != cross_) {
            // a new crossing: the lap that ended was a flying one -> it is done
            if (cross_ >= 0 && flying_) {
                done_ms_ = int(std::lround((s.cross - cross_) * 1000.0));
                done_split_[0] = split_[0];
                done_split_[1] = split_[1];
                done_at_ = s.sim;
                game_last_seen_ = s.game_last_ms;
            }
            cross_ = s.cross;
            // a lap from the line, seen as it happened (a crossing first met long after it, as on joining the
            // session, may have had the pit lane since)
            flying_ = !s.in_pit && s.sim - s.cross < 1.0;
            split_[0] = split_[1] = -1;
            if (flying_)
                laps_++;
            prev_ok_ = false;
        }
        if (s.in_pit)
            flying_ = false;

        // the game's own time for the lap just done, once its record has it (thousandths as the game shows them)
        if (done_ms_ > 0 && s.game_last_ms > 0 && s.game_last_ms != game_last_seen_ &&
            std::abs(s.game_last_ms - done_ms_) <= kAgree) {
            done_ms_ = s.game_last_ms;
            game_last_seen_ = s.game_last_ms;
        }

        if (flying_ && cross_ >= 0) {
            for (int k = 0; k < 2; k++) {
                if (split_[k] >= 0 || (k == 1 && split_[0] < 0))
                    continue;
                double at = -1;
                if (s.end[k] > 0) {
                    if (prev_ok_ && prev_m_ < s.end[k] && s.lap_m >= s.end[k] && s.lap_m - prev_m_ < 400.f) {
                        const double f = double(s.end[k] - prev_m_) / double(s.lap_m - prev_m_);
                        at = prev_t_ + f * (s.sim - prev_t_);
                    }
                } else if (s.sector > k && prev_sector_ == k) {
                    at = s.sim;
                }
                if (at >= cross_) {
                    split_[k] = int(std::lround((at - cross_) * 1000.0));
                    split_at_[k] = at;
                }
            }
            // the game's figures for the same splits: cumulative, checked against ours
            int game_cum = 0;
            for (int k = 0; k < 2; k++) {
                if (s.game_sec[k] <= 0 || split_[k] < 0)
                    break;
                game_cum += s.game_sec[k];
                if (std::abs(game_cum - split_[k]) <= kAgree)
                    split_[k] = game_cum;
            }
        }
        prev_m_ = s.lap_m;
        prev_t_ = s.sim;
        prev_sector_ = s.sector;
        prev_ok_ = true;

        out.known = cross_ >= 0 || done_ms_ > 0;
        out.flying = flying_ && cross_ >= 0;
        out.t_ms = out.flying ? int(std::lround((s.sim - cross_) * 1000.0)) : -1;
        out.split[0] = out.flying ? split_[0] : -1;
        out.split[1] = out.flying ? split_[1] : -1;
        for (int k = 0; k < 2; k++)
            out.split_age[k] = out.split[k] >= 0 ? s.sim - split_at_[k] : -1;
        out.done_ms = done_ms_;
        out.done_split[0] = done_split_[0];
        out.done_split[1] = done_split_[1];
        out.done_age = done_ms_ > 0 ? s.sim - done_at_ : -1;
        out.laps = laps_;
        return out;
    }

private:
    int vid_ = -1;
    double cross_ = -1;
    bool flying_ = false;
    int split_[2] = {-1, -1};
    double split_at_[2] = {0, 0};
    int done_ms_ = -1;
    int done_split_[2] = {-1, -1};
    double done_at_ = 0;
    int game_last_seen_ = -1;
    uint32_t laps_ = 0;
    bool prev_ok_ = false;
    float prev_m_ = 0;
    double prev_t_ = 0;
    int prev_sector_ = -1;
};

} // namespace tvt
