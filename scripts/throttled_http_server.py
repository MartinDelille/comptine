#!/usr/bin/env python3
"""Serve files over HTTP with a configurable bandwidth limit."""

import argparse
import os
import time
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer


class ThrottledRequestHandler(SimpleHTTPRequestHandler):
    rate_bytes_per_second = 64 * 1024

    def copyfile(self, source, outputfile):
        total = os.fstat(source.fileno()).st_size
        transferred = 0
        last_logged_percent = -1
        while True:
            chunk = source.read(16 * 1024)
            if not chunk:
                break
            outputfile.write(chunk)
            outputfile.flush()
            transferred += len(chunk)
            percent = int(transferred * 100 / total) if total else 100
            if percent != last_logged_percent and (percent % 5 == 0 or percent == 100):
                print(
                    f"{self.path}: {percent}% ({transferred}/{total} bytes)",
                    flush=True,
                )
                last_logged_percent = percent
            time.sleep(len(chunk) / self.rate_bytes_per_second)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--rate",
        type=float,
        default=64,
        help="Maximum transfer rate in KiB/s (default: 64)",
    )
    parser.add_argument("--bind", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8000)
    args = parser.parse_args()

    if args.rate <= 0:
        parser.error("--rate must be greater than zero")

    ThrottledRequestHandler.rate_bytes_per_second = args.rate * 1024
    server = ThreadingHTTPServer((args.bind, args.port), ThrottledRequestHandler)
    print(f"Serving on http://{args.bind}:{args.port} at {args.rate:g} KiB/s")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
