#pragma once

#include <cstdint>

// ─── Wire constants ───────────────────────────────────────────────────────────

static constexpr int    MAX_BUF_SIZE     = 1024;  // max UDP payload (bytes)
static constexpr int    REQUEST_ID_LEN   = 16;    // 128-bit UUID
static constexpr int    PASSWORD_LEN     = 8;     // fixed-length password (bytes)
static constexpr size_t HEADER_SIZE      = 1 + REQUEST_ID_LEN + 1;
//                                         ^                    ^
//                                     msg_type             opcode/status

// ─── Message type (1 byte) ────────────────────────────────────────────────────
// Tells the receiver whether this packet is a client request,
// a server response, or a server-initiated callback.

enum class MessageType : uint8_t {
    REQUEST  = 0,
    RESPONSE = 1,
    CALLBACK = 2   // server → monitoring client
};

// ─── Currency codes (1 byte) ──────────────────────────────────────────────────

enum class Currency : uint8_t {
    SGD = 0,
    USD = 1,
    INR = 2,
    AUD = 3,
    CNY = 4,
    EUR = 5,
    CAD = 6,
    GBP = 7,
    CHF = 8
};

inline std::string toString(Currency c)
{
    switch (c)
    {
    case Currency::SGD:
        return "SGD";
    case Currency::USD:
        return "USD";
    case Currency::INR:
        return "INR";
    case Currency::AUD:
        return "AUD";
    case Currency::CNY:
        return "CNY";
    case Currency::EUR:
        return "EUR";
    case Currency::CAD:
        return "CAD";
    case Currency::GBP:
        return "GBP";
    case Currency::CHF:
        return "CHF";
    default:
        return "UNKNOWN";
    }
}

inline std::ostream &operator<<(std::ostream &os, Currency c)
{
    return os << toString(c);
}

// ─── Opcodes (1 byte) ─────────────────────────────────────────────────────────
// Sent in every REQUEST to identify which service to invoke.
// Also echoed in the RESPONSE header so the client can dispatch the reply.
//
//  Required services (from spec):
//    1. OPEN_ACCOUNT    — non-idempotent (creates a new account each call)
//    2. CLOSE_ACCOUNT   — non-idempotent
//    3. DEPOSIT         — non-idempotent
//    4. WITHDRAW        — non-idempotent
//    5. MONITOR         — registers a callback listener for a time interval
//
//  Extra services (self-designed):
//    6. TRANSFER        — non-idempotent (send money to another account)
//    7. CHECK_BALANCE   — idempotent     (read-only, safe to re-execute)

enum class Opcode : uint8_t {
    OPEN_ACCOUNT   = 1,
    CLOSE_ACCOUNT  = 2,
    DEPOSIT        = 3,
    WITHDRAW       = 4,
    MONITOR        = 5,
    TRANSFER       = 6,
    CHECK_BALANCE  = 7
};

// ─── Status codes (1 byte) ────────────────────────────────────────────────────
// First byte of every RESPONSE body.

enum class Status : uint8_t {
    // General / Protocol-Level
    SUCCESS                  = 0,
    UNKNOWN_OPERATION        = 1,
    MALFORMED_REQUEST        = 2,   // arguments aren't in the right order
    CORRUPT_REQUEST          = 3,   // checksum didn't match
    DUPLICATE_REQUEST        = 4,   // for at-most-once filtering

    // Authentication / Identity
    AUTH_FAILED              = 10,  // wrong password
    ACCOUNT_NOT_FOUND        = 11,
    NAME_ACCOUNT_MISMATCH    = 12,  // account number not under given name

    // Account State
    INSUFFICIENT_BALANCE     = 20,
    ACCOUNT_ALREADY_EXISTS   = 21,
    ACCOUNT_ALREADY_CLOSED   = 22,
    ACCOUNT_NOT_CLOSED       = 23,  // closing fails due to rule

    // Operation-Specific
    INVALID_CURRENCY         = 30,
    CURRENCY_MISMATCH        = 31,
    INVALID_AMOUNT           = 32,  // negative or zero amount where not allowed

    // Monitoring / Callback
    MONITOR_REGISTRATION_FAILED = 40,
    MONITOR_INTERVAL_INVALID    = 41,
    NOT_REGISTERED_FOR_MONITOR  = 42,
};

// ─── Request wire layout ──────────────────────────────────────────────────────
//
//  Byte offset   Field              Size
//  ──────────────────────────────────────────────────────
//  0             message_type       1 B   (MessageType::REQUEST = 0)
//  1–16          request_id         16 B  (UUID — unique per client request)
//  17            opcode             1 B   (Opcode enum)
//  18+           arguments          varies by opcode (see below)
//
//  Per-opcode argument layout (all multi-byte fields in network byte order):
//
//  OPEN_ACCOUNT
//    name        : 2-byte length  + N bytes
//    password    : PASSWORD_LEN bytes (fixed, no length prefix)
//    currency    : 1 B
//    balance     : 4 B  (float via htonl trick)
//
//  CLOSE_ACCOUNT
//    account_num : 4 B
//    name        : 2-byte length  + N bytes
//    password    : PASSWORD_LEN bytes
//
//  DEPOSIT / WITHDRAW
//    account_num : 4 B
//    name        : 2-byte length  + N bytes
//    password    : PASSWORD_LEN bytes
//    currency    : 1 B
//    amount      : 4 B  (float)
//
//  MONITOR
//    duration    : 4 B  (seconds, uint32)
//
//  TRANSFER
//    sender_account_num   : 4 B
//    sender_name          : 2-byte length + N bytes
//    sender_password      : PASSWORD_LEN bytes
//    receiver_account_num : 4 B
//    currency             : 1 B
//    amount               : 4 B  (float)
//
//  CHECK_BALANCE
//    account_num : 4 B
//    name        : 2-byte length + N bytes
//    password    : PASSWORD_LEN bytes

// ─── Response wire layout ─────────────────────────────────────────────────────
//
//  Byte offset   Field              Size
//  ──────────────────────────────────────────────────────
//  0             message_type       1 B   (MessageType::RESPONSE = 1)
//  1–16          request_id         16 B  (echoed from request)
//  17            opcode             1 B   (echoed — lets client dispatch reply)
//  18            status             1 B   (Status enum — see below)
//  19+           body               varies (see below)
//
//  SUCCESS (status = 0) bodies:
//    OPEN_ACCOUNT    → account_num : 4 B
//    CLOSE_ACCOUNT   → (empty — acknowledgement implied by SUCCESS status)
//    DEPOSIT         → new_balance : 4 B (float),  currency : 1 B
//    WITHDRAW        → new_balance : 4 B (float),  currency : 1 B
//    MONITOR         → (empty — client just waits for CALLBACKs)
//    TRANSFER        → sender_new_balance : 4 B (float), currency : 1 B
//    CHECK_BALANCE   → balance : 4 B (float), currency : 1 B
//
//  FAILURE body (all non-zero status codes):
//    error_message : 2-byte length + N bytes
//
//  Status codes (1 byte):
//    General / Protocol-Level
//      0  SUCCESS
//      1  UNKNOWN_OPERATION
//      2  MALFORMED_REQUEST        — arguments aren't in the right order
//      3  CORRUPT_REQUEST          — checksum didn't match
//      4  DUPLICATE_REQUEST        — for at-most-once filtering
//    Authentication / Identity
//      10 AUTH_FAILED              — wrong password
//      11 ACCOUNT_NOT_FOUND
//      12 NAME_ACCOUNT_MISMATCH    — account number not under given name
//    Account State
//      20 INSUFFICIENT_BALANCE
//      21 ACCOUNT_ALREADY_EXISTS
//      22 ACCOUNT_ALREADY_CLOSED
//      23 ACCOUNT_NOT_CLOSED       — closing fails due to rule
//    Operation-Specific
//      30 INVALID_CURRENCY
//      31 CURRENCY_MISMATCH
//      32 INVALID_AMOUNT           — negative or zero amount where not allowed
//    Monitoring / Callback
//      40 MONITOR_REGISTRATION_FAILED
//      41 MONITOR_INTERVAL_INVALID
//      42 NOT_REGISTERED_FOR_MONITOR

// ─── Callback wire layout ─────────────────────────────────────────────────────
//
//  Sent server → monitoring client whenever any account is updated.
//
//  Byte offset   Field              Size
//  ──────────────────────────────────────────────────────
//  0             message_type       1 B   (MessageType::CALLBACK = 2)
//  1             trigger_opcode     1 B   (which operation caused the update)
//  2             account_num        4 B
//  6             holder_name        2-byte length + N bytes
//  …             balance            4 B   (float)
//  …             currency           1 B
//  …             event_description  2-byte length + N bytes  (e.g. "Account opened")

