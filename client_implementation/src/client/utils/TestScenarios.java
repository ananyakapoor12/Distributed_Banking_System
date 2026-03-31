package client.utils;

import client.core.BankingClient;
import client.protocol.Protocol;

/**
 * TestScenarios demonstrates the differences between at-least-once and at-most-once
 * invocation semantics, particularly for non-idempotent operations.
 */
public class TestScenarios {
    
    private final BankingClient client;
    
    public TestScenarios(BankingClient client) {
        this.client = client;
    }
    
    /**
     * Scenario 1: Test TRANSFER (non-idempotent) with packet loss
     * 
     * Expected behavior:
     * - AT-LEAST-ONCE: May execute transfer multiple times if reply is lost
     * - AT-MOST-ONCE: Executes transfer exactly once, even with retries
     */
    public void testTransferWithPacketLoss() {
        System.out.println("\n═══════════════════════════════════════════════════════════");
        System.out.println("  TEST SCENARIO 1: Non-Idempotent TRANSFER with Packet Loss");
        System.out.println("═══════════════════════════════════════════════════════════\n");
        
        try {
            // Create two test accounts
            System.out.println("Setting up test accounts...");
            int account1 = client.openAccount("Alice", "pass123", Protocol.Currency.USD, 1000.0f);
            int account2 = client.openAccount("Bob", "pass456", Protocol.Currency.USD, 500.0f);
            
            System.out.println("\nInitial state:");
            System.out.println("  Alice (Account #" + account1 + "): $1000.00");
            System.out.println("  Bob   (Account #" + account2 + "): $500.00");
            
            // Simulate reply loss: request reaches server (executes), but reply is dropped.
            // Client retries → server executes AGAIN (ALO problem for non-idempotent ops).
            System.out.println("\n⚠ Enabling 100% response loss (request reaches server, reply dropped)...");
            client.setResponseLossRate(1.0);

            // Attempt transfer (will retry due to reply loss, server executes multiple times under ALO)
            System.out.println("\nAttempting to transfer $100 from Alice to Bob...");
            System.out.println("(Expect timeouts and retries — server executes each retry)\n");

            try {
                float aliceNewBalance = client.transfer(account1, "Alice", "pass123",
                                                       account2, Protocol.Currency.USD, 100.0f, "Bob");
            } catch (Exception ignored) {
                // Expected: all 3 replies dropped → timeout after max retries
            }

            // Disable loss before checking balances
            client.setResponseLossRate(0.0);
            
            // Check final balances
            System.out.println("\n\nChecking final balances...");
            float aliceBalance = client.checkBalance(account1, "Alice", "pass123");
            float bobBalance = client.checkBalance(account2, "Bob", "pass456");
            
            System.out.println("\n┌─────────────────────────────────────────────────────────┐");
            System.out.println("│                   FINAL RESULTS                         │");
            System.out.println("├─────────────────────────────────────────────────────────┤");
            System.out.printf("│  Alice's Balance: $%.2f                              │%n", aliceBalance);
            System.out.printf("│  Bob's Balance:   $%.2f                              │%n", bobBalance);
            System.out.println("└─────────────────────────────────────────────────────────┘");
            
            // Analyze results
            System.out.println("\n ANALYSIS:");
            if (aliceBalance == 900.0f && bobBalance == 600.0f) {
                System.out.println("AT-MOST-ONCE semantics working correctly!");
                System.out.println("  Transfer executed exactly once despite retries.");
            } else if (aliceBalance < 900.0f) {
                System.out.println("AT-LEAST-ONCE semantics detected!");
                System.out.println("  Transfer may have executed multiple times.");
                System.out.println("  This demonstrates why non-idempotent operations");
                System.out.println("  are problematic with at-least-once semantics!");
            }
            
        } catch (Exception e) {
            System.out.println("Test failed: " + e.getMessage());
            e.printStackTrace();
        }
    }
    
    /**
     * Scenario 2: Test CHECK_BALANCE (idempotent) with packet loss
     * 
     * Expected behavior:
     * - Both semantics should work correctly
     * - Safe to execute multiple times
     */
    public void testCheckBalanceWithPacketLoss() {
        System.out.println("\n═══════════════════════════════════════════════════════════");
        System.out.println("  TEST SCENARIO 2: Idempotent CHECK_BALANCE with Packet Loss");
        System.out.println("═══════════════════════════════════════════════════════════\n");
        
        try {
            // Create test account
            System.out.println("Setting up test account...");
            int account = client.openAccount("Charlie", "pass789", Protocol.Currency.EUR, 2500.0f);
            
            System.out.println("\nInitial balance: €2500.00");
            
            // Enable packet loss
            System.out.println("\n Enabling 80% packet loss simulation...");
            client.setPacketLossRate(0.4);
            
            // Check balance multiple times (may retry due to packet loss)
            System.out.println("\nChecking balance 3 times with packet loss...");
            System.out.println("(Watch for retry attempts)\n");
            
            float balance1 = client.checkBalance(account, "Charlie", "pass789");
            float balance2 = client.checkBalance(account, "Charlie", "pass789");
            float balance3 = client.checkBalance(account, "Charlie", "pass789");
            
            // Disable packet loss
            client.setPacketLossRate(0.0);
            
            System.out.println("\n┌─────────────────────────────────────────────────────────┐");
            System.out.println("│                   RESULTS                               │");
            System.out.println("├─────────────────────────────────────────────────────────┤");
            System.out.printf("│  Check 1: €%.2f                                      │%n", balance1);
            System.out.printf("│  Check 2: €%.2f                                      │%n", balance2);
            System.out.printf("│  Check 3: €%.2f                                      │%n", balance3);
            System.out.println("└─────────────────────────────────────────────────────────┘");
            
            // Analyze results
            System.out.println("\n ANALYSIS:");
            if (balance1 == balance2 && balance2 == balance3 && balance1 == 2500.0f) {
                System.out.println("✓ Idempotent operation works correctly!");
                System.out.println("  Balance check is safe to repeat - same result every time.");
                System.out.println("  This works correctly with both invocation semantics.");
            }
            
        } catch (Exception e) {
            System.out.println("Test failed: " + e.getMessage());
            e.printStackTrace();
        }
    }
    
    /**
     * Scenario 3: Test DEPOSIT (non-idempotent) with simulated timeout
     */
    public void testDepositWithTimeout() {
        System.out.println("\n═══════════════════════════════════════════════════════════");
        System.out.println("  TEST SCENARIO 3: Non-Idempotent DEPOSIT with Packet Loss");
        System.out.println("═══════════════════════════════════════════════════════════\n");
        
        try {
            // Create test account
            System.out.println("Setting up test account...");
            int account = client.openAccount("David", "pass000", Protocol.Currency.SGD, 100.0f);
            
            System.out.println("\nInitial balance: SGD $100.00");
            
            // Simulate reply loss: request reaches server (executes), reply is dropped.
            // Under ALO, each retry re-executes the deposit.
            System.out.println("\n⚠ Enabling 100% response loss (request reaches server, reply dropped)...");
            client.setResponseLossRate(1.0);

            // Attempt ONE deposit — all replies dropped, client retries MAX_RETRIES times.
            // Under ALO: server deposits $50 up to 3 times (once per retry attempt).
            System.out.println("\nDepositing SGD $50.00 once (with reply loss)...");
            System.out.println("(Expect retries — each retry re-executes deposit under ALO)\n");

            try {
                client.deposit(account, "David", "pass000", Protocol.Currency.SGD, 50.0f);
            } catch (Exception ignored) {
                // Expected: all replies dropped → timeout
            }

            // Disable loss before checking final balance
            client.setResponseLossRate(0.0);
            
            // Check final balance
            float finalBalance = client.checkBalance(account, "David", "pass000");
            
            System.out.println("\n┌─────────────────────────────────────────────────────────┐");
            System.out.println("│                   FINAL RESULT                          │");
            System.out.println("├─────────────────────────────────────────────────────────┤");
            System.out.printf("│  Initial: SGD $100.00  +  1 deposit of SGD $50.00       │%n");
            System.out.printf("│  Expected (at-most-once): SGD $150.00                   │%n");
            System.out.printf("│  Actual Final Balance:    SGD $%.2f                 │%n", finalBalance);
            System.out.println("└─────────────────────────────────────────────────────────┘");

            // Analyze results
            System.out.println("\nANALYSIS:");
            if (finalBalance == 150.0f) {
                System.out.println("AT-MOST-ONCE: deposit executed exactly once (cached reply returned on retries).");
            } else if (finalBalance > 150.0f) {
                int times = Math.round((finalBalance - 100.0f) / 50.0f);
                System.out.println("AT-LEAST-ONCE: deposit executed " + times + " times instead of 1!");
                System.out.printf("  Extra deposited: SGD $%.2f — each retry re-executed the operation.%n",
                        finalBalance - 150.0f);
            }
            
        } catch (Exception e) {
            System.out.println("Test failed: " + e.getMessage());
            e.printStackTrace();
        }
    }
    
    /**
     * Run all test scenarios
     */
    public void runAllTests() {
        System.out.println("\n╔═══════════════════════════════════════════════════════════╗");
        System.out.println("║         AUTOMATED TEST SUITE - INVOCATION SEMANTICS       ║");
        System.out.println("╚═══════════════════════════════════════════════════════════╝");
        
        testCheckBalanceWithPacketLoss();
        
        System.out.println("\n\nPress Enter to continue to next test...");
        try { System.in.read(); } catch (Exception e) {}
        
        testDepositWithTimeout();
        
        System.out.println("\n\nPress Enter to continue to next test...");
        try { System.in.read(); } catch (Exception e) {}
        
        testTransferWithPacketLoss();
        
        System.out.println("\n\n═══════════════════════════════════════════════════════════");
        System.out.println("                   ALL TESTS COMPLETED                      ");
        System.out.println("═══════════════════════════════════════════════════════════\n");
    }
    
    /**
     * Main method for running tests independently
     */
    public static void main(String[] args) {
        String serverHost = args.length > 0 ? args[0] : "localhost";
        int serverPort = args.length > 1 ? Integer.parseInt(args[1]) : 2222;
        Protocol.Semantics semantics = Protocol.Semantics.AT_MOST_ONCE;
        
        if (args.length > 2) {
            semantics = args[2].equalsIgnoreCase("at-least-once") ? 
                       Protocol.Semantics.AT_LEAST_ONCE : Protocol.Semantics.AT_MOST_ONCE;
        }
        
        System.out.println("Test Configuration:");
        System.out.println("  Server: " + serverHost + ":" + serverPort);
        System.out.println("  Semantics: " + semantics);
        
        try {
            BankingClient client = new BankingClient(serverHost, serverPort, semantics);
            TestScenarios tests = new TestScenarios(client);
            tests.runAllTests();
            client.close();
        } catch (Exception e) {
            System.err.println("Failed to initialize client: " + e.getMessage());
            e.printStackTrace();
        }
    }
}
