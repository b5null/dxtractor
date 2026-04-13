import re
import base64
import sys
import os

# Default log file
DEFAULT_LOG = "/var/log/dnsmasq.log"

# CLI override
log_file = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_LOG

if not os.path.exists(log_file):
    print(f"[-] Log file not found: {log_file}")
    sys.exit(1)

print(f"[+] Using log file: {log_file}")

# Regex for: seq.session.data.domain
pattern = re.compile(r"query\[A\] (\d+)\.(\d+)\.([A-Z2-7]+)\.")

sessions = {}

with open(log_file, "r") as f:
    for line in f:
        match = pattern.search(line)
        if not match:
            continue

        seq = int(match.group(1))
        session = match.group(2)
        data = match.group(3)

        if session not in sessions:
            sessions[session] = {}

        sessions[session][seq] = data

# Process sessions
for session, chunks in sessions.items():

    print(f"\n[+] Session detected: {session}")
    print(f"[+] Total chunks: {len(chunks)}")

    if 999 not in chunks:
        print("[-] Missing END marker, skipping")
        continue

    # Remove END marker
    chunks.pop(999)

    # Reassemble in order
    try:
        ordered = "".join(chunks[i] for i in sorted(chunks.keys()))
    except KeyError:
        print("[-] Missing sequence chunk, skipping")
        continue

    print("[+] Reassembling Base32 stream...")

    try:
        decoded = base64.b32decode(ordered)
    except Exception as e:
        print(f"[-] Decode error: {e}")
        continue

    output_file = f"recovered_{session}.bin"

    with open(output_file, "wb") as out:
        out.write(decoded)

    print(f"[+] File written: {output_file}")
