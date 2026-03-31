# Quick Reference Guide

## Build Commands

### Linux/Mac
```bash
./build.sh
```

### Windows
```batch
build.bat
```

## Run Commands

### Basic Usage
```bash
# Local server with at-least-once
java -jar dist/BankingClient.jar localhost 2222 at-least-once

# Local server with at-most-once
java -jar dist/BankingClient.jar localhost 2222 at-most-once

# Remote server
java -jar dist/BankingClient.jar 192.168.1.100 2222 at-most-once
```

### Run Tests
```bash
# Test at-most-once semantics
java -cp build client.utils.TestScenarios localhost 2222 at-most-once

# Test at-least-once semantics
java -cp build client.utils.TestScenarios localhost 2222 at-least-once
```

## Common Operations

### 1. Create Account
```
Menu Option: 1
Required: Name, Password (≤8 chars), Currency, Initial Balance
Returns: Account Number (save this!)
```

### 2. Deposit Money
```
Menu Option: 3
Required: Account #, Name, Password, Currency, Amount
Returns: New Balance
```

### 3. Transfer (Non-Idempotent)
```
Menu Option: 5
Required: Sender Account, Sender Credentials, Receiver Account, Amount
Returns: Sender's New Balance
Warning: NOT safe to retry! Use at-most-once semantics
```

### 4. Check Balance (Idempotent)
```
Menu Option: 6
Required: Account #, Name, Password
Returns: Current Balance
Safe to retry: Always returns same result
```

### 5. Monitor Updates
```
Menu Option: 7
Required: Duration in seconds
Behavior: Blocks client, displays all account updates in real-time
Demo: Run two clients - one monitoring, one doing transactions
```

## Testing Packet Loss

### Enable Simulation
```
Menu Option: 8
Enter rate: 0.3 (for 30% packet loss)
```

### What to Observe
- Watch console for "[SIMULATED LOSS]" messages
- See "[RETRY]" attempts
- Compare behavior with different semantics

### Disable Simulation
```
Menu Option: 8
Enter rate: 0.0
```

## Demonstration Setup

### For Project Demo

**Terminal 1 - Server:**
```bash
cd your_project/server
./server 2222 at-most-once
```

**Terminal 2 - Monitoring Client:**
```bash
java -jar dist/BankingClient.jar localhost 2222 at-most-once
# Select option 7, duration 60 seconds
```

**Terminal 3 - Transaction Client:**
```bash
java -jar dist/BankingClient.jar localhost 2222 at-most-once
# Perform operations - they appear in Terminal 2!
```

## Invocation Semantics Comparison

### At-Least-Once
- ✅ Simple implementation
- ✅ Guarantees execution
- ⚠️ May execute multiple times
- ❌ Unsafe for transfers, deposits
- Use case: Read-only operations

### At-Most-Once  
- ✅ Executes exactly once
- ✅ Safe for all operations
- ✅ Request deduplication
- ⚠️ Requires caching
- Use case: Production banking system

## Protocol Details

### Message Types
- 0 = REQUEST
- 1 = RESPONSE  
- 2 = CALLBACK

### Opcodes
- 1 = OPEN_ACCOUNT
- 2 = CLOSE_ACCOUNT
- 3 = DEPOSIT
- 4 = WITHDRAW
- 5 = MONITOR
- 6 = TRANSFER (non-idempotent)
- 7 = CHECK_BALANCE (idempotent)

### Status Codes
- 0 = SUCCESS
- 10 = AUTH_FAILED
- 11 = ACCOUNT_NOT_FOUND
- 20 = INSUFFICIENT_BALANCE
- 30 = INVALID_CURRENCY

### Currencies (byte values)
- 0 = SGD, 1 = USD, 2 = INR, 3 = AUD, 4 = CNY
- 5 = EUR, 6 = CAD, 7 = GBP, 8 = CHF

## Troubleshooting

### Server Not Responding
```bash
# Check if server is running
netstat -an | grep 2222

# On Windows
netstat -an | findstr 2222
```

### Compilation Errors
```bash
# Check Java version
java -version
javac -version

# Should be Java 8 or higher
```

### Can't Connect to Remote Server
```bash
# Test connectivity
ping <server_ip>

# Test UDP port (if nc available)
nc -u <server_ip> 2222
```

### Packet Loss Not Working
- Packet loss is SIMULATED in client code
- It's for testing retry logic
- Real network loss is separate

## Report Experiments

### Experiment 1: Idempotent Operation Test
```
Setup: Enable 30% packet loss
Operation: CHECK_BALANCE x3 times
Expected: Same balance every time (idempotent)
Both semantics: Should work correctly
```

### Experiment 2: Non-Idempotent Test (Deposit)
```
Setup: Enable 25% packet loss  
Operation: DEPOSIT $50 three times
At-least-once: May deposit more than 3 times
At-most-once: Exactly $150 deposited
```

### Experiment 3: Transfer Correctness
```
Setup: Two accounts, 30% packet loss
Operation: Transfer $100 from A to B
At-least-once: May transfer multiple times (WRONG!)
At-most-once: Exactly $100 transferred (CORRECT!)
```

## File Locations

```
client_implementation/
├── src/                    # Java source code
├── build/                  # Compiled .class files
├── dist/
│   └── BankingClient.jar  # Executable JAR
├── build.sh               # Linux/Mac build
├── build.bat              # Windows build
└── README.md              # Full documentation
```

## Key Classes

- `Main.java` - Entry point
- `BankingClient.java` - High-level API
- `UDPClient.java` - UDP socket handling
- `Marshaller.java` - Byte conversion
- `MessageBuilder.java` - Request construction
- `MessageParser.java` - Response parsing
- `ConsoleUI.java` - User interface
- `TestScenarios.java` - Automated tests

## Common Issues

**Issue: "Port already in use"**
Solution: Server is already running on that port

**Issue: Timeout on every request**
Solution: Check server address/port, firewall

**Issue: Password too long**  
Solution: Max 8 bytes, will be truncated

**Issue: Wrong account balance**
Solution: Check invocation semantics, packet loss rate

## Contact

For questions about the implementation:
- Check README.md for detailed documentation
- Review source code comments
- Run TestScenarios for examples
