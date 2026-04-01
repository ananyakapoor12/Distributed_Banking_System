
# Distributed Banking System

A distributed banking application built for CE4013/CZ4013/SC4051 (NTU Distributed Systems). The system uses UDP sockets and demonstrates **at-least-once** and **at-most-once** invocation semantics under simulated packet loss.

The server is written in C++ and the client in Java; they communicate over a shared binary wire protocol.

## Project Structure

```
Distributed_Banking_System/
├── server/                     # C++ UDP server + SQLite persistence
│   ├── server.cpp              # Main event loop and request dispatch
│   ├── handlers.cpp/h          # Per-opcode argument parsers
│   ├── account_store.cpp/h     # SQLite-backed account CRUD
│   ├── marshaller.cpp/h        # Binary serialisation helpers
│   ├── udp_socket.cpp/h        # UDP send/receive abstraction
│   ├── protocol.h              # Wire constants, enums, packet layout
│   ├── test_all.py             # Automated Python test suite
│   ├── test_monitor.py         # Monitor/callback test script
│   └── README.md
└── client_implementation/      # Java UDP client
    ├── src/client/
    │   ├── Main.java
    │   ├── core/               # BankingClient, UDPClient
    │   ├── protocol/           # Protocol constants, marshalling, message builders/parsers
    │   ├── ui/                 # Interactive console UI
    │   └── utils/              # Automated test scenarios
    ├── build.sh / build.bat
    └── README.md
```

## Supported Operations

| Opcode | Operation     | Idempotent | Description                              |
|--------|---------------|------------|------------------------------------------|
| 1      | OPEN_ACCOUNT  | No         | Create a new account, returns account number |
| 2      | CLOSE_ACCOUNT | No         | Close an existing account                |
| 3      | DEPOSIT       | No         | Deposit funds, returns new balance       |
| 4      | WITHDRAW      | No         | Withdraw funds, returns new balance      |
| 5      | MONITOR       | —          | Register for real-time account callbacks |
| 6      | TRANSFER      | No         | Transfer funds between accounts          |
| 7      | CHECK_BALANCE | Yes        | Query current balance (read-only)        |

## Invocation Semantics

Both components must be started with the same semantics flag.

**At-Most-Once (`amo`):**
- Server caches responses keyed by `ip:port` + request ID
- Duplicate requests return the cached reply without re-executing
- Safe for non-idempotent operations (deposits, transfers, etc.) under packet loss

**At-Least-Once (`alo`):**
- Server re-executes every received packet — no deduplication
- Non-idempotent operations may execute multiple times on retries
- Idempotent operations (CHECK_BALANCE) are safe regardless

## Wire Protocol

All multi-byte fields use network byte order (big-endian). Strings use a 2-byte length prefix followed by UTF-8 bytes. Passwords are fixed 8 bytes, zero-padded.

**Request:**
```
[msg_type:1][request_id:16][opcode:1][arguments:varies]
```

**Response:**
```
[msg_type:1][request_id:16][opcode:1][status:1][body:varies]
```

**Callback (server → monitoring client):**
```
[msg_type:1][trigger_opcode:1][account_num:4][name:var][balance:4][currency:1][description:var]
```

## Monitoring

Clients can register for server-initiated callbacks:

1. Client sends a `MONITOR` request with a duration (seconds)
2. Server registers the client's `ip:port` for that duration
3. Server pushes a `CALLBACK` packet to all registered clients after every account operation
4. Registration expires automatically after the requested duration

**Demo setup:**
- Terminal 1: Run the server
- Terminal 2: Run a monitoring client (`MONITOR` request)
- Terminal 3: Run a regular client making transactions
- Terminal 2: Observe real-time callbacks

## Quick Start

See the component READMEs for build and run instructions:
- **Server:** [server/README.md](server/README.md)
- **Client:** [client_implementation/README.md](client_implementation/README.md)

## Important Notes

1. **Password length**: Maximum 8 bytes (enforced by the wire protocol)
2. **Concurrency**: The server handles one request at a time (single-threaded, per specification)
3. **Network**: Ensure UDP traffic is permitted through any firewall
4. **Semantics must match**: Run the server and client with the same `amo`/`alo` flag

---
**NTU College of Computing and Data Science**
**SC4051 Distributed Systems — Session 2025/2026 Semester 2**
