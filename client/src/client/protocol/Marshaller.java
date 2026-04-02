package client.protocol;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.UUID;

/**
 * Marshaller handles conversion between Java types and byte arrays for network transmission.
 * All multi-byte values use network byte order (big-endian).
 */
public class Marshaller {
    
    /**
     * Marshal an integer (4 bytes, network byte order)
     */
    public static void marshalInt(ByteBuffer buffer, int value) {
        buffer.putInt(value);  // ByteBuffer defaults to big-endian
    }
    
    /**
     * Unmarshal an integer (4 bytes, network byte order)
     */
    public static int unmarshalInt(ByteBuffer buffer) {
        return buffer.getInt();
    }
    
    /**
     * Marshal a float (4 bytes, network byte order)
     * C++ server uses htonl/ntohl on float bits, so convert via int
     */
    public static void marshalFloat(ByteBuffer buffer, float value) {
        // Convert float to int bits, then write as network-order int
        int bits = Float.floatToRawIntBits(value);
        buffer.putInt(bits);  // big-endian order
    }
    
    /**
     * Unmarshal a float (4 bytes, network byte order)
     * C++ server uses htonl/ntohl on float bits, so convert via int
     */
    public static float unmarshalFloat(ByteBuffer buffer) {
        // Read as network-order integer, then convert to float bits
        int bits = buffer.getInt();
        return Float.intBitsToFloat(bits);
    }
    
    /**
     * Marshal a string with 2-byte length prefix
     * Format: [2 bytes length][N bytes UTF-8 data]
     */
    public static void marshalString(ByteBuffer buffer, String str) {
        byte[] bytes = str.getBytes(StandardCharsets.UTF_8);
        if (bytes.length > 65535) {
            throw new IllegalArgumentException("String too long: " + bytes.length + " bytes");
        }
        buffer.putShort((short) bytes.length);
        buffer.put(bytes);
    }
    
    /**
     * Unmarshal a string with 2-byte length prefix
     */
    public static String unmarshalString(ByteBuffer buffer) {
        int length = buffer.getShort() & 0xFFFF;  // unsigned short
        byte[] bytes = new byte[length];
        buffer.get(bytes);
        return new String(bytes, StandardCharsets.UTF_8);
    }
    
    /**
     * Marshal fixed-length password (PASSWORD_LEN bytes, no length prefix)
     * Pads with zeros if password is shorter than PASSWORD_LEN
     */
    public static void marshalPassword(ByteBuffer buffer, String password) {
        byte[] passwordBytes = password.getBytes(StandardCharsets.UTF_8);
        
        if (passwordBytes.length > Protocol.PASSWORD_LEN) {
            throw new IllegalArgumentException("Password too long: " + passwordBytes.length + " bytes");
        }
        
        // Write password bytes
        buffer.put(passwordBytes);
        
        // Pad with zeros to reach PASSWORD_LEN
        int padding = Protocol.PASSWORD_LEN - passwordBytes.length;
        for (int i = 0; i < padding; i++) {
            buffer.put((byte) 0);
        }
    }
    
    /**
     * Unmarshal fixed-length password (PASSWORD_LEN bytes)
     */
    public static String unmarshalPassword(ByteBuffer buffer) {
        byte[] passwordBytes = new byte[Protocol.PASSWORD_LEN];
        buffer.get(passwordBytes);
        
        // Find the first null byte to trim padding
        int actualLength = Protocol.PASSWORD_LEN;
        for (int i = 0; i < Protocol.PASSWORD_LEN; i++) {
            if (passwordBytes[i] == 0) {
                actualLength = i;
                break;
            }
        }
        
        return new String(passwordBytes, 0, actualLength, StandardCharsets.UTF_8);
    }
    
    /**
     * Generate a 16-byte UUID for request identification - size prefixed in Protocol.REQUEST_ID_LEN
     */
    public static byte[] generateRequestId() {
        UUID uuid = UUID.randomUUID();
        ByteBuffer buffer = ByteBuffer.wrap(new byte[Protocol.REQUEST_ID_LEN]);
        buffer.order(ByteOrder.BIG_ENDIAN);
        buffer.putLong(uuid.getMostSignificantBits());
        buffer.putLong(uuid.getLeastSignificantBits());
        return buffer.array();
    }
    
    /**
     * Convert request ID bytes to UUID string for display
     */
    public static String requestIdToString(byte[] requestId) {
        if (requestId.length != Protocol.REQUEST_ID_LEN) {
            return Arrays.toString(requestId);
        }
        ByteBuffer buffer = ByteBuffer.wrap(requestId);
        buffer.order(ByteOrder.BIG_ENDIAN);
        long mostSigBits = buffer.getLong();
        long leastSigBits = buffer.getLong();
        return new UUID(mostSigBits, leastSigBits).toString();
    }
    
    /**
     * Marshal a byte value
     */
    public static void marshalByte(ByteBuffer buffer, byte value) {
        buffer.put(value);
    }
    
    /**
     * Unmarshal a byte value
     */
    public static byte unmarshalByte(ByteBuffer buffer) {
        return buffer.get();
    }
    
    /**
     * Create a ByteBuffer configured for network byte order (big-endian)
     */
    public static ByteBuffer createBuffer(int capacity) {
        ByteBuffer buffer = ByteBuffer.allocate(capacity);
        buffer.order(ByteOrder.BIG_ENDIAN);
        return buffer;
    }
    
    /**
     * Wrap existing byte array in a ByteBuffer configured for network byte order
     */
    public static ByteBuffer wrapBuffer(byte[] data) {
        ByteBuffer buffer = ByteBuffer.wrap(data);
        buffer.order(ByteOrder.BIG_ENDIAN);
        return buffer;
    }
}