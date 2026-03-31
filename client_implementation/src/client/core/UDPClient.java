package client.core;

import client.protocol.Marshaller;
import client.protocol.MessageParser;
import client.protocol.Protocol;

import java.io.IOException;
import java.net.*;
import java.util.HashMap;
import java.util.Map;

/**
 * UDPClient handles low-level UDP communication with the server,
 * including timeout/retry logic and request history for at-most-once semantics.
 */
public class UDPClient {
    
    private final String serverHost;
    private final int serverPort;
    private final DatagramSocket socket;
    private final InetAddress serverAddress;
    private final Protocol.Semantics semantics;
    
    // Configuration
    private static final int TIMEOUT_MS = 3000;          // 3 seconds
    private static final int MAX_RETRIES = 3;
    
    // Request history for at-most-once semantics
    private final Map<String, MessageParser.Response> responseCache;
    
    // Simulated packet loss (for testing)
    private double packetLossRate = 0.0;  // 0.0 = no loss, 0.3 = 30% loss
    
    public UDPClient(String serverHost, int serverPort, Protocol.Semantics semantics) 
            throws SocketException, UnknownHostException {
        this.serverHost = serverHost;
        this.serverPort = serverPort;
        this.semantics = semantics;
        this.socket = new DatagramSocket();
        this.socket.setSoTimeout(TIMEOUT_MS);
        this.serverAddress = InetAddress.getByName(serverHost);
        this.responseCache = new HashMap<>();
        
        System.out.println("UDP Client initialized:");
        System.out.println("  Server: " + serverHost + ":" + serverPort);
        System.out.println("  Semantics: " + semantics);
        System.out.println("  Local port: " + socket.getLocalPort());
    }
    
    /**
     * Send a request and wait for response with timeout/retry logic
     */
    public MessageParser.Response sendRequest(byte[] requestData, byte[] requestId) throws IOException {
        String requestIdStr = Marshaller.requestIdToString(requestId);
        
        // For at-most-once, check cache first
        if (semantics == Protocol.Semantics.AT_MOST_ONCE && responseCache.containsKey(requestIdStr)) {
            System.out.println("  [CACHE HIT] Returning cached response for request " + requestIdStr);
            return responseCache.get(requestIdStr);
        }
        
        MessageParser.Response response = null;
        int attempt = 0;
        
        while (attempt < MAX_RETRIES) {
            attempt++;
            
            try {
                // Simulate packet loss for testing (request loss)
                if (!shouldDropPacket()) {
                    System.out.println("  [SEND] Attempt " + attempt + "/" + MAX_RETRIES + 
                                     " - Request ID: " + requestIdStr);
                    DatagramPacket sendPacket = new DatagramPacket(
                            requestData, requestData.length, serverAddress, serverPort);
                    socket.send(sendPacket);
                } else {
                    System.out.println("  [SIMULATED LOSS] Request packet dropped (attempt " + attempt + ")");
                }
                
                // Wait for response
                byte[] receiveBuffer = new byte[Protocol.MAX_BUF_SIZE];
                DatagramPacket receivePacket = new DatagramPacket(receiveBuffer, receiveBuffer.length);
                
                socket.receive(receivePacket);
                
                // Simulate packet loss for testing (response loss)
                if (shouldDropPacket()) {
                    System.out.println("  [SIMULATED LOSS] Response packet dropped");
                    throw new SocketTimeoutException("Simulated response loss");
                }
                
                // Parse response
                response = MessageParser.parseResponse(receivePacket.getData(), receivePacket.getLength());
                System.out.println("  [RECV] Response received - Status: " + response.status);
                
                // Cache response for at-most-once semantics
                if (semantics == Protocol.Semantics.AT_MOST_ONCE) {
                    responseCache.put(requestIdStr, response);
                }
                
                return response;
                
            } catch (SocketTimeoutException e) {
                System.out.println("  [TIMEOUT] No response received (attempt " + attempt + ")");
                
                if (attempt >= MAX_RETRIES) {
                    throw new IOException("Request failed after " + MAX_RETRIES + " attempts: timeout");
                }
                
                // For at-most-once, we retry with the SAME request ID
                // For at-least-once, we could generate a new ID, but spec says requests are well-separated
                System.out.println("  [RETRY] Retrying with same request ID...");
            }
        }
        
        throw new IOException("Request failed after " + MAX_RETRIES + " attempts");
    }
    
    /**
     * Receive a callback message (for monitoring)
     * This is a blocking call that waits for a callback
     */
    public MessageParser.Callback receiveCallback(int timeoutMs) throws IOException {
        socket.setSoTimeout(timeoutMs);
        
        try {
            byte[] receiveBuffer = new byte[Protocol.MAX_BUF_SIZE];
            DatagramPacket receivePacket = new DatagramPacket(receiveBuffer, receiveBuffer.length);
            
            socket.receive(receivePacket);
            
            // Parse callback
            MessageParser.Callback callback = MessageParser.parseCallback(
                    receivePacket.getData(), receivePacket.getLength());
            
            return callback;
            
        } finally {
            // Restore default timeout
            socket.setSoTimeout(TIMEOUT_MS);
        }
    }
    
    /**
     * Enable/disable simulated packet loss for testing
     */
    public void setPacketLossRate(double rate) {
        if (rate < 0.0 || rate > 1.0) {
            throw new IllegalArgumentException("Packet loss rate must be between 0.0 and 1.0");
        }
        this.packetLossRate = rate;
        System.out.println("Packet loss simulation set to: " + (rate * 100) + "%");
    }
    
    /**
     * Determine if a packet should be dropped (for testing)
     */
    private boolean shouldDropPacket() {
        return Math.random() < packetLossRate;
    }
    
    /**
     * Get local port (useful for debugging)
     */
    public int getLocalPort() {
        return socket.getLocalPort();
    }
    
    /**
     * Close the socket
     */
    public void close() {
        if (socket != null && !socket.isClosed()) {
            socket.close();
            System.out.println("UDP Client closed");
        }
    }
    
    /**
     * Get response cache size (for debugging)
     */
    public int getCacheSize() {
        return responseCache.size();
    }
    
    /**
     * Clear response cache
     */
    public void clearCache() {
        responseCache.clear();
        System.out.println("Response cache cleared");
    }
}
