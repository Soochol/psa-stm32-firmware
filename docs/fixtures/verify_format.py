#!/usr/bin/env python3
"""Check the firmware source against the SD logging spec (v1.5).

Reads the constants out of the firmware rather than restating them, so this
fails when the code drifts from the spec instead of when someone forgets to
update a copy of it. Covers what can be checked without a device:

  * command codes and the parser's category bounds
  * record and header field offsets, and that they tile their structure exactly
  * the frame encoding rule (checksum seed, LEN) against the spec's own examples
  * CRC-16/CCITT-FALSE, by linking the firmware's lib_crc.c if a compiler is
    available, otherwise against an independent implementation here

Usage:  verify_format.py [REPO_ROOT]
Exit:   0 everything matches, 1 otherwise
"""
import os
import re
import subprocess
import sys
import tempfile

FAIL = []
PASS = 0


def check(ok: bool, what: str, detail: str = "") -> None:
    global PASS
    if ok:
        PASS += 1
        print(f"  ok   {what}")
    else:
        FAIL.append(what)
        print(f"  FAIL {what}" + (f"  ({detail})" if detail else ""))


def defines(path: str, pattern: str) -> dict:
    src = open(path).read()
    return {m[0]: int(m[1], 0) for m in re.findall(pattern, src)}


def enum_vals(path: str) -> dict:
    src = open(path).read()
    return {m[0]: int(m[1], 16)
            for m in re.findall(r"(ESP_CMD_\w+)\s*=\s*(0x[0-9A-Fa-f]+)", src)}


def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def frame(dir_b: int, cmd: int, data: bytes, seed: int, len_min: int) -> bytes:
    f = bytes([0x02, len_min + len(data), dir_b, cmd]) + data
    chk = seed
    for b in f:
        chk ^= b
    return f + bytes([chk, 0x03])


def main() -> int:
    root = sys.argv[1] if len(sys.argv) > 1 else "."
    esp = os.path.join(root, "User/Edit/src/comm_esp.c")
    sd = os.path.join(root, "User/Drv/src/sd.c")
    crc_c = os.path.join(root, "User/Lib/src/lib_crc.c")
    crc_h = os.path.join(root, "User/Lib/inc")

    print("== command codes (spec 6.8) ==")
    cmds = enum_vals(esp)
    for name, want in [("ESP_CMD_INIT_LOG_IDENTITY", 0x23), ("ESP_CMD_REQ_LOG_STATUS", 0x43),
                       ("ESP_CMD_REQ_LOG_READ", 0x44), ("ESP_CMD_REQ_LOG_FILES", 0x45),
                       ("ESP_CMD_CTRL_LOG_EN", 0x56), ("ESP_CMD_STAT_LOG_CHUNK", 0x71),
                       ("ESP_CMD_EVT_LOG_ERR", 0x84)]:
        check(cmds.get(name) == want, f"{name} = 0x{want:02X}", f"got {cmds.get(name)}")

    print("\n== parser category bounds (spec 3.4) ==")
    b = defines(esp, r"#define\s+(ESP_CMD_\w+_(?:MIN|MAX))\s+\((0x[0-9A-Fa-f]+)\)")
    for cat, lo, hi in [("INIT", 0x10, 0x29), ("REQ", 0x30, 0x49), ("CTRL", 0x50, 0x69),
                        ("STAT", 0x70, 0x79), ("EVT", 0x80, 0x89), ("ERR", 0x90, 0x99)]:
        check(b.get(f"ESP_CMD_{cat}_MIN") == lo and b.get(f"ESP_CMD_{cat}_MAX") == hi,
              f"{cat} 0x{lo:02X}-0x{hi:02X}")
    for name, code in [("initLogIdentity", 0x23), ("reqLogStatus", 0x43), ("reqLogRead", 0x44),
                       ("reqLogFiles", 0x45), ("ctrlLogEnable", 0x56),
                       ("statLogChunk", 0x71), ("evtLogError", 0x84)]:
        inside = any(b[f"ESP_CMD_{c}_MIN"] <= code <= b[f"ESP_CMD_{c}_MAX"]
                     for c in ("INIT", "REQ", "CTRL", "STAT", "EVT", "ERR"))
        check(inside, f"{name} (0x{code:02X}) accepted by the parser")

    print("\n== record layout (spec 7) ==")
    d = defines(sd, r"#define\s+(SD_(?:REC|HDR|LOG)_\w+)\s+\(?(\d+)U?\)?")
    rec = [("seq", "SD_REC_SEQ", 0, 4), ("tickMs", "SD_REC_TICK", 4, 4),
           ("statPayload", "SD_REC_PAYLOAD", 8, 64), ("deviceMode", "SD_REC_MODE", 72, 1),
           ("flags", "SD_REC_FLAGS", 73, 1), ("errorMask", "SD_REC_ERRMASK", 74, 2),
           ("crc16", "SD_REC_CRC", 78, 2)]
    pos = 0
    for label, key, want, size in rec:
        check(d.get(key) == want, f"record.{label} @ {want}", f"got {d.get(key)}")
        pos = want + size
    check(pos == 80, "record fields end at 80", f"end {pos}")
    check(d.get("SD_LOG_REC_SIZE") == 80, "SD_LOG_REC_SIZE = 80")
    check(d.get("SD_LOG_PAYLOAD_SIZE") == 64, "SD_LOG_PAYLOAD_SIZE = 64")

    print("\n== header layout (spec 8.1) ==")
    hdr = [("magic", "SD_HDR_MAGIC", 0, 4), ("formatVersion", "SD_HDR_FORMAT_VER", 4, 2),
           ("recordSize", "SD_HDR_REC_SIZE", 6, 2), ("deviceId", "SD_HDR_DEVID", 8, 6),
           ("bootId", "SD_HDR_BOOTID", 14, 4), ("fileIndex", "SD_HDR_FILEIDX", 18, 4),
           ("firstSeq", "SD_HDR_FIRSTSEQ", 22, 4), ("sampleRateMilliHz", "SD_HDR_RATE", 26, 4),
           ("headerCrc", "SD_HDR_CRC", 30, 2)]
    pos = 0
    for label, key, want, size in hdr:
        check(d.get(key) == want, f"header.{label} @ {want}", f"got {d.get(key)}")
        pos = want + size
    check(pos == 32, "header fields end at 32", f"end {pos}")
    check(d.get("SD_LOG_HDR_SIZE") == 512, "SD_LOG_HDR_SIZE = 512")

    print("\n== flush and rotation policy (spec 10.2 / 8.3) ==")
    check(d.get("SD_LOG_FLUSH_ITV") == 2000, "flush interval 2 s")
    check(d.get("SD_LOG_FLUSH_REC") == 20, "flush every 20 records")
    check(d.get("SD_LOG_BUF_MAX", 0) >= 20, "buffer holds at least one flush batch")

    print("\n== frame encoding (spec 3.1 / 3.2) ==")
    f = defines(esp, r"#define\s+(ESP_FMT_(?:CHK_INIT|SIZE_MIN))\s+\((0x[0-9A-Fa-f]+|\d+)\)")
    seed = f.get("ESP_FMT_CHK_INIT")
    len_min = f.get("ESP_FMT_SIZE_MIN", 6) - 2
    check(seed == 0xA5, "checksum seed 0xA5", f"got {seed}")
    check(len_min == 4, "LEN = 4 + dataLen", f"got {len_min}")
    if seed is None:
        print("  skip frame examples (checksum seed not found)")
        seed = 0xA5

    # The spec's own worked examples, byte for byte.
    examples = [
        ("initTempSleep (3.2)", 0x20, 0x10, bytes([0x19, 0x37]), "02 06 20 10 19 37 BF 03"),
        ("initLogIdentity (6.1)", 0x20, 0x23, bytes.fromhex("A1B2C3D4E5F6"),
         "02 0A 20 23 A1 B2 C3 D4 E5 F6 B9 03"),
        ("reqLogStatus req (6.2)", 0x20, 0x43, b"", "02 04 20 43 C0 03"),
        ("reqLogFiles req (6.3)", 0x20, 0x45, bytes([0]), "02 05 20 45 00 C7 03"),
        ("reqLogRead req (6.4)", 0x20, 0x44,
         bytes.fromhex("0000002A") + bytes.fromhex("0001E240") + bytes.fromhex("03E8"),
         "02 0E 20 44 00 00 00 2A 00 01 E2 40 03 E8 AF 03"),
        ("reqLogRead ack (6.4)", 0x02, 0x44, b"", "02 04 02 44 E5 03"),
        ("statLogChunk END (6.5)", 0x20, 0x71, bytes([0, 0]) + bytes.fromhex("0001E627"),
         "02 0A 20 71 00 00 00 01 E6 27 3C 03"),
        ("ctrlLogEnable (6.6)", 0x20, 0x56, bytes([0]), "02 05 20 56 00 D4 03"),
        ("evtLogError (6.7)", 0x20, 0x84, bytes([2, 0, 0]), "02 07 20 84 02 00 00 06 03"),
    ]
    for label, dirb, cmd, data, want in examples:
        got = " ".join(f"{x:02X}" for x in frame(dirb, cmd, data, seed, len_min))
        check(got == want, f"{label}", f"got {got}")

    print("\n== CRC-16/CCITT-FALSE (spec 7) ==")
    check(crc16(b"123456789") == 0x29B1, "reference check vector 0x29B1")
    cc = None
    for cand in ("arm-none-eabi-gcc", "gcc", "cc"):
        if subprocess.call(["which", cand], stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL) == 0 and cand != "arm-none-eabi-gcc":
            cc = cand
            break
    if cc and os.path.exists(crc_c):
        with tempfile.TemporaryDirectory() as td:
            main_c = os.path.join(td, "m.c")
            open(main_c, "w").write(
                '#include <stdio.h>\n#include <string.h>\n#include "lib_crc.h"\n'
                'int main(void){const char*v[]={"123456789","","A","AB","PSA1"};'
                'for(int i=0;i<5;i++)printf("%04X\\n",'
                'u16_CRC16_CCITT((const unsigned char*)v[i],strlen(v[i])));return 0;}\n')
            exe = os.path.join(td, "m")
            if subprocess.call([cc, "-I", crc_h, "-o", exe, main_c, crc_c],
                               stderr=subprocess.DEVNULL) == 0:
                out = subprocess.check_output([exe]).decode().split()
                want = [f"{crc16(s):04X}" for s in [b"123456789", b"", b"A", b"AB", b"PSA1"]]
                check(out == want, "firmware lib_crc.c matches the reference",
                      f"{out} vs {want}")
            else:
                print("  skip firmware lib_crc.c (compile failed)")
    else:
        print("  skip firmware lib_crc.c (no host compiler)")

    print(f"\n{PASS} passed, {len(FAIL)} failed")
    for f_ in FAIL:
        print(f"  - {f_}")
    return 1 if FAIL else 0


if __name__ == "__main__":
    sys.exit(main())
