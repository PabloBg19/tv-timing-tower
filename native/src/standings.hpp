// SPDX-License-Identifier: MIT
// TV Timing Tower - the drivers' championship of a career, read from the game's own standings (shown on the formation
// lap, as the broadcast does). Swofty gives addons no standings call, so this follows the chain the SDK's generated
// game headers document (swofty/game/career.hpp, entrants.hpp, marked experimental) and Swofty's own dropped-scores
// code reads (native/src/core/standings_drop.cpp):
//
//   career entity (swofty::career::entity())
//     +0x1058 Championship *        +0x548/+0x550 seasons {_, Season *} of 0x10, +0x560 season index
//   Season +0x2a8 F1SeasonStandings  +0x40/+0x48 the cumulative table: rounds of 0x20 {_, sessions begin, end, _}
//     session of 0x28 {_, entries begin, end, _, id}: entries {u32 key, i32 total}, sorted by the game (total, then
//     the countback, then the key) after every session. The last session of the last round with any is the table
//     as it stands now: every race and sprint so far and the extra points, the totals the game itself shows.
//   career entity +0x2e0/+0x2e8 entrant rows of 0x88 (EntrantRecord): the driver's key at +0x50 and, in a session,
//     his car's vehicle id at +0x82 (0xff without a car; the byte Swofty's live state reads the badges by)
//
// A driver's key is his stat id in the high 16 bits (as in the results Silly Season reads). Every read goes through
// the caller's guarded reader, and anything that does not look right (counts, totals, order, cars) gives up with a
// reason: the formation lap then keeps the usual tower. Nothing is ever written.
#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "model.hpp"

namespace tvt {

namespace standings {

constexpr uintptr_t kChampionship = 0x1058, kSeasonsBegin = 0x548, kSeasonsEnd = 0x550, kSeasonIndex = 0x560;
constexpr size_t kSeasonEntry = 0x10, kSeasonPtr = 0x8;
constexpr uintptr_t kStandings = 0x2a8, kTable = 0x40;
constexpr size_t kRound = 0x20, kSession = 0x28, kEntry = 8;
constexpr uintptr_t kEntrantsBegin = 0x2e0, kEntrantsEnd = 0x2e8, kEntrantsBusy = 0x2f8;
constexpr size_t kEntrant = 0x88, kEntrantDriver = 0x50, kEntrantVid = 0x82;
constexpr size_t kMaxSeasons = 64, kMaxRounds = 64, kMaxSessions = 64, kMaxEntries = 64, kMaxEntrants = 512;

// Read(at, dst, n) copies n bytes at `at` into dst and says whether it could (a guarded read: never faults)
template <class Read> bool ptr(Read &rd, uintptr_t at, uintptr_t &out)
{
    out = 0;
    return rd(at, &out, sizeof out);
}

template <class T, class Read> bool val(Read &rd, uintptr_t at, T &out)
{
    out = T{};
    return rd(at, &out, sizeof out);
}

// a vector's [begin, end) of `stride`-byte items: its count when it looks like one, else -1
template <class Read> long span(Read &rd, uintptr_t field, size_t stride, size_t max, uintptr_t &begin)
{
    uintptr_t b = 0, e = 0;
    if (!ptr(rd, field, b) || !ptr(rd, field + 8, e) || e < b || (e - b) % stride || (e - b) / stride > max ||
        (b == 0 && e != 0) || (b & 7))
        return -1;
    begin = b;
    return long((e - b) / stride);
}

// The table as it stands now, in the game's order; why says what did not look right
template <class Read> bool read_table(Read &rd, uintptr_t entity, std::vector<ChampEntry> &out, std::string &why)
{
    out.clear();
    uintptr_t champ = 0, sb = 0;
    if (!entity || !ptr(rd, entity + kChampionship, champ) || !champ || (champ & 7))
        return why = "no championship", false;
    long seasons = span(rd, champ + kSeasonsBegin, kSeasonEntry, kMaxSeasons, sb);
    uint32_t index = 0;
    if (seasons <= 0 || !val(rd, champ + kSeasonIndex, index) || index >= uint32_t(seasons))
        return why = "no current season", false;
    uintptr_t season = 0;
    if (!ptr(rd, sb + index * kSeasonEntry + kSeasonPtr, season) || !season || (season & 7))
        return why = "no current season", false;
    uintptr_t table = season + kStandings + kTable, rb = 0;
    long rounds = span(rd, table, kRound, kMaxRounds, rb);
    if (rounds < 0)
        return why = "the standings don't look like rounds", false;
    for (long r = rounds - 1; r >= 0; r--) {
        uintptr_t sesb = 0;
        long sessions = span(rd, rb + uintptr_t(r) * kRound + 8, kSession, kMaxSessions, sesb);
        if (sessions < 0)
            return why = "a round doesn't look like sessions", false;
        if (sessions == 0)
            continue;
        uintptr_t eb = 0;
        long n = span(rd, sesb + uintptr_t(sessions - 1) * kSession + 8, kEntry, kMaxEntries, eb);
        if (n < 0)
            return why = "the table doesn't look like entries", false;
        if (n == 0)
            continue;
        std::vector<uint8_t> buf(size_t(n) * kEntry);
        if (!rd(eb, buf.data(), buf.size()))
            return why = "the table is unreadable", false;
        std::unordered_set<uint32_t> seen;
        for (long i = 0; i < n; i++) {
            uint32_t key = 0;
            int32_t total = 0;
            std::memcpy(&key, buf.data() + i * kEntry, 4);
            std::memcpy(&total, buf.data() + i * kEntry + 4, 4);
            if (total < 0 || total > 100000)
                return why = "a total out of range", out.clear(), false;
            if (!out.empty() && total > out.back().points)
                return why = "totals not in the game's order", out.clear(), false;
            if (!seen.insert(key >> 16).second)
                return why = "a driver twice", out.clear(), false;
            out.push_back({key >> 16, total, int(i) + 1, -1});
        }
        if (out.size() < 8)
            return why = std::to_string(out.size()) + " drivers in the table", out.clear(), false;
        return true;
    }
    return why = "no session run yet this season", false;
}

// Each driver's car in this session (stat id -> vehicle id); false while the game is rewriting the rows or when
// they don't look like a grid
template <class Read>
bool read_cars(Read &rd, uintptr_t entity, std::unordered_map<uint32_t, int> &out, std::string &why)
{
    out.clear();
    uint8_t busy = 0;
    uintptr_t b = 0;
    if (!entity || !val(rd, entity + kEntrantsBusy, busy))
        return why = "no entrant rows", false;
    if (busy)
        return why = "the game is updating the entrant rows", false;
    long n = span(rd, entity + kEntrantsBegin, kEntrant, kMaxEntrants, b);
    if (n <= 0)
        return why = "the entrant rows don't look like a list", false;
    std::vector<uint8_t> buf(size_t(n) * kEntrant);
    if (!rd(b, buf.data(), buf.size()))
        return why = "the entrant rows are unreadable", false;
    std::unordered_set<int> cars;
    for (long i = 0; i < n; i++) {
        uint32_t key = 0;
        std::memcpy(&key, buf.data() + i * kEntrant + kEntrantDriver, 4);
        uint8_t vid = buf[size_t(i) * kEntrant + kEntrantVid];
        if (vid >= 32 || (key & 0xffu) != 0)
            continue; // no car, or not a driver's row
        if (!cars.insert(vid).second)
            return why = "two drivers in one car", out.clear(), false;
        out[key >> 16] = vid;
    }
    if (out.size() < 8 || out.size() > 30)
        return why = std::to_string(out.size()) + " drivers with a car", out.clear(), false;
    return true;
}

}

}
