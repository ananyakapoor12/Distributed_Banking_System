# Distributed Banking System - Java Client

A comprehensive Java client implementation for the Distributed Banking System project (CE4013/CZ4013/SC4051). This client communicates with the C++ server using UDP sockets and implements both **at-least-once** and **at-most-once** invocation semantics.

## Features

### Core Banking Operations
1. **Open Account** - Create new bank account with initial balance
2. **Close Account** - Close existing account
3. **Deposit** - Add money to account (non-idempotent)
4. **Withdraw** - Remove money from account (non-idempotent)
5. **Monitor** - Real-time monitoring of account updates via callbacks

### Extra Services
6. **Transfer** - Send money between accounts (**non-idempotent**)
7. **Check Balance** - Query account balance (**idempotent**)

### Advanced Features
- Full marshalling/unmarshalling implementation (no Java serialization)
- Both invocation semantics (at-least-once & at-most-once)
- Timeout and retry logic with configurable attempts
- Request deduplication and response caching
- Simulated packet loss for testing
- Interactive console UI
- Automated test suite
- Comprehensive error handling

## Project Structure

```
client_implementation/
├── src/
│   └── client/
│       ├── Main.java                    # Entry point
│       ├── core/
│       │   ├── BankingClient.java       # High-level banking operations
│       │   └── UDPClient.java           # Low-level UDP communication
│       ├── protocol/
│       │   ├── Protocol.java            # Protocol constants and enums
│       │   ├── Marshaller.java          # Byte marshalling/unmarshalling
│       │   ├── MessageBuilder.java      # Request message construction
│       │   └── MessageParser.java       # Response message parsing
│       ├── ui/
│       │   └── ConsoleUI.java           # Interactive text interface
│       └── utils/
│           └── TestScenarios.java       # Automated testing
├── build.sh                             # Linux/Mac build script
├── build.bat                            # Windows build script
└── README.md                            # This file
```

## Quick Start

### Prerequisites
- Java Development Kit (JDK) 8 or higher
- C++ server running and accessible
- Network access for UDP communication

### Building the Client

**On Linux/Mac:**
```bash
./build.sh
```

**On Windows:**
```batch
build.bat
```

This will compile all Java files and create `dist/BankingClient.jar`.

### Running the Client

**Using the JAR file:**
```bash
java -jar dist/BankingClient.jar <server_host> <server_port> <semantics>
```

**Examples:**
```bash
# Connect to localhost with at-least-once semantics
java -jar dist/BankingClient.jar localhost 2222 at-least-once

# Connect to remote server with at-most-once semantics
java -jar dist/BankingClient.jar 192.168.1.100 2222 at-most-once
```

**Using compiled classes:**
```bash
java -cp build client.Main localhost 2222 at-most-once
```

### Parameters
- `server_host`: IP address or hostname (default: localhost)
- `server_port`: Port number (default: 2222)
- `semantics`: `at-least-once` or `at-most-once` (default: at-least-once)

## Using the Client

The client provides an interactive menu with the following options:

```
┌─────────────────────────────────────────────────────────┐
│                      MAIN MENU                          │
├─────────────────────────────────────────────────────────┤
│  1. Open New Account                                    │
│  2. Close Account                                       │
│  3. Deposit Money                                       │
│  4. Withdraw Money                                      │
│  5. Transfer Money (Non-Idempotent)                     │
│  6. Check Balance (Idempotent)                          │
│  7. Monitor Account Updates                             │
│  8. Simulate Packet Loss (Testing)                      │
│  9. Exit                                                │
└─────────────────────────────────────────────────────────┘
```

### Example Session

1. **Open an account:**
   - Select option 1
   - Enter name, password (max 8 chars), currency, initial balance
   - Note the account number returned

2. **Deposit money:**
   - Select option 3
   - Enter account number, credentials, amount
   - See updated balance

3. **Transfer money:**
   - Select option 5
   - Enter sender and receiver account details
   - Confirm transfer

## Testing Invocation Semantics

### Interactive Testing
Use option 8 to enable packet loss simulation:
```
Enter packet loss rate: 0.3  (for 30% loss)
```

Then perform operations and observe retry behavior.

### Automated Tests
Run the test suite:
```bash
java -cp build client.utils.TestScenarios <server_host> <server_port> <semantics>
```

**Example:**
```bash
# Test with at-most-once semantics
java -cp build client.utils.TestScenarios localhost 2222 at-most-once

# Test with at-least-once semantics
java -cp build client.utils.TestScenarios localhost 2222 at-least-once
```

The test suite includes:
1. **CHECK_BALANCE test** - Demonstrates idempotent operation safety
2. **DEPOSIT test** - Shows non-idempotent operation risks
3. **TRANSFER test** - Compares semantics for money transfers

### Expected Results

**At-Most-Once Semantics:**
- Non-idempotent operations execute exactly once
- Duplicate requests return cached response
- Safe for transfers and deposits even with packet loss

**At-Least-Once Semantics:**
- Non-idempotent operations may execute multiple times
- Can lead to duplicate deposits/transfers
- Idempotent operations (CHECK_BALANCE) work fine

## Implementation Details

### Marshalling/Unmarshalling
All data is manually marshalled to byte arrays using network byte order (big-endian):

- **Integers** (4 bytes): `ByteBuffer.putInt()` / `getInt()`
- **Floats** (4 bytes): `ByteBuffer.putFloat()` / `getFloat()`
- **Strings**: 2-byte length prefix + UTF-8 bytes
- **Passwords**: Fixed 8 bytes (zero-padded if shorter)
- **Enums**: Single byte values
- **Request ID**: 16-byte UUID

### Wire Protocol

**Request Format:**
```
[message_type:1][request_id:16][opcode:1][arguments:varies]
```

**Response Format:**
```
[message_type:1][request_id:16][opcode:1][status:1][body:varies]
```

**Callback Format:**
```
[message_type:1][opcode:1][account_num:4][name:var][balance:4][currency:1][description:var]
```

### Fault Tolerance

**Timeout & Retry:**
- Timeout: 3 seconds per attempt
- Max retries: 3 attempts
- Same request ID used across retries

**At-Most-Once Implementation:**
- Server maintains request history
- Duplicate requests return cached response
- Client caches responses locally

**At-Least-Once Implementation:**
- Server re-executes every request
- No deduplication or caching
- Simpler but unsafe for non-idempotent operations

## Monitoring Feature

The monitoring feature demonstrates server-to-client callbacks:

1. Client registers for monitoring with duration (seconds)
2. Client is blocked from new requests during monitoring
3. Server sends callbacks for all account updates:
   - Account opened/closed
   - Deposits/withdrawals
   - Transfers
4. Client displays updates in real-time
5. Monitoring expires automatically after duration

**Demo Setup:**
- Terminal 1: Run monitoring client
- Terminal 2: Run regular client making transactions
- Terminal 1: Observe real-time updates

## Multi-Language Implementation

This Java client interoperates with the C++ server through:
- Platform-independent UDP sockets
- Network byte order (big-endian) for all multi-byte values
- UTF-8 encoding for strings
- Standardized wire protocol

**Key Considerations:**
- Java uses big-endian by default (via `ByteBuffer`)
- C++ server uses `htonl()`/`ntohl()` for byte order
- String encoding must match (UTF-8)
- Struct padding differences handled by explicit marshalling


## Important Notes

1. **Password Length**: Maximum 8 bytes (enforced by protocol)
2. **Concurrent Clients**: Server handles one request at a time (per specification)
3. **Network**: Ensure UDP traffic is allowed through firewall
4. **Testing**: Use packet loss simulation to test fault tolerance

## Troubleshooting

**"Connection refused" or timeout:**
- Check server is running: `netstat -an | grep 2222`
- Verify server host/port are correct
- Check firewall settings

**"Compilation failed":**
- Ensure JDK is installed: `java -version`
- Check JAVA_HOME environment variable
- Verify all source files are present

**Unexpected behavior with packet loss:**
- This is intentional for testing.
- Disable simulation (option 8, rate 0.0)
- Observe retry messages in console

## License

This project is for educational purposes as part of SC4051 Distributed Systems course at NTU.

---
**NTU College of Computing and Data Science**
**Session 2025/2026 Semester 2**
