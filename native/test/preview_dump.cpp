// SPDX-License-Identifier: MIT
// Runs every demo scenario through the same Tower model the DLL uses and writes the views as JSON for the HTML
// preview (preview/index.html). Usage: preview_dump <out.json>
#include <cstdio>
#include <string>

#include "../src/demo.hpp"
#include "../src/model.hpp"

using namespace tvt;

static std::string esc(const std::string &s)
{
    std::string o;
    for (char c : s) {
        if (c == '"' || c == '\\')
            o += '\\';
        o += c;
    }
    return o;
}

static void row_json(FILE *f, const Row &r, const CarIn *car)
{
    std::fprintf(f,
                 "{\"vid\":%d,\"pos\":\"%s\",\"p1\":%d,\"up\":%d,\"dn\":%d,\"gap\":\"%s\",\"purple\":%d,\"green\":%d,"
                 "\"label\":%d,\"inpit\":%d,\"tyre\":\"%s\",\"age\":\"%s\",\"pit\":%d,\"pen\":\"%s\",\"fl\":%d,\"chq\":%d,"
                 "\"out\":%d,\"ko\":%d,\"kol\":%d,\"elim\":%d,\"sep\":%d,\"me\":%d,\"gain\":%d,\"gon\":%d,\"team\":%d,\"name\":\"%s\","
                 "\"code\":\"%s\",\"st\":\"%s\"}",
                 r.vid, r.pos.c_str(), r.p1, r.up, r.dn, esc(r.gap).c_str(), r.gap_purple, r.gap_green, r.gap_label, r.gap_inpit,
                 r.tyre ? std::string(1, r.tyre).c_str() : "", r.tyre_age.c_str(), r.pit,
                 r.pen ? esc(r.pen_text).c_str() : "", r.fl, r.chq, r.out, r.ko, r.ko_line, r.elim, r.sep, r.me,
                 r.gain, r.gain_on, r.team, car ? car->name.c_str() : "", car ? car->code.c_str() : "",
                 r.start_tyre ? std::string(1, r.start_tyre).c_str() : "");
}

int main(int argc, char **argv)
{
    const char *out = argc > 1 ? argv[1] : "preview/data/frames.json";
    FILE *f = std::fopen(out, "wb");
    if (!f)
        return 1;
    struct Variant {
        const char *name;
        Config cfg;
    };
    Config base;
    Config top10 = base;
    top10.rows = Config::Rows::top10;
    Config compact = base;
    compact.rows = Config::Rows::compact;
    Config codes = base;
    codes.names = Config::Names::code;
    codes.tyre_age = true;
    const Variant variants[] = {{"all", base}, {"top10", top10}, {"compact", compact}, {"codes", codes}};

    std::fprintf(f, "{\"step\":0.5,\"seconds\":%.1f,\"scenarios\":{", demo::kScenarioSeconds);
    for (int sc = 0; sc < demo::count; sc++) {
        std::fprintf(f, "%s\"%s\":{", sc ? "," : "", demo::scenario_name(sc));
        for (size_t vi = 0; vi < sizeof variants / sizeof variants[0]; vi++) {
            std::fprintf(f, "%s\"%s\":[", vi ? "," : "", variants[vi].name);
            Tower t;
            int k = 0;
            for (double time = 0; time <= demo::kScenarioSeconds + 1e-6; time += 0.5, k++) {
                Snapshot s = demo::make(sc, time);
                const View &v = t.update(s, variants[vi].cfg, time);
                const Header &h = v.h;
                std::fprintf(f,
                             "%s{\"t\":%.1f,\"h\":{\"race\":%d,\"sess\":\"%s\",\"lap\":\"%s\",\"laps\":\"%s\","
                             "\"clock\":\"%s\",\"card\":%d,\"abbr\":%d,\"gained\":%d,\"pits\":%d,\"form\":%d,\"tyv\":%d,\"sty\":%d,\"fcl\":%d,\"sprint\":%d,\"ysec\":\"%s\",\"lead\":%d,"
                             "\"ban\":%d,\"bnm\":\"%s\",\"btm\":%d},\"rows\":[",
                             k ? "," : "", time, h.race, h.sess.c_str(), h.lap.c_str(), h.laps_total.c_str(),
                             h.clock.c_str(), h.card, h.abbr, h.gained, h.pitstops, h.formation, h.tyres, h.start_tyres, h.final_cls, int(!h.sub.empty()), h.ysec.c_str(), h.lead,
                             h.banner, h.ban_name.c_str(), h.ban_team);
                for (int i = 0; i < v.shown; i++) {
                    const Row &r = v.rows[size_t(i)];
                    const CarIn *car = nullptr;
                    for (const CarIn &c : s.cars)
                        if (int(c.vid) == r.vid)
                            car = &c;
                    if (i)
                        std::fputc(',', f);
                    row_json(f, r, car);
                }
                std::fprintf(f, "]}");
            }
            std::fprintf(f, "]");
        }
        std::fprintf(f, "}");
    }
    std::fprintf(f, "}}\n");
    std::fclose(f);
    std::printf("preview frames written to %s\n", out);
    return 0;
}
