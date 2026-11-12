#!/usr/bin/env python3
"""Lists a PE DLL's imported modules and exports; fails when a network library is imported.

    python tools/pe_imports.py build/tv_timing_tower.dll
"""
import struct
import sys

NETWORK = {"ws2_32.dll", "wsock32.dll", "wininet.dll", "winhttp.dll", "urlmon.dll", "iphlpapi.dll",
           "dnsapi.dll", "mswsock.dll", "websocket.dll", "httpapi.dll"}


def main(path):
    b = open(path, "rb").read()
    pe = struct.unpack_from("<I", b, 0x3C)[0]
    assert b[pe:pe + 4] == b"PE\0\0"
    nsec = struct.unpack_from("<H", b, pe + 6)[0]
    opt = pe + 24
    magic = struct.unpack_from("<H", b, opt)[0]
    dd = opt + (112 if magic == 0x20B else 96)
    exp_rva, _ = struct.unpack_from("<II", b, dd)
    imp_rva, _ = struct.unpack_from("<II", b, dd + 8)
    sec = opt + struct.unpack_from("<H", b, pe + 20)[0]
    sections = []
    for i in range(nsec):
        o = sec + i * 40
        vs, va, raw_sz, raw = struct.unpack_from("<IIII", b, o + 8)
        sections.append((va, max(vs, raw_sz), raw))

    def off(rva):
        for va, sz, raw in sections:
            if va <= rva < va + sz:
                return rva - va + raw
        raise ValueError(hex(rva))

    def cstr(o):
        return b[o:b.index(b"\0", o)].decode("ascii", "replace")

    mods = []
    o = off(imp_rva) if imp_rva else None
    while o is not None:
        name_rva = struct.unpack_from("<I", b, o + 12)[0]
        if not name_rva:
            break
        mods.append(cstr(off(name_rva)).lower())
        o += 20
    exports = []
    if exp_rva:
        e = off(exp_rva)
        n_names = struct.unpack_from("<I", b, e + 24)[0]
        names_rva = struct.unpack_from("<I", b, e + 32)[0]
        for i in range(n_names):
            exports.append(cstr(off(struct.unpack_from("<I", b, off(names_rva) + 4 * i)[0])))
    print("imports:", ", ".join(mods))
    print("exports:", ", ".join(exports))
    bad = sorted(set(mods) & NETWORK)
    if bad:
        print("NETWORK LIBRARIES IMPORTED:", bad)
        return 1
    print("no network library imported")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
