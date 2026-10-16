# mission-control-server.py
# Ashen Spiral - Edison bridge (v1, read-only)
#
# Serves C:\Games\AshenSpiral\STATUS.md over HTTP so Edison can poll it
# through the Tailscale private network. Read-only: exposes exactly one
# file, nothing else. Run it and leave the window open:
#
#   cd C:\Games\AshenSpiral
#   python mission-control-server.py
#
# If STATUS.md does not exist yet, the endpoint returns 404 with a short
# message ("not written yet") instead of failing.
#
# The agent (Claude Code) is responsible for keeping STATUS.md current -
# see agent-status-contract.md for the format it must follow.

import http.server
import os
import socketserver

REPO = os.path.dirname(os.path.abspath(__file__))
STATUS_FILE = os.path.join(REPO, "STATUS.md")
PORT = 8765


class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path not in ("/", "/status"):
            self.send_response(404)
            self.end_headers()
            return
        try:
            with open(STATUS_FILE, "r", encoding="utf-8") as f:
                body = f.read().encode("utf-8")
        except FileNotFoundError:
            body = b"STATUS.md not found - the agent has not written one yet."
            self.send_response(404)
            self.send_header("Content-Type", "text/plain; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return
        self.send_response(200)
        self.send_header("Content-Type", "text/markdown; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *args):
        pass  # quiet


if __name__ == "__main__":
    # Bind all interfaces; Edison reaches it via the Tailscale address.
    with socketserver.TCPServer(("0.0.0.0", PORT), Handler) as httpd:
        print("Mission-control server running.")
        print("Serving: %s" % STATUS_FILE)
        print("Port: %d (reach Edison via your Tailscale IP, e.g. http://100.x.y.z:%d/status)" % (PORT, PORT))
        print("Leave this window open. Ctrl+C to stop.")
        httpd.serve_forever()
