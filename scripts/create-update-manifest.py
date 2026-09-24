#!/usr/bin/env python3

import argparse
import base64
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path


def sign(payload: bytes, private_key: Path) -> str:
    with tempfile.NamedTemporaryFile() as payload_file:
        payload_file.write(payload)
        payload_file.flush()
        result = subprocess.run(
            [
                "openssl",
                "pkeyutl",
                "-sign",
                "-rawin",
                "-inkey",
                str(private_key),
                "-in",
                payload_file.name,
            ],
            check=True,
            capture_output=True,
        )
    return base64.b64encode(result.stdout).decode("ascii")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--version", required=True)
    parser.add_argument("--notes", required=True)
    parser.add_argument("--private-key", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--asset", action="append", nargs=5, metavar=("PLATFORM", "ARCH", "URL", "FILE", "SHA256"), required=True)
    args = parser.parse_args()

    assets = []
    for platform, architecture, url, filename, expected_hash in args.asset:
        digest = hashlib.sha256(Path(filename).read_bytes()).hexdigest()
        if expected_hash and digest != expected_hash:
            raise SystemExit(f"hash mismatch for {filename}")
        payload = "\n".join((args.version, platform, architecture, url, digest)).encode()
        assets.append({
            "platform": platform,
            "architecture": architecture,
            "url": url,
            "sha256": digest,
            "signature": sign(payload, args.private_key),
        })

    manifest = {
        "schemaVersion": 1,
        "version": args.version,
        "releaseNotes": Path(args.notes).read_text(encoding="utf-8"),
        "assets": assets,
    }
    args.output.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
