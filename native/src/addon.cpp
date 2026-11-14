// SPDX-License-Identifier: MIT
// TV Timing Tower for F1 25 (Swofty Launcher). Entry point: settings, the frame callback, the hotkey, demo mode and
// Offline only: no network, no hooks, no writes to game memory.
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include <swofty/events.hpp>
#include <swofty/mixin.hpp>

#include "demo.hpp"
#include "leaves.hpp"
#include "model.hpp"
#include "source_live.hpp"

#ifndef TVT_VERSION
#define TVT_VERSION "0.17.3"
#endif

// ---------------------------------------------------------------------------------------------------- settings
SWOFTY_SETTING(bool, enabled, true);
SWOFTY_SETTING(std::string_view, rows, "all");
SWOFTY_SETTING(std::string_view, names, "auto");
SWOFTY_SETTING(std::string_view, gap_mode, "auto");
SWOFTY_SETTING(std::string_view, gap_decimals, "one");
SWOFTY_SETTING(bool, ban_leader, true);
SWOFTY_SETTING(bool, champ_formation, true);
SWOFTY_SETTING(bool, tyres, true);
SWOFTY_SETTING(bool, tyre_age, false);
SWOFTY_SETTING(std::string_view, background, "broadcast");
SWOFTY_SETTING(bool, arrows, true);
SWOFTY_SETTING(bool, team_bar, false);
SWOFTY_SETTING(bool, animations, true);
SWOFTY_SETTING(std::string_view, logos, "tv");
SWOFTY_SETTING(bool, highlight, true);
SWOFTY_SETTING(bool, in_practice, true);
SWOFTY_SETTING(bool, in_quali, true);
SWOFTY_SETTING(bool, in_sprint_quali, true);
SWOFTY_SETTING(bool, in_race, true);
SWOFTY_SETTING(std::string_view, hotkey, "F8");
SWOFTY_SETTING(bool, demo, false);
SWOFTY_SETTING(std::string_view, demo_scenario, "cycle");
SWOFTY_SETTING(bool, debug_log, false);

namespace {

constexpr const char *kTag = "[tv-tower]";

tvt::Config read_config()
{
    tvt::Config c;
    std::string_view r = rows;
    c.rows = r == "top10" ? tvt::Config::Rows::top10 : r == "compact" ? tvt::Config::Rows::compact
                                                                      : tvt::Config::Rows::all;
    std::string_view nm = names;
    c.names = nm == "code" ? tvt::Config::Names::code : nm == "surname" ? tvt::Config::Names::surname
                                                                       : tvt::Config::Names::autoswitch;
    std::string_view g = gap_mode;
    c.gap = g == "interval" ? tvt::Config::Gap::interval : g == "leader" ? tvt::Config::Gap::leader
          : g == "gained"   ? tvt::Config::Gap::gained
          : g == "pitstops" ? tvt::Config::Gap::pitstops
          : g == "tyres"    ? tvt::Config::Gap::tyres    : tvt::Config::Gap::autoswitch;
    c.gap_ms = std::string_view(gap_decimals) == "three";
    c.ban_leader = ban_leader;
    c.champ_formation = champ_formation;
    c.tyres = tyres;
    c.tyre_age = tyre_age;
    std::string_view b = background;
    c.bg = b == "solid" ? 0 : b == "light" ? 2 : 1;
    c.arrows = arrows;
    c.colbar = team_bar;
    std::string_view lg = logos;
    c.logos = lg == "game" ? tvt::Config::Logos::game : lg == "game_white" ? tvt::Config::Logos::game_white
                                                                           : tvt::Config::Logos::tv;
    c.highlight = highlight;
    c.en_practice = in_practice;
    c.en_quali = in_quali;
    c.en_sprint_quali = in_sprint_quali;
    c.en_race = in_race;
    return c;
}

int scenario_from_setting()
{
    std::string_view s = demo_scenario;
    for (int i = 0; i < tvt::demo::count; i++)
        if (s == tvt::demo::scenario_name(i))
            return i;
    return -1; // cycle
}

int vk_from_name(std::string_view k)
{
    if (k.size() >= 2 && (k[0] == 'F' || k[0] == 'f')) {
        int n = 0;
        for (size_t i = 1; i < k.size(); i++) {
            if (k[i] < '0' || k[i] > '9')
                return 0;
            n = n * 10 + (k[i] - '0');
        }
        if (n >= 1 && n <= 12)
            return VK_F1 + n - 1;
    }
    return 0;
}

bool game_in_front()
{
    HWND w = GetForegroundWindow();
    DWORD pid = 0;
    if (w)
        GetWindowThreadProcessId(w, &pid);
    return pid == GetCurrentProcessId();
}

// The addon's own folder (one up from native/), for the team icon list.
std::string addon_dir()
{
    HMODULE self = nullptr;
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCSTR>(&addon_dir), &self))
        return {};
    char path[MAX_PATH] = {};
    DWORD n = GetModuleFileNameA(self, path, MAX_PATH);
    std::string p(path, n);
    for (int up = 0; up < 2; up++) {
        size_t s = p.find_last_of("\\/");
        if (s == std::string::npos)
            return {};
        p.resize(s);
    }
    return p;
}

std::vector<int> read_team_list(const std::string &dir)
{
    std::vector<int> out;
    if (dir.empty())
        return out;
    FILE *f = std::fopen((dir + "\\native\\tvt_logos.txt").c_str(), "rb");
    if (!f)
        return out;
    int v;
    while (std::fscanf(f, "%d", &v) == 1)
        if (v >= 0 && v < tvt::kTeams)
            out.push_back(v);
    std::fclose(f);
    return out;
}

const char *why_name(tvt::LiveSource::Why w)
{
    switch (w) {
    case tvt::LiveSource::Why::ok: return "live";
    case tvt::LiveSource::Why::no_core: return "no live state (Swofty core without it, or this game build is not verified)";
    case tvt::LiveSource::Why::no_session: return "no session";
    case tvt::LiveSource::Why::no_order: return "no classification yet";
    case tvt::LiveSource::Why::time_trial: return "Time Trial (the game's own board stays)";
    }
    return "?";
}

// Settings are read through the core's API (a lookup by name each); they change only from ADDON SETTINGS, so the
// frame callback reads them four times a second, not every frame.
struct Settings {
    tvt::Config cfg;
    bool enabled = true, demo = false, debug = false, anim = true;
    int vk = 0;
    int scenario = -1;
};

Settings load_settings()
{
    Settings s;
    s.cfg = read_config();
    s.enabled = enabled;
    s.demo = demo;
    s.debug = debug_log;
    s.anim = animations;
    s.vk = vk_from_name(std::string_view(hotkey));
    s.scenario = scenario_from_setting();
    return s;
}

struct State {
    tvt::Publisher *pub = nullptr;
    tvt::Tower tower;
    tvt::LiveSource src;
    Settings set;
    double next_settings = 0;
    double now = 0;
    double acc = 0;             // time since the last tower tick
    bool ticked = false;
    bool hidden = false;        // hotkey
    bool demo_key = false;      // Ctrl + hotkey
    int demo_pick = -2;         // Shift + hotkey steps scenarios; -2 follows the setting
    int last_scenario = -99;
    std::string last_champ_note;
    double demo_t0 = 0;
    int key_prev = 0;
    tvt::LiveSource::Why last_why = tvt::LiveSource::Why::no_session;
    bool was_on = false;
    double cost_sum = 0, cost_max = 0;
    int cost_n = 0;
    double next_cost_log = 60;
};

State *g = nullptr;

// The tower's data is read, modelled and published 30 times a second: smoother than the board needs, and the
// game's own HUD refreshes its values at that order. The hotkey is polled every frame.
constexpr double kTick = 1.0 / 30.0;

void handle_keys()
{
    int vk = g->set.vk;
    if (!vk || !game_in_front()) {
        g->key_prev = 0;
        return;
    }
    int down = (GetAsyncKeyState(vk) & 0x8000) ? 1 : 0;
    if (down && !g->key_prev) {
        bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        if (ctrl) {
            g->demo_key = !g->demo_key;
            swofty::log("%s demo mode %s (hotkey)", kTag, g->demo_key ? "on" : "off");
        } else if (shift) {
            int cur = g->demo_pick >= 0 ? g->demo_pick : (g->last_scenario >= 0 ? g->last_scenario : 0);
            g->demo_pick = (cur + 1) % tvt::demo::count;
            swofty::log("%s demo scenario %s", kTag, tvt::demo::scenario_name(g->demo_pick));
        } else if (g->last_why == tvt::LiveSource::Why::ok || g->set.demo || g->demo_key) {
            // only in a session: a press in the menus (where the tower can't be seen) used to hide it unnoticed
            g->hidden = !g->hidden;
            swofty::log("%s tower %s (hotkey)", kTag, g->hidden ? "hidden" : "shown");
        }
        g->acc = kTick; // apply at once
    }
    g->key_prev = down;
}

void frame(double dt, void *)
{
    if (!g || !g->pub)
        return;
    if (dt < 0 || dt > 1.0)
        dt = 0.0;
    g->now += dt;
    if (g->now >= g->next_settings) {
        g->set = load_settings();
        g->next_settings = g->now + 0.25;
    }
    handle_keys();
    g->acc += dt;
    if (g->ticked && g->acc < kTick)
        return;
    const double step = g->acc;
    g->acc = 0;
    g->ticked = true;

    LARGE_INTEGER f, t0, t1;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&t0);

    tvt::Config cfg = g->set.cfg;
    tvt::Snapshot snap;
    bool demo_on = g->set.demo || g->demo_key;
    bool have = false;

    if (demo_on) {
        // the demo draws wherever the game shows its HUD (any session, Time Trial included)
        int sc = g->demo_pick >= 0 ? g->demo_pick : g->set.scenario;
        double t = g->now - g->demo_t0;
        if (sc < 0) { // cycle
            int k = int(g->now / tvt::demo::kScenarioSeconds) % tvt::demo::count;
            sc = k;
            t = g->now - double(int(g->now / tvt::demo::kScenarioSeconds)) * tvt::demo::kScenarioSeconds;
        }
        if (sc != g->last_scenario) {
            g->tower.reset_session();
            g->demo_t0 = g->now;
            t = 0;
            g->last_scenario = sc;
        }
        if (t > tvt::demo::kScenarioSeconds && g->demo_pick >= 0) {
            g->demo_t0 = g->now; // a fixed scenario loops
            g->tower.reset_session();
            t = 0;
        }
        snap = tvt::demo::make(sc, t);
        have = true;
        cfg.en_practice = cfg.en_quali = cfg.en_sprint_quali = cfg.en_race = true;
    } else {
        if (g->last_scenario != -99) {
            g->tower.reset_session();
            g->last_scenario = -99;
        }
        tvt::LiveSource::Why why = g->src.read(snap, step, g->now);
        if (why != g->last_why) {
            swofty::log("%s source: %s", kTag, why_name(why));
            // a new session shows the tower again: F8 hides it for the session it was pressed in
            if (why == tvt::LiveSource::Why::ok && g->hidden) {
                g->hidden = false;
                swofty::log("%s tower shown again (new session)", kTag);
            }
            g->last_why = why;
        }
        have = why == tvt::LiveSource::Why::ok;
        // what came of the championship read, once a session (standings.hpp)
        if (g->src.champ_note() != g->last_champ_note) {
            g->last_champ_note = g->src.champ_note();
            if (!g->last_champ_note.empty())
                swofty::log("%s %s", kTag, g->last_champ_note.c_str());
        }
    }

    bool show = g->set.enabled && !g->hidden && have;
    if (show) {
        const tvt::View &v = g->tower.update(snap, cfg, g->now);
        show = v.h.on;
        if (show)
            g->pub->publish(v, cfg, g->now, g->set.anim);
    }
    if (!show && g->was_on)
        g->pub->off();
    else if (!show && !g->was_on && g->cost_n == 0)
        g->pub->off(); // first tick: make sure the stock board is the one shown
    if (show != g->was_on && g->set.debug)
        swofty::log("%s tower %s", kTag, show ? "on" : "off");
    g->was_on = show;

    QueryPerformanceCounter(&t1);
    double us = double(t1.QuadPart - t0.QuadPart) * 1e6 / double(f.QuadPart);
    g->cost_sum += us;
    g->cost_max = us > g->cost_max ? us : g->cost_max;
    g->cost_n++;
    if (g->now >= g->next_cost_log) {
        if (g->set.debug)
            swofty::log("%s tick cost %.0f us mean, %.0f us max over %d ticks (30 a second)", kTag,
                        g->cost_sum / g->cost_n, g->cost_max, g->cost_n);
        g->cost_sum = g->cost_max = 0;
        g->cost_n = 1;
        g->next_cost_log = g->now + 60;
    }
}

} // namespace

SWOFTY_ON_CHANGE(enabled, bool on)
{
    swofty::log("%s %s in ADDON SETTINGS", kTag, on ? "enabled" : "disabled");
}

SWOFTY_ON_LOAD
{
    static State state;
    g = &state;
    std::string dir = addon_dir();
    std::vector<int> teams = read_team_list(dir);
    static tvt::Publisher pub(teams);
    g->pub = &pub;
    swofty::log("%s TV Timing Tower %s loaded: %zu leaves, %zu broadcast team icons, logos '%.*s', hotkey %.*s", kTag,
                TVT_VERSION, pub.L.size(), teams.size(), int(std::string_view(logos).size()),
                std::string_view(logos).data(), int(std::string_view(hotkey).size()), std::string_view(hotkey).data());

    if (!tvt::live::available())
        swofty::log("%s the live state is not available yet (needs Swofty core with 'telemetry'; off on unverified game "
                    "builds). The game's own board stays until it is.", kTag);

    auto r = swofty::events::on_frame(frame);
    if (r != swofty::events::Result::ok)
        swofty::log("%s could not register the frame callback (%d): the tower stays off", kTag, int(r));

}

SWOFTY_ADDON()
