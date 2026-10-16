# mission-control-server.py v2 - Edison bridge (two-way)
#
# GET  /status -> serves C:\Games\AshenSpiral\STATUS.md (read-only)
# GET  /inbox  -> serves C:\Games\AshenSpiral\INBOX.md (read-only)
# POST /inbox  -> appends {"from": "...", "text": "..."} as a new INBOX.md entry
#
# Run:  cd C:\Games\AshenSpiral
#       python mission-control-server.py
# Leave the window open. Ctrl+C to stop.
#
# Replaces v1 (same /status behavior). Restart the server after replacing
# the file. Only ever exposes/appends these two markdown files - nothing else.

import datetime
import http.server
import json
import os
import socketserver

REPO = os.path.dirname(os.path.abspath(__file__))
STATUS_FILE = os.path.join(REPO, "STATUS.md")
INBOX_FILE = os.path.join(REPO, "INBOX.md")
PORT = 8765
MAX_BODY = 8192

INBOX_HEADER = (
    "# Inbox - instructions from Josue via Edison\n"
    "\n"
    "New entries below are instructions. Act on them, then delete the entry\n"
    "(or move it under ## Handled). Do not commit this file.\n"
)


def read_status():
    try:
        with open(STATUS_FILE, "r", encoding="utf-8") as f:
            return 200, f.read().encode("utf-8"), "text/markdown; charset=utf-8"
    except FileNotFoundError:
        body = b"STATUS.md not found - the agent has not written one yet."
        return 404, body, "text/plain; charset=utf-8"


def read_inbox():
    try:
        with open(INBOX_FILE, "r", encoding="utf-8") as f:
            return f.read().encode("utf-8")
    except FileNotFoundError:
        return b"# Inbox\n\n(empty)\n"


class Handler(http.server.BaseHTTPRequestHandler):
    def _send(self, code, body, ctype):
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path in ("/", "/status"):
            code, body, ctype = read_status()
            self._send(code, body, ctype)
        elif self.path == "/inbox":
            self._send(200, read_inbox(), "text/markdown; charset=utf-8")
        else:
            self.send_response(404)
            self.end_headers()

    def do_POST(self):
        if self.path != "/inbox":
            self.send_response(404)
            self.end_headers()
            return
        try:
            length = int(self.headers.get("Content-Length", 0))
        except ValueError:
            length = 0
        if length <= 0 or length > MAX_BODY:
            self.send_response(400)
            self.end_headers()
            return
        try:
            data = json.loads(self.rfile.read(length).decode("utf-8"))
            text = str(data.get("text", "")).strip()
            sender = str(data.get("from", "Edison"))[:40]
        except Exception:
            text = ""
        if not text:
            self.send_response(400)
            self.end_headers()
            return
        if not os.path.exists(INBOX_FILE):
            with open(INBOX_FILE, "w", encoding="utf-8") as f:
                f.write(INBOX_HEADER)
        ts = datetime.datetime.now().strftime("%Y-%m-%d %H:%M")
        entry = "\n## %s - From %s (via bridge)\n\n%s\n" % (ts, sender, text)
        with open(INBOX_FILE, "a", encoding="utf-8") as f:
            f.write(entry)
        self._send(200, b'{"ok": true}', "application/json")

    def log_message(self, *args):
        pass  # quiet


if __name__ == "__main__":
    # Bind all interfaces; Edison reaches it via the Tailscale address.
    # Append-only inbox, single file - nothing else on the PC is exposed.
    with socketserver.TCPServer(("0.0.0.0", PORT), Handler) as httpd:
        print("Mission-control server v2 running (two-way).")
        print("Serving: %s" % STATUS_FILE)
        print("Inbox:   %s" % INBOX_FILE)
        print("Port: %d" % PORT)
        print("Leave this window open. Ctrl+C to stop.")
        httpd.serve_forever()
