"""A local stand-in for the stats site's matchmaking API (br-hub /api/v2), for the diag rig.

A local br-hub instance (default 127.0.0.1:8765) serves the real routes and is the test to run for
the happy path. This stub is for the failure modes a real instance will not produce on demand.
Point a DIAG client at it with -br-mm-endpoint=http://127.0.0.1:8766/ (in ClientLaunchParams).

  POST /api/v2/matchmaking/available  -> {"available": true|false}       (--available)
  POST /api/v2/matchmaking/search     -> depends on --search:
        found    "wait" --wait-count times, then "found" with --server   (the happy path, with retries)
        wait     always "wait" with --retry-ms                           (search budget -> fallback)
        refused  HTTP 404, as for a server not enabled for matchmaking
        error    HTTP 500
        garbage  HTTP 200 with a body that is not JSON
        hang     never answers within the client's timeout               (watchdog -> fallback)
--available takes true, false, error or hang the same way.

Every request is printed, so the stub's console is the server half of the test log.
Stdlib only.
"""
import argparse
import http.server
import json
import time

ROUTE = "/api/v2/matchmaking/"


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", type=int, default=8766)
    ap.add_argument("--available", choices=["true", "false", "error", "hang"], default="true")
    ap.add_argument("--search", choices=["found", "wait", "refused", "error", "garbage", "hang"], default="found")
    ap.add_argument("--wait-count", type=int, default=2, help="'wait' replies before 'found' answers")
    ap.add_argument("--server", default="127.0.0.1:2302", help="ip:port returned on a match")
    ap.add_argument("--password", default=None, help="match password (default: null, as br-hub sends it)")
    ap.add_argument("--retry-ms", type=int, default=4000)
    ap.add_argument("--hang-s", type=float, default=20)
    args = ap.parse_args()

    searches = {"n": 0}

    class Handler(http.server.BaseHTTPRequestHandler):
        def reply(self, code, body):
            if isinstance(body, (dict, list)):
                body = json.dumps(body, separators=(",", ":"))
            data = body.encode("utf-8")
            self.send_response(code)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
            print(f"  -> {code} {body}", flush=True)

        def answer(self, mode, body):
            if mode == "error":
                self.reply(500, {"detail": "stub error"})
            elif mode == "refused":
                self.reply(404, {"detail": "matchmaking is not enabled for this server"})
            elif mode == "garbage":
                self.reply(200, "<html>not json</html>")
            elif mode == "hang":
                time.sleep(args.hang_s)
                self.reply(200, body)
            else:
                self.reply(200, body)

        def do_POST(self):
            length = int(self.headers.get("Content-Length", 0))
            payload = self.rfile.read(length).decode("utf-8", "replace")
            print(f"{time.strftime('%H:%M:%S')} POST {self.path} {payload[:300]}", flush=True)
            try:
                json.loads(payload or "{}")
            except json.JSONDecodeError:
                print("  !! request body is not JSON", flush=True)

            if self.path == ROUTE + "available":
                return self.answer(args.available, {"available": args.available == "true"})

            if self.path == ROUTE + "search":
                searches["n"] += 1
                wait = {"status": "wait", "retry_ms": args.retry_ms}
                body = wait
                if args.search == "found" and searches["n"] > args.wait_count:
                    host, _, port = args.server.rpartition(":")
                    body = {"status": "found", "ip": host, "port": int(port), "password": args.password}
                return self.answer(args.search, body)

            self.reply(404, {"detail": "Not Found"})

        def do_GET(self):
            print(f"{time.strftime('%H:%M:%S')} GET {self.path} (v2 has no GET route)", flush=True)
            self.reply(405, {"detail": "Method Not Allowed"})

        def log_message(self, *a):
            pass

    print(f"mm_stub on :{args.port}  available={args.available} search={args.search}", flush=True)
    http.server.ThreadingHTTPServer(("127.0.0.1", args.port), Handler).serve_forever()


if __name__ == "__main__":
    main()
