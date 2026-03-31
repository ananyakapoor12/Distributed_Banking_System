package client.protocol;

/**
 * Protocol constants matching the C++ server implementation.
 * Wire format specifications for request/response/callback messages.
 */
public class Protocol {
    
    // ─── Wire constants ───────────────────────────────────────────────────────────
    public static final int MAX_BUF_SIZE = 1024;        // max UDP payload (bytes)
    public static final int REQUEST_ID_LEN = 16;        // 128-bit UUID
    public static final int PASSWORD_LEN = 8;           // fixed-length password (bytes)
    public static final int HEADER_SIZE = 1 + REQUEST_ID_LEN + 1; // msg_type + request_id + opcode/status
    
    // ─── Message types ────────────────────────────────────────────────────────────
    public enum MessageType {
        REQUEST((byte) 0),
        RESPONSE((byte) 1),
        CALLBACK((byte) 2);
        
        private final byte value;
        
        MessageType(byte value) {
            this.value = value;
        }
        
        public byte getValue() {
            return value;
        }
        
        public static MessageType fromByte(byte b) {
            for (MessageType type : values()) {
                if (type.value == b) return type;
            }
            throw new IllegalArgumentException("Unknown MessageType: " + b);
        }
    }
    
    // ─── Invocation semantics ─────────────────────────────────────────────────────
    public enum Semantics {
        AT_LEAST_ONCE,      // retransmit on timeout; server re-executes every duplicate
        AT_MOST_ONCE        // server caches replies; duplicates get cached response
    }
    
    // ─── Opcodes ──────────────────────────────────────────────────────────────────
    public enum Opcode {
        OPEN_ACCOUNT((byte) 1),
        CLOSE_ACCOUNT((byte) 2),
        DEPOSIT((byte) 3),
        WITHDRAW((byte) 4),
        MONITOR((byte) 5),
        TRANSFER((byte) 6),          // non-idempotent
        CHECK_BALANCE((byte) 7);     // idempotent
        
        private final byte value;
        
        Opcode(byte value) {
            this.value = value;
        }
        
        public byte getValue() {
            return value;
        }
        
        public static Opcode fromByte(byte b) {
            for (Opcode op : values()) {
                if (op.value == b) return op;
            }
            throw new IllegalArgumentException("Unknown Opcode: " + b);
        }
    }
    
    // ─── Status codes ─────────────────────────────────────────────────────────────
    public enum Status {
        // General / Protocol-Level
        SUCCESS((byte) 0),
        UNKNOWN_OPERATION((byte) 1),
        MALFORMED_REQUEST((byte) 2),
        CORRUPT_REQUEST((byte) 3),
        DUPLICATE_REQUEST((byte) 4),
        
        // Authentication / Identity
        AUTH_FAILED((byte) 10),
        ACCOUNT_NOT_FOUND((byte) 11),
        NAME_ACCOUNT_MISMATCH((byte) 12),
        
        // Account State
        INSUFFICIENT_BALANCE((byte) 20),
        ACCOUNT_ALREADY_EXISTS((byte) 21),
        ACCOUNT_ALREADY_CLOSED((byte) 22),
        ACCOUNT_NOT_CLOSED((byte) 23),
        
        // Operation-Specific
        INVALID_CURRENCY((byte) 30),
        CURRENCY_MISMATCH((byte) 31),
        INVALID_AMOUNT((byte) 32),
        
        // Monitoring / Callback
        MONITOR_REGISTRATION_FAILED((byte) 40),
        MONITOR_INTERVAL_INVALID((byte) 41),
        NOT_REGISTERED_FOR_MONITOR((byte) 42);
        
        private final byte value;
        
        Status(byte value) {
            this.value = value;
        }
        
        public byte getValue() {
            return value;
        }
        
        public static Status fromByte(byte b) {
            for (Status status : values()) {
                if (status.value == b) return status;
            }
            return null; // Return null for unknown status codes
        }
        
        public boolean isSuccess() {
            return this == SUCCESS;
        }
    }
    
    // ─── Currency enumeration ─────────────────────────────────────────────────────
    public enum Currency {
        SGD((byte) 0),
        USD((byte) 1),
        INR((byte) 2),
        AUD((byte) 3),
        CNY((byte) 4),
        EUR((byte) 5),
        CAD((byte) 6),
        GBP((byte) 7),
        CHF((byte) 8);
        
        private final byte value;
        
        Currency(byte value) {
            this.value = value;
        }
        
        public byte getValue() {
            return value;
        }
        
        public static Currency fromByte(byte b) {
            for (Currency currency : values()) {
                if (currency.value == b) return currency;
            }
            throw new IllegalArgumentException("Unknown Currency: " + b);
        }
    }
}
