#!/usr/bin/env python3
"""Validate a .psa log file against the SD logging spec (v1.5, sections 7 / 8.1).

Checks what the device cannot check itself: that the header is coherent, that
every record's CRC holds, and above all that the n-th record carries
seq == firstSeq + n. Backfill seeks with 512 + (seq - firstSeq) * 80 instead of
scanning, so a single broken slot makes the device answer a seq request with the
wrong sample -- silently, because the record it returns is itself valid.

Usage:  validate_psa.py [--decode=0,1,2] FILE [FILE ...]
Exit:   0 all files valid, 1 otherwise

--decode prints the full expected field decoding of the named records. That is
what settles whether both sides read the payload the same way -- a file can
parse cleanly and still be decoded with the wrong scale or endianness.
"""
import sys
import struct

HDR_SIZE = 512
REC_SIZE = 80
MAGIC = b"PSA1"

MODE_NAMES = {
    0: "SLEEP", 1: "WAITING", 2: "FORCE_UP", 3: "FORCE_ON", 4: "FORCE_DOWN",
    5: "TEST", 6: "ERROR", 7: "HEALING", 8: "BOOTING", 9: "WAKE_UP", 10: "OFF",
}
FLAG_TX_OK = 1 << 0
FLAG_INVALID = 1 << 1


def crc16_ccitt_false(data: bytes) -> int:
    """poly 0x1021, init 0xFFFF, no reflection, xorout 0x0000. "123456789" -> 0x29B1."""
    crc = 0xFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def parse_header(raw: bytes) -> dict:
    magic, fmt, rec_size = struct.unpack_from(">4sHH", raw, 0)
    dev = raw[8:14]
    boot_id, file_idx, first_seq, rate = struct.unpack_from(">IIII", raw, 14)
    stored_crc = struct.unpack_from(">H", raw, 30)[0]
    return {
        "magic": magic, "formatVersion": fmt, "recordSize": rec_size,
        "deviceId": dev, "bootId": boot_id, "fileIndex": file_idx,
        "firstSeq": first_seq, "sampleRateMilliHz": rate,
        "headerCrc": stored_crc, "headerCrcCalc": crc16_ccitt_false(raw[:30]),
    }


# STAT(0x70) payload layout, from comm_esp.c v_ESP_Send_Sensing. Offsets are
# relative to the payload, i.e. record offset 8 + these.
def decode_payload(p: bytes) -> list:
    def i16(o):
        return int.from_bytes(p[o:o + 2], "big", signed=True)

    def u16(o):
        return int.from_bytes(p[o:o + 2], "big")

    def dec(o):
        # [integer, fraction x100] -- the code stores value*100 minus int*100
        return p[o] + p[o + 1] / 100.0

    def f32(o):
        return struct.unpack_from(">f", p, o)[0]

    out = [
        ("imu_l_gyro", f"{i16(0)}, {i16(2)}, {i16(4)}"),
        ("imu_l_accel", f"{i16(6)}, {i16(8)}, {i16(10)}"),
        ("imu_r_gyro", f"{i16(12)}, {i16(14)}, {i16(16)}"),
        ("imu_r_accel", f"{i16(18)}, {i16(20)}, {i16(22)}"),
        ("fsr_left", u16(24)), ("fsr_right", u16(26)),
        ("temp_out", f"{dec(28):.2f}"), ("temp_in", f"{dec(30):.2f}"),
        ("temp_ir", f"{dec(32):.2f}"),
        ("tof1", u16(34)), ("tof2", u16(36)),
        ("battery", f"{dec(38):.2f}"),
        ("imu_l_evt", p[40]), ("imu_r_evt", p[41]),
        ("gps_lat", f"{f32(42):.6f}"), ("gps_lon", f"{f32(46):.6f}"),
        ("gps_sat", p[50]), ("gps_fix", p[51]),
        ("angle_l", f"{i16(52)/100.0:.2f}, {i16(54)/100.0:.2f}, {i16(56)/100.0:.2f}"),
        ("angle_r", f"{i16(58)/100.0:.2f}, {i16(60)/100.0:.2f}, {i16(62)/100.0:.2f}"),
    ]
    return out


def decode(path: str, want: list) -> None:
    """Print the full expected decoding of chosen records, for cross-checking."""
    with open(path, "rb") as fh:
        raw = fh.read()
    h = parse_header(raw)
    n_total = (len(raw) - HDR_SIZE) // REC_SIZE
    print(f"\n=== {path} — expected decoding ===")
    print(f"  header: bootId={h['bootId']} fileIndex={h['fileIndex']} "
          f"firstSeq={h['firstSeq']} records={n_total}")
    for n in want:
        if n >= n_total:
            print(f"  record {n}: out of range")
            continue
        r = raw[HDR_SIZE + n * REC_SIZE:HDR_SIZE + (n + 1) * REC_SIZE]
        seq, tick = struct.unpack_from(">II", r, 0)
        mode, flags = r[72], r[73]
        err = struct.unpack_from(">H", r, 74)[0]
        crc = struct.unpack_from(">H", r, 78)[0]
        print(f"\n  --- record {n} (file offset {HDR_SIZE + n * REC_SIZE}) ---")
        print(f"    seq          = {seq}      (expected firstSeq+n = {h['firstSeq'] + n})")
        print(f"    tickMs       = {tick} ms   [big-endian]")
        print(f"    deviceMode   = {mode} ({MODE_NAMES.get(mode, '?')})")
        print(f"    flags        = 0x{flags:02X} "
              f"(txOk={'y' if flags & FLAG_TX_OK else 'n'}, "
              f"placeholder={'YES' if flags & FLAG_INVALID else 'no'})")
        print(f"    errorMask    = 0x{err:04X}")
        print(f"    crc16        = 0x{crc:04X}  (recomputed 0x{crc16_ccitt_false(r[:78]):04X})")
        if flags & FLAG_INVALID:
            print("    statPayload  = placeholder, all zero -- do not consume")
            continue
        for name, val in decode_payload(r[8:72]):
            print(f"    {name:<12} = {val}")


def validate(path: str) -> bool:
    with open(path, "rb") as fh:
        raw = fh.read()

    errors, warnings = [], []
    print(f"\n=== {path} ({len(raw)} B) ===")

    if len(raw) < HDR_SIZE:
        print(f"  [ERROR] shorter than a header ({len(raw)} < {HDR_SIZE})")
        return False

    h = parse_header(raw)
    if h["magic"] != MAGIC:
        errors.append(f"magic {h['magic']!r} != {MAGIC!r}")
    if h["recordSize"] != REC_SIZE:
        errors.append(f"recordSize {h['recordSize']} != {REC_SIZE}")
    if h["headerCrc"] != h["headerCrcCalc"]:
        errors.append(f"header CRC 0x{h['headerCrc']:04X} != 0x{h['headerCrcCalc']:04X}")

    dev = "UNKNOWN" if h["deviceId"] == b"\xff" * 6 else h["deviceId"].hex().upper()
    print(f"  header : fmt={h['formatVersion']} recSize={h['recordSize']} dev={dev}")
    print(f"           bootId={h['bootId']} fileIndex={h['fileIndex']} "
          f"firstSeq={h['firstSeq']} rate={h['sampleRateMilliHz']} mHz")

    body = raw[HDR_SIZE:]
    n_full, leftover = divmod(len(body), REC_SIZE)
    if leftover:
        # Expected after a power cut: the merge tool truncates the partial tail.
        warnings.append(f"{leftover} trailing bytes, partial record truncated")

    if n_full == 0:
        # Spec 6.3 (correction 26): such a file is excluded from the device index.
        warnings.append("no records -- should not appear in reqLogFiles")

    bad_crc, bad_seq, placeholders, tx_fail = 0, 0, 0, 0
    modes, ticks, first_tick, last_tick = {}, [], None, None

    for n in range(n_full):
        r = body[n * REC_SIZE:(n + 1) * REC_SIZE]
        seq, tick = struct.unpack_from(">II", r, 0)
        mode, flags = r[72], r[73]
        stored = struct.unpack_from(">H", r, 78)[0]

        if stored != crc16_ccitt_false(r[:78]):
            bad_crc += 1
            continue                      # a bad CRC makes the rest of the record meaningless
        if seq != h["firstSeq"] + n:
            bad_seq += 1
            if bad_seq <= 3:
                errors.append(f"record {n}: seq {seq}, expected {h['firstSeq'] + n}")
        if flags & FLAG_INVALID:
            placeholders += 1
        if not (flags & FLAG_TX_OK):
            tx_fail += 1
        modes[mode] = modes.get(mode, 0) + 1
        if first_tick is None:
            first_tick = tick
        last_tick = tick
        ticks.append(tick)

    if bad_crc:
        errors.append(f"{bad_crc} record(s) failed CRC")
    if bad_seq:
        errors.append(f"{bad_seq} record(s) break seq == firstSeq + n")

    print(f"  records: {n_full}  placeholders={placeholders}  txFail={tx_fail}")
    if modes:
        print("  modes  : " + ", ".join(
            f"{MODE_NAMES.get(m, f'?{m}')}={c}" for m, c in sorted(modes.items())))
    if first_tick is not None and n_full > 1:
        span = last_tick - first_tick
        gaps = [b - a for a, b in zip(ticks, ticks[1:])]
        if gaps:
            print(f"  tick   : {first_tick}..{last_tick} ms (span {span/1000:.1f} s), "
                  f"step min={min(gaps)} max={max(gaps)} ms")
            if max(gaps) > 200:
                warnings.append(f"largest tick step {max(gaps)} ms -- sampling paused?")

    for w in warnings:
        print(f"  [warn ] {w}")
    for e in errors:
        print(f"  [ERROR] {e}")
    print(f"  --> {'VALID' if not errors else 'INVALID'}")
    return not errors


def main() -> int:
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        return 2
    assert crc16_ccitt_false(b"123456789") == 0x29B1, "CRC self test failed"

    # --decode N,N,N prints the full expected field decoding of those records,
    # which is what pins the two sides' interpretation of the payload.
    want = None
    if args[0].startswith("--decode"):
        want = [int(x) for x in args[0].split("=", 1)[1].split(",")] if "=" in args[0] else [0]
        args = args[1:]

    ok = all([validate(p) for p in args])
    if want is not None:
        for p in args:
            decode(p, want)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
