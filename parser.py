import re
import base64

log_file = "/var/log/dnsmasq.log"

pattern = re.compile(
    r'query\[A\]\s+(\d+)\.(\d+)\.([A-Za-z2-7]+)\.',
    re.IGNORECASE
)

sessions = {}

with open(log_file, "r") as f:
    for line in f:
        m = pattern.search(line)
        if not m:
            continue

        seq = int(m.group(1))
        sess = m.group(2)
        data = m.group(3).upper()

        if data == "END":
            continue

        sessions.setdefault(sess, {})[seq] = data


for sess, chunks in sessions.items():
    print(f"\n[+] Session {sess}")

    ordered = [chunks[k] for k in sorted(chunks.keys())]

    combined = "".join(ordered)

    # 🔧 restore padding
    pad = (8 - (len(combined) % 8)) % 8
    combined += "=" * pad

    try:
        decoded = base64.b32decode(combined, casefold=True)

        print("\n--- DECODED ---\n")
        print(decoded.decode(errors="ignore"))

    except Exception as e:
        print(f"[!] Decode error: {e}")
                                        
