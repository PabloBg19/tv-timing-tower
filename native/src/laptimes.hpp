// SPDX-License-Identifier: MIT
// TV Timing Tower - every car's lap times, read from the game's own lap times system: the times the game itself shows
// on its timing screens and sends as UDP "Session History", simulated laps (time skipped in the garage) included.
// Swofty gives addons no lap times, so this follows the game the way Swofty's own live state does (its registry walk,
// native/src/core/live/live_state.hpp `find`), with the layout read off the game's UDP history builder
// (Game_TelemetryUdp_UpdateCarSessionHistory 0x142cfe4f0) and its per-car getter (Sess_LapTimes_GetCarData 0x142f0c7f0):
//
//   g_Game_Manager (0x14604c2c8) -> manager +0x30 the system registry: {key, system} slots of 0x10 from +0x18 to the
//     used-byte array at +0x20; the lap times system is the slot whose key is 0x146096ae8 (its getter 0x140606320)
//   system +0x58/+0x60  one record pointer per vehicle id; the car's data is record +0x38
//   car data +0x08 + lap * 0xc8   200 laps of 0xc8; car data +0x9c48 the laps completed (byte)
//   a lap: sector 1/2/3 at +0x08/+0x18/+0x28 and the lap at +0xa8, each a float of seconds XOR'ed with the dword 8
//     bytes after it; the lap's valid flag is the byte at +0x98 equal to bit 0 of the qword at +0xa0
//   the game's best lap is the fastest valid lap with all three sectors (the UDP builder's own rule)
//
// Only on the build these addresses were read from (the PE timestamp below) and only when the registry key names
// "VehicleLapTimesSystem"; every read is guarded and every time checked (range, sectors adding up to the lap). Anything
// else: no times, as before. Nothing is ever written.
#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace tvt::laptimes {

constexpr uintptr_t kImageBase = 0x140000000;        // the addresses below are this build's (fixed base, no ASLR)
constexpr uint32_t kPeStamp = 0x6aa3c898;            // F1_25.exe Steam, sha256 d873a1c8d2044cef...
constexpr uintptr_t kGameManager = 0x14604c2c8;      // g_Game_Manager
constexpr uintptr_t kSystemKey = 0x146096ae8;        // the lap times system's registry key
constexpr const char *kSystemName = "VehicleLapTimesSystem";
constexpr uintptr_t kRegistry = 0x30, kSlots = 0x18, kInfo = 0x20;
constexpr size_t kMaxSlots = 4096;
constexpr uintptr_t kCars = 0x58, kCarsEnd = 0x60, kCarData = 0x38;
constexpr uintptr_t kLap0 = 0x08, kLapCount = 0x9c48;
constexpr size_t kLapSize = 0xc8, kMaxLaps = 200, kMaxCars = 64;
constexpr uintptr_t kS1 = 0x08, kS2 = 0x18, kS3 = 0x28, kTime = 0xa8, kValid = 0x98;

struct Car {
    int laps = 0;      // laps the game has recorded for the car
    int best_ms = -1;  // its best valid lap, -1 none
    int last_ms = -1;  // its last completed lap (valid or not), -1 none
    bool last_valid = false;
};

// a float of seconds stored XOR'ed with the dword 8 bytes after it
inline float xor_float(const uint8_t *p)
{
    uint32_t a = 0, b = 0;
    std::memcpy(&a, p, 4);
    std::memcpy(&b, p + 8, 4);
    a ^= b;
    float f;
    std::memcpy(&f, &a, 4);
    return f;
}

inline bool flag(const uint8_t *p)
{
    uint64_t q = 0;
    std::memcpy(&q, p + 8, 8);
    return p[0] == (q & 1);
}

// a time the game could have recorded: finite, between 10 s and 10 min
inline bool sane(float s) { return std::isfinite(s) && s > 10.f && s < 600.f; }
inline bool sector_sane(float s) { return std::isfinite(s) && s > 0.5f && s < 600.f; }

// Read(at, dst, n): a guarded read. The lap times system, or 0 with `why`.
template <class Read> uintptr_t find_system(Read &rd, uintptr_t base, std::string &why)
{
    auto at = [&](uintptr_t va) { return va - kImageBase + base; };
    uintptr_t mgr = 0, reg = 0, sl[2] = {};
    if (!rd(at(kGameManager), &mgr, 8) || !mgr || (mgr & 7))
        return why = "no game manager", 0;
    if (!rd(mgr + kRegistry, &reg, 8) || !reg || (reg & 7) || !rd(reg + kSlots, sl, sizeof sl))
        return why = "no system registry", 0;
    const uintptr_t slots = sl[0], info = sl[1];
    if (!slots || (slots & 7) || info <= slots || (info - slots) % 16 || (info - slots) / 16 > kMaxSlots)
        return why = "the system registry doesn't look like one", 0;
    const uintptr_t key = at(kSystemKey);
    const size_t n = (info - slots) / 16;
    uintptr_t chunk[2 * 64];
    for (size_t i = 0; i < n; i += 64) {
        const size_t k = n - i < 64 ? n - i : 64;
        if (!rd(slots + i * 16, chunk, k * 16))
            return why = "the system registry is unreadable", 0;
        for (size_t j = 0; j < k; j++) {
            const uintptr_t value = chunk[2 * j + 1];
            uint8_t used = 0;
            if (chunk[2 * j] == key && value && !(value & 7) && rd(info + i + j, &used, 1) && used)
                return value;
        }
    }
    return why = "no lap times system in the registry", 0;
}

// The registry key's class name (its type record +8): the check that the key is the lap times system's
template <class Read> std::string key_name(Read &rd, uintptr_t base)
{
    uintptr_t p = 0;
    if (!rd(kSystemKey - kImageBase + base + 8, &p, 8) || !p)
        return {};
    char s[48] = {};
    for (size_t i = 0; i + 1 < sizeof s; i++) {
        if (!rd(p + i, &s[i], 1))
            return {};
        if (!s[i])
            break;
        if (s[i] < 0x20 || s[i] > 0x7e)
            return {};
    }
    return s;
}

// The laps the game has recorded for vehicle `vid` (cheap: one byte), -1 when the car has no record
template <class Read> int lap_count(Read &rd, uintptr_t system, uint32_t vid)
{
    uintptr_t v[2] = {};
    if (!rd(system + kCars, v, sizeof v) || !v[0] || v[1] < v[0] || (v[1] - v[0]) % 8 || (v[1] - v[0]) / 8 > kMaxCars)
        return -1;
    if (vid >= (v[1] - v[0]) / 8)
        return -1;
    uintptr_t rec = 0;
    uint8_t n = 0;
    if (!rd(v[0] + vid * 8, &rec, 8) || !rec || (rec & 7) || !rd(rec + kCarData + kLapCount, &n, 1) || n > kMaxLaps)
        return -1;
    return n;
}

// Every lap of vehicle `vid`: its best valid lap and its last lap. False when the record can't be read or doesn't
// look right (`why`); a car with no lap yet is fine (laps 0).
template <class Read> bool read_car(Read &rd, uintptr_t system, uint32_t vid, Car &out, std::string &why)
{
    out = Car{};
    uintptr_t v[2] = {};
    if (!rd(system + kCars, v, sizeof v) || !v[0] || v[1] < v[0] || (v[1] - v[0]) % 8 || (v[1] - v[0]) / 8 > kMaxCars)
        return why = "the lap times system's cars don't look like a list", false;
    if (vid >= (v[1] - v[0]) / 8)
        return true; // no record for this car (yet)
    uintptr_t rec = 0;
    if (!rd(v[0] + vid * 8, &rec, 8))
        return why = "a car record pointer is unreadable", false;
    if (!rec)
        return true;
    if (rec & 7)
        return why = "a car record pointer is misaligned", false;
    const uintptr_t data = rec + kCarData;
    uint8_t n = 0;
    if (!rd(data + kLapCount, &n, 1) || n > kMaxLaps)
        return why = "a car's lap count is out of range", false;
    out.laps = n;
    if (!n)
        return true;
    std::vector<uint8_t> buf(size_t(n) * kLapSize);
    if (!rd(data + kLap0, buf.data(), buf.size()))
        return why = "a car's laps are unreadable", false;
    float best = 0;
    for (int i = 0; i < n; i++) {
        const uint8_t *lap = buf.data() + size_t(i) * kLapSize;
        const float t = xor_float(lap + kTime), s1 = xor_float(lap + kS1), s2 = xor_float(lap + kS2),
                    s3 = xor_float(lap + kS3);
        const bool sectors = sector_sane(s1) && sector_sane(s2) && sector_sane(s3);
        // a recorded lap must add up: its three sectors are its time (a few ms of rounding)
        if (sane(t) && sectors && std::fabs(s1 + s2 + s3 - t) > 0.05f)
            return why = "a lap whose sectors don't add up to it", out = Car{}, false;
        if (i == n - 1 && sane(t)) {
            out.last_ms = int(std::lround(t * 1000.0));
            out.last_valid = flag(lap + kValid);
        }
        if (sane(t) && sectors && flag(lap + kValid) && (best == 0 || t < best))
            best = t;
    }
    if (best > 0)
        out.best_ms = int(std::lround(best * 1000.0));
    return true;
}

} // namespace tvt::laptimes
