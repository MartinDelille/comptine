#!/usr/bin/env python3
"""
Generate platform-specific application icons from SVG source files.

Usage:
  generate_icon.py --os macos  --svg <full.svg> --svg-small <small.svg> --out <output.icns>
  generate_icon.py --os windows --svg <full.svg> --svg-small <small.svg> --out <output.ico>
"""

import argparse
import io
from pathlib import Path

from PIL import Image
from resvg_py import svg_to_bytes


def svg_to_image(svg_path: str, width: int, height: int) -> Image.Image:
    """Render an SVG file to a Pillow Image at the given size."""
    data = svg_to_bytes(svg_path=svg_path, width=width, height=height)
    return Image.open(io.BytesIO(data)).convert("RGBA")


def generate_macos(svg: str, svg_small: str, out: str) -> None:
    """Generate a macOS .icns file.

    Icon sizes required by Apple (iconset spec):
      16x16, 16x16@2x(=32), 32x32, 32x32@2x(=64), 128x128, 128x128@2x(=256),
      256x256, 256x256@2x(=512), 512x512, 512x512@2x(=1024)
    Icons < 128 px are rendered from the small SVG for better detail.
    """
    # (size, use_small)
    sizes = [
        (16, True),
        (32, True),
        (64, True),
        (128, False),
        (256, False),
        (512, False),
        (1024, False),
    ]

    images = []
    for px, use_small in sizes:
        src = svg_small if use_small else svg
        images.append(svg_to_image(src, px, px))

    # Pillow's ICNS writer picks up all sizes from append_images
    base = images[0]
    base.save(
        out,
        format="ICNS",
        append_images=images[1:],
        sizes=[img.size for img in images],
    )
    print(f"Generated macOS icon: {out}")


def generate_windows(svg: str, svg_small: str, out: str) -> None:
    """Generate a Windows .ico file.

    Standard ICO sizes: 16, 32, 48 (from small SVG), 128, 256 (from full SVG).
    """
    small_sizes = [16, 32, 48, 64]
    full_sizes = [128, 256]

    images = []
    for px in small_sizes:
        images.append(svg_to_image(svg_small, px, px))
    for px in full_sizes:
        images.append(svg_to_image(svg, px, px))

    base = images[0]
    base.save(
        out,
        format="ICO",
        append_images=images[1:],
        sizes=[img.size for img in images],
    )
    print(f"Generated Windows icon: {out}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate platform application icon")
    parser.add_argument(
        "--os",
        choices=["macos", "windows"],
        required=True,
        help="Target platform",
    )
    parser.add_argument("--svg", required=True, help="Full-size SVG source")
    parser.add_argument("--svg-small", required=True, help="Small-detail SVG source")
    parser.add_argument("--out", required=True, help="Output icon file")
    args = parser.parse_args()

    Path(args.out).parent.mkdir(parents=True, exist_ok=True)

    if args.os == "macos":
        generate_macos(args.svg, args.svg_small, args.out)
    elif args.os == "windows":
        generate_windows(args.svg, args.svg_small, args.out)


if __name__ == "__main__":
    main()
