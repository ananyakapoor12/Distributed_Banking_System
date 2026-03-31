package client.protocol;

import java.nio.ByteBuffer;

/**
 * MessageBuilder constructs request messages according to the wire protocol.
 * Field order must match exactly what handlers.cpp reads.
 *
 * IMPORTANT: Password is FIXED 8 bytes (no length prefix).
 * Server uses read_fixed_string(buf, offset, buf_len, 8) to read it.
 */
public class MessageBuilder {

    /**
     * Build OPEN_ACCOUNT request
     * handlers.cpp parse_open_account reads: password(fixed), currency, balance, name(varchar)
     */
    public static byte[] buildOpenAccountRequest(byte[] requestId, String name,
                                                 String password, Protocol.Currency currency,
                                                 float initialBalance) {
        ByteBuffer buffer = Marshaller.createBuffer(Protocol.MAX_BUF_SIZE);

        buffer.put(Protocol.MessageType.REQUEST.getValue());
        buffer.put(requestId);
        buffer.put(Protocol.Opcode.OPEN_ACCOUNT.getValue());

        Marshaller.marshalPassword(buffer, password);    // 1. password (8 bytes fixed)
        buffer.put(currency.getValue());                 // 2. currency (1 byte)
        Marshaller.marshalFloat(buffer, initialBalance); // 3. balance (4 bytes float)
        Marshaller.marshalString(buffer, name);          // 4. name (varchar)

        return extractBytes(buffer);
    }

    /**
     * Build CLOSE_ACCOUNT request
     * handlers.cpp parse_close_account reads: account_num, password(fixed), name(varchar)
     */
    public static byte[] buildCloseAccountRequest(byte[] requestId, int accountNum,
                                                   String name, String password) {
        ByteBuffer buffer = Marshaller.createBuffer(Protocol.MAX_BUF_SIZE);

        buffer.put(Protocol.MessageType.REQUEST.getValue());
        buffer.put(requestId);
        buffer.put(Protocol.Opcode.CLOSE_ACCOUNT.getValue());

        Marshaller.marshalInt(buffer, accountNum);       // 1. account num (4 bytes)
        Marshaller.marshalPassword(buffer, password);    // 2. password (8 bytes fixed)
        Marshaller.marshalString(buffer, name);          // 3. name (varchar)

        return extractBytes(buffer);
    }

    /**
     * Build DEPOSIT request
     * handlers.cpp parse_deposit reads: account_num, password(fixed), currency, amount, name(varchar)
     */
    public static byte[] buildDepositRequest(byte[] requestId, int accountNum,
                                              String name, String password,
                                              Protocol.Currency currency, float amount) {
        ByteBuffer buffer = Marshaller.createBuffer(Protocol.MAX_BUF_SIZE);

        buffer.put(Protocol.MessageType.REQUEST.getValue());
        buffer.put(requestId);
        buffer.put(Protocol.Opcode.DEPOSIT.getValue());

        Marshaller.marshalInt(buffer, accountNum);       // 1. account num (4 bytes)
        Marshaller.marshalPassword(buffer, password);    // 2. password (8 bytes fixed)
        buffer.put(currency.getValue());                 // 3. currency (1 byte)
        Marshaller.marshalFloat(buffer, amount);         // 4. amount (4 bytes float)
        Marshaller.marshalString(buffer, name);          // 5. name (varchar)

        return extractBytes(buffer);
    }

    /**
     * Build WITHDRAW request
     * handlers.cpp parse_withdraw reads: account_num, password(fixed), currency, amount, name(varchar)
     */
    public static byte[] buildWithdrawRequest(byte[] requestId, int accountNum,
                                               String name, String password,
                                               Protocol.Currency currency, float amount) {
        ByteBuffer buffer = Marshaller.createBuffer(Protocol.MAX_BUF_SIZE);

        buffer.put(Protocol.MessageType.REQUEST.getValue());
        buffer.put(requestId);
        buffer.put(Protocol.Opcode.WITHDRAW.getValue());

        Marshaller.marshalInt(buffer, accountNum);       // 1. account num (4 bytes)
        Marshaller.marshalPassword(buffer, password);    // 2. password (8 bytes fixed)
        buffer.put(currency.getValue());                 // 3. currency (1 byte)
        Marshaller.marshalFloat(buffer, amount);         // 4. amount (4 bytes float)
        Marshaller.marshalString(buffer, name);          // 5. name (varchar)

        return extractBytes(buffer);
    }

    /**
     * Build MONITOR request
     * handlers.cpp parse_monitor reads: duration(uint32)
     */
    public static byte[] buildMonitorRequest(byte[] requestId, int durationSeconds) {
        ByteBuffer buffer = Marshaller.createBuffer(Protocol.MAX_BUF_SIZE);

        buffer.put(Protocol.MessageType.REQUEST.getValue());
        buffer.put(requestId);
        buffer.put(Protocol.Opcode.MONITOR.getValue());

        Marshaller.marshalInt(buffer, durationSeconds);  // 1. duration (4 bytes)

        return extractBytes(buffer);
    }

    /**
     * Build TRANSFER request (non-idempotent extra service)
     * handlers.cpp parse_transfer reads:
     *   sender_account_num, sender_password(fixed), receiver_account_num,
     *   amount, currency, sender_name(varchar), receiver_name(varchar)
     */
    public static byte[] buildTransferRequest(byte[] requestId, int senderAccountNum,
                                               String senderName, String senderPassword,
                                               int receiverAccountNum, Protocol.Currency currency,
                                               float amount, String receiverName) {
        ByteBuffer buffer = Marshaller.createBuffer(Protocol.MAX_BUF_SIZE);

        buffer.put(Protocol.MessageType.REQUEST.getValue());
        buffer.put(requestId);
        buffer.put(Protocol.Opcode.TRANSFER.getValue());

        Marshaller.marshalInt(buffer, senderAccountNum);     // 1. sender account num (4 bytes)
        Marshaller.marshalPassword(buffer, senderPassword);  // 2. sender password (8 bytes fixed)
        Marshaller.marshalInt(buffer, receiverAccountNum);   // 3. receiver account num (4 bytes)
        Marshaller.marshalFloat(buffer, amount);             // 4. amount (4 bytes float)
        buffer.put(currency.getValue());                     // 5. currency (1 byte)
        Marshaller.marshalString(buffer, senderName);        // 6. sender name (varchar)
        Marshaller.marshalString(buffer, receiverName);      // 7. receiver name (varchar)

        return extractBytes(buffer);
    }

    /**
     * Build CHECK_BALANCE request (idempotent extra service)
     * handlers.cpp parse_check_balance reads: account_num, password(fixed), name(varchar)
     */
    public static byte[] buildCheckBalanceRequest(byte[] requestId, int accountNum,
                                                   String name, String password) {
        ByteBuffer buffer = Marshaller.createBuffer(Protocol.MAX_BUF_SIZE);

        buffer.put(Protocol.MessageType.REQUEST.getValue());
        buffer.put(requestId);
        buffer.put(Protocol.Opcode.CHECK_BALANCE.getValue());

        Marshaller.marshalInt(buffer, accountNum);       // 1. account num (4 bytes)
        Marshaller.marshalPassword(buffer, password);    // 2. password (8 bytes fixed)
        Marshaller.marshalString(buffer, name);          // 3. name (varchar)

        return extractBytes(buffer);
    }

    private static byte[] extractBytes(ByteBuffer buffer) {
        int length = buffer.position();
        byte[] result = new byte[length];
        buffer.rewind();
        buffer.get(result);
        return result;
    }
}
