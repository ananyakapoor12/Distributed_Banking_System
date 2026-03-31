package client.core;

import client.protocol.*;

import java.io.IOException;

/**
 * BankingClient provides high-level banking operations that use the UDPClient
 * to communicate with the server.
 */
public class BankingClient {
    
    private final UDPClient udpClient;
    
    public BankingClient(String serverHost, int serverPort, Protocol.Semantics semantics) 
            throws Exception {
        this.udpClient = new UDPClient(serverHost, serverPort, semantics);
    }
    
    /**
     * Open a new bank account
     * 
     * @param name Account holder's name
     * @param password Account password (max 8 bytes)
     * @param currency Currency type
     * @param initialBalance Initial deposit amount
     * @return Account number if successful
     */
    public int openAccount(String name, String password, Protocol.Currency currency, 
                           float initialBalance) throws IOException {
        System.out.println("\n[OPEN ACCOUNT] Initiating request...");
        System.out.println("  Name: " + name);
        System.out.println("  Currency: " + currency);
        System.out.println("  Initial Balance: " + initialBalance);
        
        byte[] requestId = Marshaller.generateRequestId();
        byte[] requestData = MessageBuilder.buildOpenAccountRequest(
                requestId, name, password, currency, initialBalance);
        
        MessageParser.Response response = udpClient.sendRequest(requestData, requestId);
        
        if (response.status.isSuccess()) {
            MessageParser.OpenAccountResult result = (MessageParser.OpenAccountResult) response.data;
            System.out.println("✓ SUCCESS: " + result);
            return result.accountNum;
        } else {
            System.out.println("✗ FAILED: " + response.errorMessage);
            throw new IOException("Open account failed: " + response.errorMessage);
        }
    }
    
    /**
     * Close an existing account
     */
    public void closeAccount(int accountNum, String name, String password) throws IOException {
        System.out.println("\n[CLOSE ACCOUNT] Initiating request...");
        System.out.println("  Account Number: " + accountNum);
        System.out.println("  Name: " + name);
        
        byte[] requestId = Marshaller.generateRequestId();
        byte[] requestData = MessageBuilder.buildCloseAccountRequest(
                requestId, accountNum, name, password);
        
        MessageParser.Response response = udpClient.sendRequest(requestData, requestId);
        
        if (response.status.isSuccess()) {
            System.out.println("✓ SUCCESS: Account closed");
        } else {
            System.out.println("✗ FAILED: " + response.errorMessage);
            throw new IOException("Close account failed: " + response.errorMessage);
        }
    }
    
    /**
     * Deposit money into an account
     */
    public float deposit(int accountNum, String name, String password, 
                        Protocol.Currency currency, float amount) throws IOException {
        System.out.println("\n[DEPOSIT] Initiating request...");
        System.out.println("  Account Number: " + accountNum);
        System.out.println("  Amount: " + amount + " " + currency);
        
        byte[] requestId = Marshaller.generateRequestId();
        byte[] requestData = MessageBuilder.buildDepositRequest(
                requestId, accountNum, name, password, currency, amount);
        
        MessageParser.Response response = udpClient.sendRequest(requestData, requestId);
        
        if (response.status.isSuccess()) {
            MessageParser.BalanceResult result = (MessageParser.BalanceResult) response.data;
            System.out.println("✓ SUCCESS: " + result);
            return result.balance;
        } else {
            System.out.println("✗ FAILED: " + response.errorMessage);
            throw new IOException("Deposit failed: " + response.errorMessage);
        }
    }
    
    /**
     * Withdraw money from an account
     */
    public float withdraw(int accountNum, String name, String password, 
                         Protocol.Currency currency, float amount) throws IOException {
        System.out.println("\n[WITHDRAW] Initiating request...");
        System.out.println("  Account Number: " + accountNum);
        System.out.println("  Amount: " + amount + " " + currency);
        
        byte[] requestId = Marshaller.generateRequestId();
        byte[] requestData = MessageBuilder.buildWithdrawRequest(
                requestId, accountNum, name, password, currency, amount);
        
        MessageParser.Response response = udpClient.sendRequest(requestData, requestId);
        
        if (response.status.isSuccess()) {
            MessageParser.BalanceResult result = (MessageParser.BalanceResult) response.data;
            System.out.println("✓ SUCCESS: " + result);
            return result.balance;
        } else {
            System.out.println("✗ FAILED: " + response.errorMessage);
            throw new IOException("Withdraw failed: " + response.errorMessage);
        }
    }
    
    /**
     * Transfer money to another account (non-idempotent operation)
     */
    public float transfer(int senderAccountNum, String senderName, String senderPassword,
                         int receiverAccountNum, Protocol.Currency currency,
                         float amount, String receiverName) throws IOException {
        System.out.println("\n[TRANSFER] Initiating request...");
        System.out.println("  From Account: " + senderAccountNum);
        System.out.println("  To Account: " + receiverAccountNum);
        System.out.println("  Amount: " + amount + " " + currency);

        byte[] requestId = Marshaller.generateRequestId();
        byte[] requestData = MessageBuilder.buildTransferRequest(
                requestId, senderAccountNum, senderName, senderPassword,
                receiverAccountNum, currency, amount, receiverName);
        
        MessageParser.Response response = udpClient.sendRequest(requestData, requestId);
        
        if (response.status.isSuccess()) {
            MessageParser.TransferResult result = (MessageParser.TransferResult) response.data;
            System.out.println("✓ SUCCESS: " + result);
            return result.senderNewBalance;
        } else {
            System.out.println("✗ FAILED: " + response.errorMessage);
            throw new IOException("Transfer failed: " + response.errorMessage);
        }
    }
    
    /**
     * Check account balance (idempotent operation)
     */
    public float checkBalance(int accountNum, String name, String password) throws IOException {
        System.out.println("\n[CHECK BALANCE] Initiating request...");
        System.out.println("  Account Number: " + accountNum);
        
        byte[] requestId = Marshaller.generateRequestId();
        byte[] requestData = MessageBuilder.buildCheckBalanceRequest(
                requestId, accountNum, name, password);
        
        MessageParser.Response response = udpClient.sendRequest(requestData, requestId);
        
        if (response.status.isSuccess()) {
            MessageParser.BalanceResult result = (MessageParser.BalanceResult) response.data;
            System.out.println("✓ SUCCESS: " + result);
            return result.balance;
        } else {
            System.out.println("✗ FAILED: " + response.errorMessage);
            throw new IOException("Check balance failed: " + response.errorMessage);
        }
    }
    
    /**
     * Register for monitoring account updates
     * 
     * @param durationSeconds How long to monitor (in seconds)
     * @param callbackHandler Handler for received callbacks
     */
    public void monitor(int durationSeconds, CallbackHandler callbackHandler) throws IOException {
        System.out.println("\n[MONITOR] Registering for updates...");
        System.out.println("  Duration: " + durationSeconds + " seconds");
        
        byte[] requestId = Marshaller.generateRequestId();
        byte[] requestData = MessageBuilder.buildMonitorRequest(requestId, durationSeconds);
        
        MessageParser.Response response = udpClient.sendRequest(requestData, requestId);
        
        if (response.status.isSuccess()) {
            System.out.println("✓ Monitoring registered. Waiting for callbacks...");
            System.out.println("  (You will be blocked from making new requests during this time)");
            
            // Calculate end time
            long endTime = System.currentTimeMillis() + (durationSeconds * 1000L);
            
            // Listen for callbacks
            while (System.currentTimeMillis() < endTime) {
                try {
                    int remainingTime = (int) (endTime - System.currentTimeMillis());
                    if (remainingTime <= 0) break;
                    
                    // Receive callback with remaining time as timeout
                    MessageParser.Callback callback = udpClient.receiveCallback(remainingTime);
                    
                    System.out.println("\n>>> CALLBACK RECEIVED <<<");
                    System.out.println(callback);
                    
                    // Notify handler
                    if (callbackHandler != null) {
                        callbackHandler.onCallback(callback);
                    }
                    
                } catch (IOException e) {
                    // Timeout or error - check if monitoring period is over
                    if (System.currentTimeMillis() >= endTime) {
                        break;
                    }
                }
            }
            
            System.out.println("\n[MONITOR] Monitoring period ended");
            
        } else {
            System.out.println("✗ FAILED: " + response.errorMessage);
            throw new IOException("Monitor registration failed: " + response.errorMessage);
        }
    }
    
    /**
     * Enable simulated packet loss for testing
     */
    public void setPacketLossRate(double rate) {
        udpClient.setPacketLossRate(rate);
    }
    
    /**
     * Get UDP client for advanced operations
     */
    public UDPClient getUdpClient() {
        return udpClient;
    }
    
    /**
     * Close the client
     */
    public void close() {
        udpClient.close();
    }
    
    /**
     * Interface for handling callbacks
     */
    public interface CallbackHandler {
        void onCallback(MessageParser.Callback callback);
    }
}
