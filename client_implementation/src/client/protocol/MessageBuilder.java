package client.protocol;

import java.nio.ByteBuffer;

/**
 * MessageBuilder constructs request messages according to the wire protocol.
 * Order fo fields per opcode must be identical to what server side expects.
 *
 */
public class MessageBuilder {

    /**
     * Build OPEN_ACCOUNT request
     * password(fixed), currency, balance, name(varchar)
     */
    public static byte[] buildOpenAccountRequest(byte[] requestId, String name,
                                                 String password, Protocol.Currency currency,
                                                 float initialBalance) {
        ByteBuffer buffer = Marshaller.createBuffer(Protocol.MAX_BUF_SIZE);

        buffer.put(Protocol.MessageType.REQUEST.getValue());
        buffer.put(requestId);
        buffer.put(Protocol.Opcode.OPEN_ACCOUNT.getValue());

        Marshaller.marshalPassword(buffer, password);    
        buffer.put(currency.getValue());                 
        Marshaller.marshalFloat(buffer, initialBalance); 
        Marshaller.marshalString(buffer, name);          

        return extractBytes(buffer);
    }

    /**
     * Build CLOSE_ACCOUNT request
     * account_num, password(fixed), name(varchar)
     */
    public static byte[] buildCloseAccountRequest(byte[] requestId, int accountNum,
                                                   String name, String password) {
        ByteBuffer buffer = Marshaller.createBuffer(Protocol.MAX_BUF_SIZE);

        buffer.put(Protocol.MessageType.REQUEST.getValue());
        buffer.put(requestId);
        buffer.put(Protocol.Opcode.CLOSE_ACCOUNT.getValue());

        Marshaller.marshalInt(buffer, accountNum);       
        Marshaller.marshalPassword(buffer, password);    
        Marshaller.marshalString(buffer, name);         

        return extractBytes(buffer);
    }

    /**
     * Build DEPOSIT request
     * account_num, password(fixed), currency, amount, name(varchar)
     */
    public static byte[] buildDepositRequest(byte[] requestId, int accountNum,
                                              String name, String password,
                                              Protocol.Currency currency, float amount) {
        ByteBuffer buffer = Marshaller.createBuffer(Protocol.MAX_BUF_SIZE);

        buffer.put(Protocol.MessageType.REQUEST.getValue());
        buffer.put(requestId);
        buffer.put(Protocol.Opcode.DEPOSIT.getValue());

        Marshaller.marshalInt(buffer, accountNum);       
        Marshaller.marshalPassword(buffer, password);    
        buffer.put(currency.getValue());                
        Marshaller.marshalFloat(buffer, amount);   
        Marshaller.marshalString(buffer, name);         

        return extractBytes(buffer);
    }

    /**
     * Build WITHDRAW request: account_num, password(fixed), currency, amount, name(varchar)
     */
    public static byte[] buildWithdrawRequest(byte[] requestId, int accountNum,
                                               String name, String password,
                                               Protocol.Currency currency, float amount) {
        ByteBuffer buffer = Marshaller.createBuffer(Protocol.MAX_BUF_SIZE);

        buffer.put(Protocol.MessageType.REQUEST.getValue());
        buffer.put(requestId);
        buffer.put(Protocol.Opcode.WITHDRAW.getValue());

        Marshaller.marshalInt(buffer, accountNum);      
        Marshaller.marshalPassword(buffer, password);    
        buffer.put(currency.getValue());                
        Marshaller.marshalFloat(buffer, amount);   
        Marshaller.marshalString(buffer, name);    

        return extractBytes(buffer);
    }

    /**
     * Build MONITOR request: duration(uint32)
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

        Marshaller.marshalInt(buffer, senderAccountNum);     
        Marshaller.marshalPassword(buffer, senderPassword);  
        Marshaller.marshalInt(buffer, receiverAccountNum);   
        Marshaller.marshalFloat(buffer, amount);             
        buffer.put(currency.getValue());                     
        Marshaller.marshalString(buffer, senderName);        
        Marshaller.marshalString(buffer, receiverName);      

        return extractBytes(buffer);
    }

    /**
     * Build CHECK_BALANCE request (idempotent extra service)
     * account_num, password(fixed), name(varchar)
     */
    public static byte[] buildCheckBalanceRequest(byte[] requestId, int accountNum,
                                                   String name, String password) {
        ByteBuffer buffer = Marshaller.createBuffer(Protocol.MAX_BUF_SIZE);

        buffer.put(Protocol.MessageType.REQUEST.getValue());
        buffer.put(requestId);
        buffer.put(Protocol.Opcode.CHECK_BALANCE.getValue());

        Marshaller.marshalInt(buffer, accountNum);       
        Marshaller.marshalPassword(buffer, password);   
        Marshaller.marshalString(buffer, name);         

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
