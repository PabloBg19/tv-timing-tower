// SPDX-License-Identifier: MIT
// TV Timing Tower - guarded reads of the game's lap times (laptimes.hpp) with a region cache.
// Swofty's mem::readable caches its VirtualQuery answers for 250 ms per page; the lap times touch a few dozen
// scattered pages (the registry, the car list, one record per car), so it kept asking again: about 45 queries a
// second at ~1.4 ms each on the game thread (Swofty's perf log, 0.18.x: tv_timing_tower.dll 2,700 calls, 3.9 s a
// minute). Here one VirtualQuery answers for its whole region (the game's heap blocks hold many records), kept for
// kTtlMs and dropped at once on a session change (invalidate()), when the game may free what we were reading.
#pragma once
#include <array>
#include <cstdint>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

namespace tvt {

class GuardedReader {
public:
    static constexpr uint64_t kTtlMs = 2000;

    void invalidate() { n_ = 0; }

    bool operator()(uintptr_t at, void *dst, size_t n)
    {
        if (!at || !n || at + n < at)
            return false;
        for (uintptr_t p = at, end = at + n; p < end;) {
            uintptr_t hi = 0;
            if (!covered(p, hi) && !query(p, hi))
                return false;
            p = hi;
        }
        std::memcpy(dst, reinterpret_cast<const void *>(at), n);
        return true;
    }

    uint32_t queries() const { return queries_; } // VirtualQuery calls so far (for the log)

private:
    struct Region {
        uintptr_t lo = 0, hi = 0;
        uint64_t until = 0;
    };

    static uint64_t now_ms()
    {
#ifdef _WIN32
        return GetTickCount64();
#else
        return 0;
#endif
    }

    bool covered(uintptr_t p, uintptr_t &hi)
    {
        const uint64_t t = now_ms();
        for (size_t i = 0; i < n_; i++)
            if (p >= r_[i].lo && p < r_[i].hi && t < r_[i].until) {
                hi = r_[i].hi;
                return true;
            }
        return false;
    }

    bool query(uintptr_t p, uintptr_t &hi)
    {
#ifdef _WIN32
        MEMORY_BASIC_INFORMATION mbi{};
        queries_++;
        if (!VirtualQuery(reinterpret_cast<LPCVOID>(p), &mbi, sizeof mbi) || mbi.State != MEM_COMMIT)
            return false;
        const DWORD pr = mbi.Protect;
        if ((pr & (PAGE_GUARD | PAGE_NOACCESS)) ||
            !(pr & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
                    PAGE_EXECUTE_WRITECOPY)))
            return false;
        const uintptr_t lo = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
        hi = lo + mbi.RegionSize;
        if (hi <= p)
            return false;
        Region &slot = r_[n_ < r_.size() ? n_++ : (next_++ % r_.size())];
        slot = Region{lo, hi, now_ms() + kTtlMs};
        return true;
#else
        (void)p;
        (void)hi;
        return false;
#endif
    }

    std::array<Region, 32> r_{};
    size_t n_ = 0, next_ = 0;
    uint32_t queries_ = 0;
};

} // namespace tvt
