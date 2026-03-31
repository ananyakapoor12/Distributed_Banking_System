package client.protocol;

import java.nio.ByteBuffer;
import java.util.Arrays;

/**
 * MessageParser handles parsing of response and callback messages from the server.
 */
public class MessageParser {
    
    /**
     * Represents a parsed response message
     */
    public static class Response {
        public Protocol.MessageType messageType;
        public byte[] requestId;
        public Protocol.Opcode opcode;
        public Protocol.Status status;
        public Object data;  // Type depends on opcode and status
        public String errorMessage;
        
        @Override
        public String toString() {
            StringBuilder sb = new StringBuilder();
            sb.append("Response{\n");
            sb.append("  messageType=").append(messageType).append("\n");
            sb.append("  requestId=").append(Marshaller.requestIdToString(requestId)).append("\n");
            sb.append("  opcode=").append(opcode).append("\n");
            sb.append("  status=").append(status).append("\n");
            if (status != null && status.isSuccess()) {
                sb.append("  data=").append(data).append("\n");
            } else {
                sb.append("  errorMessage=").append(errorMessage).append("\n");
            }
            sb.append("}");
            return sb.toString();
        }
    }
    
    /**
     * Represents a parsed callback message
     */
    public static class Callback {
        public Protocol.MessageType messageType;
        public Protocol.Opcode triggerOpcode;
        public int accountNum;
        public String holderName;
        public float balance;
        public Protocol.Currency currency;
        public String eventDescription;
        
        @Override
        public String toString() {
            return String.format("Callback{opcode=%s, account=%d, holder='%s', balance=%.2f %s, event='%s'}",
                    triggerOpcode, accountNum, holderName, balance, currency, eventDescription);
        }
    }
    
    /**
     * Parse a response message from the server
     */
    public static Response parseResponse(byte[] data, int length) {
        Response response = new Response();
        ByteBuffer buffer = Marshaller.wrapBuffer(Arrays.copyOf(data, length));
        
        // Parse header
        response.messageType = Protocol.MessageType.fromByte(buffer.get());
        response.requestId = new byte[Protocol.REQUEST_ID_LEN];
        buffer.get(response.requestId);
        response.opcode = Protocol.Opcode.fromByte(buffer.get());
        response.status = Protocol.Status.fromByte(buffer.get());
        
        // Parse body based on status
        if (response.status.isSuccess()) {
            response.data = parseSuccessBody(buffer, response.opcode);
        } else {
            response.errorMessage = Marshaller.unmarshalString(buffer);
        }
        
        return response;
    }
    
    /**
     * Parse success response body based on opcode
     */
    private static Object parseSuccessBody(ByteBuffer buffer, Protocol.Opcode opcode) {
        switch (opcode) {
            case OPEN_ACCOUNT:
                // Returns: account_num (4 bytes)
                return new OpenAccountResult(Marshaller.unmarshalInt(buffer));
                
            case CLOSE_ACCOUNT:
                // Returns: empty (acknowledgement implied by SUCCESS)
                return new CloseAccountResult();
                
            case DEPOSIT:
            case WITHDRAW:
                // Returns: new_balance (4 bytes float), currency (1 byte)
                float balance = Marshaller.unmarshalFloat(buffer);
                Protocol.Currency currency = Protocol.Currency.fromByte(buffer.get());
                return new BalanceResult(balance, currency);
                
            case MONITOR:
                // Returns: empty (client waits for callbacks)
                return new MonitorResult();
                
            case TRANSFER:
                // Returns: sender_new_balance (4 bytes float), currency (1 byte)
                float newBalance = Marshaller.unmarshalFloat(buffer);
                Protocol.Currency curr = Protocol.Currency.fromByte(buffer.get());
                return new TransferResult(newBalance, curr);
                
            case CHECK_BALANCE:
                // Returns: balance (4 bytes float), currency (1 byte)
                float bal = Marshaller.unmarshalFloat(buffer);
                Protocol.Currency cur = Protocol.Currency.fromByte(buffer.get());
                return new BalanceResult(bal, cur);
                
            default:
                return null;
        }
    }
    
    /**
     * Parse a callback message from the server
     */
    public static Callback parseCallback(byte[] data, int length) {
        Callback callback = new Callback();
        ByteBuffer buffer = Marshaller.wrapBuffer(Arrays.copyOf(data, length));
        
        // Parse callback header and body
        callback.messageType = Protocol.MessageType.fromByte(buffer.get());
        callback.triggerOpcode = Protocol.Opcode.fromByte(buffer.get());
        callback.accountNum = Marshaller.unmarshalInt(buffer);
        callback.holderName = Marshaller.unmarshalString(buffer);
        callback.balance = Marshaller.unmarshalFloat(buffer);
        callback.currency = Protocol.Currency.fromByte(buffer.get());
        callback.eventDescription = Marshaller.unmarshalString(buffer);
        
        return callback;
    }
    
    // ─── Result classes ───────────────────────────────────────────────────────────
    
    public static class OpenAccountResult {
        public int accountNum;
        
        public OpenAccountResult(int accountNum) {
            this.accountNum = accountNum;
        }
        
        @Override
        public String toString() {
            return "Account Number: " + accountNum;
        }
    }
    
    public static class CloseAccountResult {
        @Override
        public String toString() {
            return "Account closed successfully";
        }
    }
    
    public static class BalanceResult {
        public float balance;
        public Protocol.Currency currency;
        
        public BalanceResult(float balance, Protocol.Currency currency) {
            this.balance = balance;
            this.currency = currency;
        }
        
        @Override
        public String toString() {
            return String.format("Balance: %.2f %s", balance, currency);
        }
    }
    
    public static class MonitorResult {
        @Override
        public String toString() {
            return "Monitoring registered. Waiting for updates...";
        }
    }
    
    public static class TransferResult {
        public float senderNewBalance;
        public Protocol.Currency currency;
        
        public TransferResult(float senderNewBalance, Protocol.Currency currency) {
            this.senderNewBalance = senderNewBalance;
            this.currency = currency;
        }
        
        @Override
        public String toString() {
            return String.format("Transfer successful. New balance: %.2f %s", senderNewBalance, currency);
        }
    }
}
