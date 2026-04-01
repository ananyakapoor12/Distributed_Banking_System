### Building the Server

**Prerequisites:**
- C++17 compiler (`g++`)
- [SQLiteCpp](https://github.com/SRombauts/SQLiteCpp) library (install via Homebrew: `brew install sqlitecpp`)

**Compile:**
```bash
g++ -std=c++17 -DDEBUG -I$(brew --prefix sqlitecpp)/include \
    -o server server.cpp udp_socket.cpp marshaller.cpp account_store.cpp handlers.cpp \
    -L$(brew --prefix sqlitecpp)/lib -lSQLiteCpp -lsqlite3 -lpthread -ldl
```

Remove `-DDEBUG` to suppress debug output:
```bash
g++ -std=c++17 -I$(brew --prefix sqlitecpp)/include \
    -o server server.cpp udp_socket.cpp marshaller.cpp account_store.cpp handlers.cpp \
    -L$(brew --prefix sqlitecpp)/lib -lSQLiteCpp -lsqlite3 -lpthread -ldl
```

### Running the Server

```bash
./server <ip> <port> <at-most-once|at-least-once>
```

**Examples:**
```bash
# Run on 127.0.0.1:8014 with at-most-once semantics
./server 127.0.0.1 8014 at-most-once

# Run on 0.0.0.0:2222 with at-least-once semantics
./server 0.0.0.0 2222 at-least-once
```

### Parameters
- `ip`: IP address to bind to (e.g. `127.0.0.1` or `0.0.0.0` for all interfaces)
- `port`: UDP port number to listen on
- `at-most-once|at-least-once`: Invocation semantics

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

### At-Most-Once
- The server caches responses keyed by `ip:port` + 16-byte request ID.
- Duplicate requests return the cached reply without re-executing the operation.
- Safe for non-idempotent operations (deposits, transfers, etc.) under packet loss.

### At-Least-Once 
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

### Source Files

| File               | Description                                      |
|--------------------|--------------------------------------------------|
| `server.cpp`       | Main event loop, request dispatch, AMO cache     |
| `handlers.cpp/h`   | Per-opcode argument parsers                      |
| `account_store.cpp/h` | SQLite-backed account CRUD operations         |
| `marshaller.cpp/h` | Binary serialisation/deserialisation helpers     |
| `udp_socket.cpp/h` | UDP send/receive abstraction                     |
| `protocol.h`       | Wire constants, enums, and packet layout comments|
