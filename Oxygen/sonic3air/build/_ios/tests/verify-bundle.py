"""Check the actual IPA payload before publishing it; no ROM is needed."""
import plistlib
import re
import struct
import sys
from pathlib import Path

app = Path(sys.argv[1])
with (app / "Info.plist").open("rb") as stream:
    info = plistlib.load(stream)
cmake = (Path(__file__).resolve().parents[1] / "CMakeLists.txt").read_text()
for plist_key, cmake_key in [
    ("CFBundleShortVersionString", "MACOSX_BUNDLE_SHORT_VERSION_STRING"),
    ("CFBundleVersion", "MACOSX_BUNDLE_BUNDLE_VERSION"),
]:
    expected = re.search(rf'{cmake_key}\s+"([^"\n]+)"', cmake)
    assert expected, f"Missing {cmake_key} in CMakeLists.txt"
    assert info[plist_key] == expected.group(1), f"{plist_key}: {info[plist_key]} != {expected.group(1)}"
assert info["UIDeviceFamily"] == [1, 2]
assert info["CFBundleSupportedPlatforms"] == ["iPhoneOS"]
assert info["CFBundleIcons"]["CFBundlePrimaryIcon"]["CFBundleIconFiles"]
assert info["CFBundleIcons~ipad"]["CFBundlePrimaryIcon"]["CFBundleIconFiles"]
for name, size in {
    "AppIcon60x60@2x.png": 120,
    "AppIcon60x60@3x.png": 180,
    "AppIcon76x76.png": 76,
    "AppIcon76x76@2x.png": 152,
    "AppIcon83.5x83.5@2x.png": 167,
}.items():
    data = (app / name).read_bytes()
    assert data[:8] == b"\x89PNG\r\n\x1a\n", name
    assert struct.unpack(">II", data[16:24]) == (size, size), name
for name in [
    "content/sonic3air/scripts/main.lemon",
    "content/sonic3air/data/metadata.json",
    "content/oxygenengine/data/oxygen_icon.png",
]:
    assert (app / name).is_file(), name
binary = (app / info["CFBundleExecutable"]).read_bytes()
magic, cpu, subtype, kind = struct.unpack_from("<4I", binary)
assert magic == 0xFEEDFACF and cpu == 0x0100000C and kind == 2
print("Verified native arm64 app, source entry point, bundle metadata and iPhone/iPad icons")
