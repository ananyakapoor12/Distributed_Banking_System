# Banking Client

A Java UDP client for the Distributed Banking System. Implements both **at-least-once** and **at-most-once** invocation semantics with timeout/retry logic and simulated packet loss for testing.

## Project Structure

```
client_implementation/
├── src/client/
│   ├── Main.java                    # Entry point
│   ├── core/
│   │   ├── BankingClient.java       # High-level banking operations
│   │   └── UDPClient.java           # Low-level UDP communication
│   ├── protocol/
│   │   ├── Protocol.java            # Protocol constants and enums
│   │   ├── Marshaller.java          # Byte marshalling/unmarshalling
│   │   ├── MessageBuilder.java      # Request message construction
│   │   └── MessageParser.java       # Response message parsing
│   ├── ui/
│   │   └── ConsoleUI.java           # Interactive text interface
│   └── utils/
│       └── TestScenarios.java       # Automated testing
├── build.sh                         # Linux/Mac build script
├── build.bat                        # Windows build script
└── README.md
```

## Building the Client

**Prerequisites:** Java Development Kit (JDK) 8 or higher.

**On Linux/Mac:**
```bash
./build.sh
```

**On Windows:**
```batch
build.bat
```

This will compile all Java files and create `dist/BankingClient.jar`.

## Running the Client

**Using the JAR file:**
```bash
java -jar dist/BankingClient.jar <server_host> <server_port> <semantics>
```

**Examples:**
```bash
# Connect to localhost with at-least-once semantics
java -jar dist/BankingClient.jar localhost 2222 at-least-once

# Connect to remote server with at-least-once semantics
java -jar dist/BankingClient.jar 192.168.1.100 2222 at-least-once
```

**Using compiled classes directly:**
```bash
# Connect to localhost with at-least-once semantics
java -cp build client.Main localhost 2222 at-least-once
```

```bash
# Connect to remote server with at-most-once semantics
java -cp build client.Main <server_host> <server_port> <semantics>
```

### Parameters
- `server_host`: IP address or hostname (default: `localhost`)
- `server_port`: Port number (default: `2222`)
- `semantics`: `at-least-once` or `at-most-once` (default: `at-least-once`)

## Using the Client

The client provides an interactive menu:

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

Then perform operations and observe retry behaviour.

### Automated Tests

Run the test suite against a running server:
```bash
java -cp build client.utils.TestScenarios <server_host> <server_port> <semantics>
```

**Examples:**
```bash
# Test with at-most-once semantics
java -cp build client.utils.TestScenarios localhost 2222 at-most-once

# Test with at-least-once semantics
java -cp build client.utils.TestScenarios localhost 2222 at-least-once
```

The test suite includes:
1. **CHECK_BALANCE test** — demonstrates idempotent operation safety
2. **DEPOSIT test** — shows non-idempotent operation risks
3. **TRANSFER test** — compares semantics for money transfers

## Implementation Details

### Marshalling/Unmarshalling

All data is manually marshalled to byte arrays using network byte order (big-endian):

- **Integers** (4 bytes): `ByteBuffer.putInt()` / `getInt()`
- **Floats** (4 bytes): `ByteBuffer.putFloat()` / `getFloat()`
- **Strings**: 2-byte length prefix + UTF-8 bytes
- **Passwords**: Fixed 8 bytes (zero-padded if shorter)
- **Enums**: Single byte values
- **Request ID**: 16-byte UUID

### Fault Tolerance

**Timeout & Retry:**
- Timeout: 3 seconds per attempt
- Max retries: 3 attempts
- Same request ID reused across all retries for a given operation

**At-Most-Once:**
- Client sends the same request ID on each retry
- Server detects the duplicate and returns the cached reply without re-executing
- Guarantees non-idempotent operations execute at most once

**At-Least-Once:**
- Client retries on timeout with the same request ID
- Server re-executes every received packet — no deduplication
- Non-idempotent operations may execute more than once if a reply is lost
