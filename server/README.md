### Building the Server

**Prerequisites:**
- C++17 compiler (`g++`)
- [SQLiteCpp](https://github.com/SRombauts/SQLiteCpp) library (install via Homebrew: `brew install sqlitecpp`)

**Compile:**
```bash
g++ -std=c++17 -DDEBUG -I$(brew --prefix sqlitecpp)/include \
    -o server server.cpp marshaller.cpp account_store.cpp handlers.cpp \
    -L$(brew --prefix sqlitecpp)/lib -lSQLiteCpp -lsqlite3 -lpthread -ldl
```

Remove `-DDEBUG` to suppress debug output:
```bash
g++ -std=c++17 -I$(brew --prefix sqlitecpp)/include \
    -o server server.cpp marshaller.cpp account_store.cpp handlers.cpp \
    -L$(brew --prefix sqlitecpp)/lib -lSQLiteCpp -lsqlite3 -lpthread -ldl
```

### Running the Server

```bash
./server <port> <amo|alo>
```

**Examples:**
```bash
# Run on port 8014 with at-most-once semantics
./server 8014 amo

# Run on port 2222 with at-least-once semantics
./server 2222 alo
```

### Parameters
- `port`: UDP port number to listen on (default: `8014`)
- `amo|alo`: Invocation semantics — `amo` for at-most-once, `alo` for at-least-once

### Environment Variables
- `DB_PATH`: Path to the SQLite database file (default: `banking.db`)

## Supported Operations

The server handles the following UDP request opcodes:

| Opcode | Operation       | Idempotent | Description                                    |
|--------|-----------------|------------|------------------------------------------------|
| 1      | OPEN_ACCOUNT    | No         | Creates a new bank account, returns account number |
| 2      | CLOSE_ACCOUNT   | No         | Closes an existing account                    |
| 3      | DEPOSIT         | No         | Deposits funds, returns new balance            |
| 4      | WITHDRAW        | No         | Withdraws funds, returns new balance           |
| 5      | MONITOR         | —          | Registers client for account update callbacks  |
| 6      | TRANSFER        | No         | Transfers funds between accounts, returns sender's new balance |
| 7      | CHECK_BALANCE   | Yes        | Returns current balance (read-only)            |

### Monitor / Callbacks

Clients registered via `MONITOR` receive server-initiated `CALLBACK` packets whenever any account operation completes. Registrations expire after the requested duration (in seconds). Expired registrations are pruned on the next incoming request.

## Invocation Semantics

### At-Most-Once (`amo`)
- The server caches responses keyed by `ip:port` + 16-byte request ID.
- Duplicate requests return the cached reply without re-executing the operation.
- Safe for non-idempotent operations (deposits, transfers, etc.) under packet loss.

### At-Least-Once (`alo`)
- No caching — every received packet is executed.
- Non-idempotent operations may execute multiple times if the client retries.
- Idempotent operations (CHECK_BALANCE) are safe under retries.

## Testing

### Automated Test Suite

Run the Python test suite against a running server:
```bash
python3 test_all.py
```

The suite targets `127.0.0.1:8014` by default and covers:
1. **OPEN_ACCOUNT** — create test accounts
2. **DEPOSIT** — success, wrong password, account not found, invalid amount
3. **WITHDRAW** — success, wrong password, account not found, insufficient balance, invalid amount
4. **TRANSFER** — success, wrong password, sender not found, insufficient balance
5. **At-most-once cache** — duplicate request ID returns identical cached response
6. **CHECK_BALANCE** — success, wrong password, account not found
7. **CLOSE_ACCOUNT** — success, wrong password, account not found

### Monitor Test

Listen for account update callbacks in a separate terminal:
```bash
python3 test_monitor.py [duration_secs]   # default: 30
```

Then run operations in another terminal to trigger callbacks. Each callback prints the triggering operation, account number, holder name, balance, currency, and event description.

## Implementation Details

### Marshalling/Unmarshalling
All data is manually marshalled to byte arrays using network byte order (big-endian):

- **Integers** (4 bytes): `write_uint` / `read_uint`
- **Floats** (4 bytes): `write_float` / `read_float`
- **Strings**: 2-byte length prefix + UTF-8 bytes
- **Passwords**: Fixed 8 bytes (zero-padded if shorter)
- **Enums**: Single byte values
- **Request ID**: 16-byte UUID

### Wire Format

**Request packet:**

| Offset | Field       | Size  |
|--------|-------------|-------|
| 0      | msg_type    | 1 B   |
| 1–16   | request_id  | 16 B  |
| 17     | opcode      | 1 B   |
| 18+    | arguments   | varies |

**Response packet:**

| Offset | Field       | Size  |
|--------|-------------|-------|
| 0      | msg_type    | 1 B   |
| 1–16   | request_id  | 16 B  |
| 17     | opcode      | 1 B   |
| 18     | status      | 1 B   |
| 19+    | body        | varies |

**Callback packet (server → monitoring client):**

| Offset | Field             | Size   |
|--------|-------------------|--------|
| 0      | msg_type (= 2)    | 1 B    |
| 1      | trigger_opcode    | 1 B    |
| 2      | account_num       | 4 B    |
| 6      | holder_name       | 2+N B  |
| …      | balance           | 4 B    |
| …      | currency          | 1 B    |
| …      | event_description | 2+N B  |

### Supported Currencies

`SGD` (0), `USD` (1), `INR` (2), `AUD` (3), `CNY` (4), `EUR` (5), `CAD` (6), `GBP` (7), `CHF` (8)

### Source Files

| File               | Description                                      |
|--------------------|--------------------------------------------------|
| `server.cpp`       | Main event loop, request dispatch, AMO cache     |
| `handlers.cpp/h`   | Per-opcode argument parsers                      |
| `account_store.cpp/h` | SQLite-backed account CRUD operations         |
| `marshaller.cpp/h` | Binary serialisation/deserialisation helpers     |
| `udp_socket.cpp/h` | UDP send/receive abstraction                     |
| `protocol.h`       | Wire constants, enums, and packet layout comments|
