#!/usr/bin/env python3
"""Testes de tools/vmu_build.py com dados sinteticos (sem os arquivos do Bucanero).
uso: python3 ports/zeebo/tests/vmu_build_test.py"""
import os
import struct
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools"))
import vmu_build as vb  # noqa: E402


def fake_main_save(slots_ok=3):
    body = b""
    for i in range(vb.SAVE_COUNT):
        s = bytearray(vb.SAVE_SIZE)
        s[4:8] = struct.pack("<i", 1000 + i)
        crc = vb.crc16(bytes(s[4:]))
        s[0:4] = struct.pack("<I", crc if i < slots_ok else crc ^ 1)
        body += bytes(s)
    return b"\0" * vb.SAVE_HEADER + body.ljust(3968, b"\0")


def fake_event(blocks=40):
    """arquivo de evento valido: soma e decifra como download_read."""
    length = blocks * 512 - 640
    dec = bytearray(length)
    for i in range(64, length):
        dec[i] = (i * 7) & 0xFF
    total = sum(b - 256 if b > 127 else b for b in dec[64:length])
    dec[0x2C:0x30] = struct.pack("<I", (~total) & vb.M32)
    enc = b"".join(struct.pack("<I", (struct.unpack_from("<I", dec, i * 4)[0] ^ ((i * vb.STEP) & vb.M32) ^ vb.KEY) & vb.M32)
                   for i in range(length // 4))
    return b"\0" * 640 + enc


class Build(unittest.TestCase):
    def test_file_names(self):
        self.assertEqual(vb.card_file_name("a1"), "sa1vmu.bin")
        self.assertEqual(vb.card_file_name("B2"), "sa1vmu_b2.bin")
        with self.assertRaises(vb.BuildError):
            vb.card_file_name("e1")

    def test_roundtrip(self):
        sys_file, ev = fake_main_save(), fake_event()
        blob = vb.build_card([("SONICADV_SYS", sys_file), ("SONICADV_509", ev)])
        files = vb.parse_card(blob)
        self.assertEqual([f[0] for f in files], ["SONICADV_SYS", "SONICADV_509"])
        self.assertEqual(files[0][1:3], (0, 10))
        self.assertEqual(files[1][1:3], (10, 40))
        self.assertEqual(files[0][3][:len(sys_file)], sys_file)
        self.assertEqual(vb.main_save_slots_ok(files[0][3]), 3)
        self.assertTrue(vb.event_sum_ok(files[1][3]))
        self.assertEqual(len(blob), 4 + 24 * 28 + 50 * 512)

    def test_limits(self):
        with self.assertRaises(vb.BuildError):
            vb.build_card([("SONICADV_SYS", b"x" * 512 * 100), ("SONICADV_509", b"x" * 512 * 101)])
        with self.assertRaises(vb.BuildError):
            vb.build_card([(f"F{i}", b"x") for i in range(25)])
        with self.assertRaises(vb.BuildError):
            vb.build_card([("A", b"x"), ("A", b"y")])
        vb.build_card([(f"F{i}", b"x") for i in range(24)])

    def test_validate(self):
        vb.validate("SONICADV_SYS", fake_main_save(1), False)
        with self.assertRaises(vb.BuildError):
            vb.validate("SONICADV_SYS", fake_main_save(0), False)
        bad = bytearray(fake_event())
        bad[640 + 200] ^= 0xFF  # corpo (>= 64 depois do cabecalho), entra na soma
        with self.assertRaises(vb.BuildError):
            vb.validate("SONICADV_509", bytes(bad), False)
        vb.validate("SONICADV_V11", b"qualquer", False)  # nao e download (V nao e hex)

    def test_parse_rejects_out_of_range(self):
        blob = bytearray(vb.build_card([("SONICADV_SYS", fake_main_save())]))
        struct.pack_into("<H", blob, 4 + 16, 195)  # first=195, 10 blocos > 200
        with self.assertRaises(vb.BuildError):
            vb.parse_card(bytes(blob))


if __name__ == "__main__":
    unittest.main()
