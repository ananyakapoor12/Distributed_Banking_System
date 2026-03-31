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
    private double packetLossRate = 0.0;       // applied to both request and response
    private double responseLossRate = 0.0;     // applied only to responses (simulates reply loss)
    
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
                System.out.println("\n  ┌─ Attempt " + attempt + "/" + MAX_RETRIES + " ──────────────────────────────");
                // Simulate packet loss for testing (request loss)
                if (!shouldDropPacket()) {
                    System.out.println("  │ REQUEST  sent    [ID: " + requestIdStr.substring(0, 8) + "...]");
                    DatagramPacket sendPacket = new DatagramPacket(
                            requestData, requestData.length, serverAddress, serverPort);
                    socket.send(sendPacket);
                } else {
                    System.out.println("  │ REQUEST  DROPPED (never reached server)");
                }

                // Wait for response
                byte[] receiveBuffer = new byte[Protocol.MAX_BUF_SIZE];
                DatagramPacket receivePacket = new DatagramPacket(receiveBuffer, receiveBuffer.length);

                socket.receive(receivePacket);

                // Simulate packet loss for testing (response loss)
                if (shouldDropPacket() || Math.random() < responseLossRate) {
                    System.out.println("  │RESPONSE DROPPED (server executed, reply lost)");
                    System.out.println("  └────────────────────────────────────────────────────");
                    throw new SocketTimeoutException("Simulated response loss");
                }

                // Parse response
                response = MessageParser.parseResponse(receivePacket.getData(), receivePacket.getLength());
                System.out.println("  │RESPONSE RECEIVED  [Status: " + response.status + "]");
                System.out.println("  └────────────────────────────────────────────────────");

                // Cache response for at-most-once semantics
                if (semantics == Protocol.Semantics.AT_MOST_ONCE) {
                    responseCache.put(requestIdStr, response);
                }

                return response;

            } catch (SocketTimeoutException e) {
                System.out.println("  │RESPONSE TIMEOUT  (no reply within " + TIMEOUT_MS/1000 + "s)");
                System.out.println("  └────────────────────────────────────────────────────");

                if (attempt >= MAX_RETRIES) {
                    System.out.println("  All " + MAX_RETRIES + " attempts failed.");
                    throw new IOException("Request failed after " + MAX_RETRIES + " attempts: timeout");
                }

                System.out.println(" Retrying with same request ID (server may have already executed)...");
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
     * Simulate reply-only loss — request always reaches server, only response is dropped.
     * triggers ALO double-execution for non-idempotent operations.
     */
    public void setResponseLossRate(double rate) {
        if (rate < 0.0 || rate > 1.0) {
            throw new IllegalArgumentException("Response loss rate must be between 0.0 and 1.0");
        }
        this.responseLossRate = rate;
        System.out.println("Response-only loss simulation set to: " + (rate * 100) + "%");
    }
    
    /**
     * Determine if a packet should be dropped (ONLY for testing )
     */
    private boolean shouldDropPacket() {
        return Math.random() < packetLossRate;
    }
    
    /**
     * Get local port number
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
     * Get response cache size 
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
