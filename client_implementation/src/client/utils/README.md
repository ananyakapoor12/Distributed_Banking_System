# Experiment: At-Least-Once vs At-Most-Once Invocation Semantics

## Overview

This experiment evaluates the behavioural difference between **At-Least-Once (ALO)** and
**At-Most-Once (AMO)** invocation semantics in a UDP-based distributed banking system.
Two operation types are tested:

- **Idempotent** — `CHECK_BALANCE`: read-only, repeating it does not change state.
- **Non-Idempotent** — `TRANSFER`: modifies account balances; repeating it causes
  incorrect results.

---

## Experimental Setup

| Parameter | Value |
|-----------|-------|
| Requests per cell (N) | 20 |
| Loss rates tested | 30%, 50% |
| Idempotent operation | CHECK_BALANCE |
| Non-idempotent operation | TRANSFER (SGD 50.00 per request) |
| Max retries per request | 3 attempts |
| Timeout per attempt | 3 seconds |
| Loss type — idempotent | Packet loss (request or response dropped randomly) |
| Loss type — non-idempotent | Response-only loss (request always reaches server, reply dropped) |

**Why response-only loss for non-idempotent?**
Response-only loss guarantees the server executes the operation on every attempt.
When the client retries with the same request ID:
- Under ALO the server re-executes the transfer → extra money is moved → inconsistent.
- Under AMO the server detects the duplicate ID and returns the cached reply → no
  re-execution → consistent.

**Metrics measured:**
- **Success Rate** — percentage of requests that eventually received a valid response
  within the retry limit.
- **Consistency** — for non-idempotent operations only: whether the final account
  balance matched the expected value (i.e. each unique logical request executed
  exactly once).

---

## How to Run

**Step 1 — ALO server + ALO client**
```bash
# Terminal 1
cd server && ./server 2222 alo

# Terminal 2
cd client_implementation
javac -d build -sourcepath src src/client/utils/ExperimentRunner.java
java -cp build client.utils.ExperimentRunner <server_ip> 2222 at-least-once
```

**Step 2 — AMO server + AMO client**
```bash
# Terminal 1 — Ctrl+C, then restart
./server 2222 amo

# Terminal 2
java -cp build client.utils.ExperimentRunner <server_ip> 2222 at-most-once
```

---

## Results

### Idempotent Operations — CHECK_BALANCE

| Operation | Loss Rate | At-Least-Once (Success Rate) | At-Most-Once (Success Rate) |
|-----------|-----------|------------------------------|-----------------------------|
| CHECK_BALANCE | 30% | 95.0% | 85.0% |
| CHECK_BALANCE | 50% | 55.0% | 65.0% |

**Observation:** Both semantics achieve similar success rates for idempotent operations.
Since `CHECK_BALANCE` is read-only, retrying it under ALO produces the same result as
under AMO — consistency is not affected by either semantic. Minor variation in success
rates between runs is expected due to the random nature of simulated packet loss.

---

### Non-Idempotent Operations — TRANSFER

| Operation | Loss Rate | ALO Success Rate | AMO Success Rate | ALO Consistency | AMO Consistency |
|-----------|-----------|-----------------|-----------------|-----------------|-----------------|
| TRANSFER | 30% | 100.0% | 95.0% | 45.0% | 100.0% |
| TRANSFER | 50% | 80.0% | 95.0% | 35.0% | 100.0% |

**Observation:**
- **AMO Consistency is always 100%** — the server's response cache ensures each unique
  request ID is executed at most once, regardless of how many times the client retries.
- **ALO Consistency drops significantly** — at 30% response loss, only 45% of transfers
  produced the correct final balance. At 50% loss this drops further to 35%, because
  more retries occur and each retry re-executes the transfer, moving extra money.
- **Success rates are comparable** between ALO and AMO — both semantics deliver a
  response to the client at similar rates. The critical difference is correctness of
  the server-side state, not reachability.

---

## Analysis

### Why ALO Fails for Non-Idempotent Operations

Under ALO, when a response is lost in transit, the client retries with the **same
request ID**. The server has no memory of prior executions, so it treats each incoming
message as a fresh request and executes the transfer again. This leads to:

- More money deducted from the sender than intended.
- More money credited to the receiver than intended.
- Final balances diverge from expected values → **inconsistent state**.

### Why AMO Guarantees Consistency

Under AMO, the server maintains a cache keyed by `(client ip:port → request_id → reply bytes)`.
On receiving a retry with an already-seen request ID, the server returns the cached
reply **without re-executing** the operation. The account state is modified exactly
once per unique logical request, regardless of how many network-level retries occur.

### Why Idempotent Operations Are Safe Under Both Semantics

`CHECK_BALANCE` is a read-only operation — executing it multiple times does not change
any account state. Whether the server processes a retry or returns a cached response,
the result is always the same. This is why ALO and AMO show similar behaviour for
idempotent operations.

---

## Files

| File | Description |
|------|-------------|
| `ExperimentRunner.java` | Runs all experiments and prints result tables |
| `TestScenarios.java` | Manual test cases for individual operation scenarios |
