# Security Policy

## Supported Versions

| Version | Supported |
|---|---|
| `main` branch | ✅ |
| Older tags | ❌ |

## Reporting a Vulnerability

This project runs on a bare-metal embedded microcontroller with no network connectivity. However, if you discover a security issue (e.g. a buffer overflow, stack corruption, or unsafe memory access that could be exploited in a connected variant), please **do not open a public issue**.

Instead:
1. Open a **private** security advisory via GitHub's *Security* tab.
2. Include: a description of the vulnerability, affected files, potential impact, and a suggested fix if available.

We aim to acknowledge reports within **5 business days** and to issue a fix within **30 days** for confirmed vulnerabilities.

## Known Attack Surface

- **UART RX**: Unvalidated input from the serial port is stored in a fixed-size buffer with no bounds check. Do not connect the RX line to an untrusted host in a safety-critical deployment.
- **I²C Bus**: No authentication is performed on I²C peripherals. Physical access to the bus allows injection of arbitrary data into the RTC registers.