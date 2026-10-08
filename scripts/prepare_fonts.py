"""Decode original PGF system fonts to a local web-profile font bundle.

Usage: prepare_fonts.py <flash0/font directory> <preload/fonts directory>
PGF layout and sceFont ABI: PPSSPP Core/Font/PGF.h and PSP SDK.
The bundle contains the input font's metrics and glyph pixels unchanged.
"""
from pathlib import Path
import struct
import sys


def bits(data, position, count):
    start = position // 8
    end = (position + count + 7) // 8
    if end > len(data):
        raise ValueError("Truncated PGF bitstream")
    return (int.from_bytes(data[start:end], "little") >> (position % 8)) & ((1 << count) - 1)


def signed(value, count):
    return value - (1 << count) if value & (1 << (count - 1)) else value


def convert(path, output):
    data = path.read_bytes()
    if len(data) < 392 or data[4:8] != b"PGF0":
        raise ValueError(f"Invalid PGF: {path}")
    revision = struct.unpack_from("<i", data, 8)[0]
    if revision != 2:
        raise ValueError(f"PGF revision {revision} is not supported: {path.name}")
    map_count, pointer_count, map_bits, pointer_bits = struct.unpack_from("<4i", data, 16)
    first, last = struct.unpack_from("<2H", data, 182)
    cursor = 392
    tables = []
    for length in data[258:262]:
        tables.append([struct.unpack_from("<2i", data, cursor + i * 8) for i in range(length)])
        cursor += length * 8
    shadow_count, shadow_bits = struct.unpack_from("<2i", data, 364)
    cursor += ((shadow_count * shadow_bits + 31) // 32) * 4
    map_size = ((map_count * map_bits + 31) // 32) * 4
    mapping = data[cursor:cursor + map_size]
    cursor += map_size
    pointer_size = ((pointer_count * pointer_bits + 31) // 32) * 4
    pointers = data[cursor:cursor + pointer_size]
    font_data = data[cursor + pointer_size:]

    name = data[53:117].split(b"\0")[0]
    latin = path.stem.startswith("ltn")
    number = int(path.stem[3:]) if latin else 0
    style = (1, 2, 5, 6)[(number % 8) // 2] if latin else 103
    family = 1 + number % 2 if latin else 1
    size_h, size_v, res_h, res_v = struct.unpack_from("<4i", data, 36)
    font_style = struct.pack("<5f6H", size_h / 64, size_v / 64, res_h / 64, res_v / 64,
                             0, family, style, 0, 2 if latin else 1, 0, 1)
    font_style += name.ljust(64, b"\0") + path.name.encode().ljust(64, b"\0") + bytes(8)
    asc, desc, left, base, center, top, adv_h, adv_v, max_w, max_h = struct.unpack_from("<10i", data, 212)
    metrics = (max_w, max_h, asc, desc, left, base, center, top, adv_h, adv_v)
    bitmap_w, bitmap_h = struct.unpack_from("<2H", data, 252)
    font_info = struct.pack("<10i10f2H2I", *metrics, *(x / 64 for x in metrics),
                            bitmap_w, bitmap_h, pointer_count, shadow_count)
    font_info += font_style + bytes((data[34], 0, 0, 0))
    assert len(font_info) == 264

    decoded = {}
    records = []
    for code in range(first, first + map_count):
        index = bits(mapping, (code - first) * map_bits, map_bits)
        if index >= pointer_count:
            continue
        if index not in decoded:
            position = bits(pointers, index * pointer_bits, pointer_bits) * 32 + 14

            def take(count):
                nonlocal position
                value = bits(font_data, position, count)
                position += count
                return value

            width, height = take(7), take(7)
            glyph_left, glyph_top = signed(take(7), 7), signed(take(7), 7)
            flags = take(6)
            shadow_flags = (take(2) << 5) | (take(2) << 3) | take(3)
            shadow_id = take(9)
            values = []
            for table, mask in zip(tables, (4, 8, 16, 32)):
                if flags & mask:
                    metric_index = take(8)
                    values.extend(table[metric_index] if metric_index < len(table) else (0, 0))
                else:
                    values.extend((signed(take(32), 32), signed(take(32), 32)))
            dim_w, dim_h, x_h, x_v, y_h, y_v, advance_h, advance_v = values
            char_info = struct.pack("<14i2h", width, height, glyph_left, glyph_top, dim_w, dim_h,
                                    y_h, y_h - dim_h, x_h, y_h, x_v, y_v,
                                    advance_h, advance_v, shadow_flags, shadow_id)
            raw = []
            position = (position // 8) * 8
            while len(raw) < width * height:
                control = take(4)
                count = control + 1 if control < 8 else 16 - control
                run = [take(4)] * count if control < 8 else [take(4) for _ in range(count)]
                raw.extend(run[:width * height - len(raw)])
            if width * height and flags & 3 not in (1, 2):
                raise ValueError(f"Unsupported PGF bitmap layout in {path.name}, glyph {index}")
            if flags & 3 == 2:
                raw = [raw[x * height + y] for y in range(height) for x in range(width)]
            decoded[index] = char_info, bytes(raw)
        info, pixels = decoded[index]
        records.append(struct.pack("<I", code) + info + struct.pack("<I", len(pixels)) + pixels)
    output.write_bytes(b"PWF1" + struct.pack("<I", len(records)) + font_info + b"".join(records))
    print(f"{path.name}: {len(records)} characters, {output.stat().st_size} bytes")


def main():
    source, target = map(Path, sys.argv[1:3])
    target.mkdir(parents=True, exist_ok=True)
    for name in ["jpn0"] + [f"ltn{i}" for i in range(16)]:
        path = source / f"{name}.pgf"
        if path.exists():
            convert(path, target / f"{name}.pwf")


if __name__ == "__main__":
    main()
