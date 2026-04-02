package client.ui;

import client.core.BankingClient;
import client.protocol.Protocol;

import java.util.Scanner;

/**
 * ConsoleUI provides a text-based interactive interface for the banking client.
 */
public class ConsoleUI {
    
    private final BankingClient client;
    private final Scanner scanner;
    
    public ConsoleUI(BankingClient client) {
        this.client = client;
        this.scanner = new Scanner(System.in);
    }
    
    /**
     * Start the interactive menu
     */
    public void start() {
        printWelcome();
        
        boolean running = true;
        while (running) {
            printMenu();
            
            try {
                int choice = getIntInput("Enter your choice: ");
                
                switch (choice) {
                    case 1:
                        handleOpenAccount();
                        break;
                    case 2:
                        handleCloseAccount();
                        break;
                    case 3:
                        handleDeposit();
                        break;
                    case 4:
                        handleWithdraw();
                        break;
                    case 5:
                        handleTransfer();
                        break;
                    case 6:
                        handleCheckBalance();
                        break;
                    case 7:
                        handleMonitor();
                        break;
                    case 8:
                        handleSimulatePacketLoss();
                        break;
                    case 9:
                        System.out.println("\nThank you for using the Distributed Banking System!");
                        running = false;
                        break;
                    default:
                        System.out.println("Invalid choice. Please try again.");
                }
                
            } catch (Exception e) {
                System.out.println("\nError: " + e.getMessage());
                if (e.getCause() != null) {
                    System.out.println("Cause: " + e.getCause().getMessage());
                }
            }
            
            if (running) {
                System.out.println("\nPress Enter to continue...");
                scanner.nextLine();
            }
        }
        
        client.close();
    }
    
    private void printWelcome() {
        System.out.println("═══════════════════════════════════════════════════════════");
        System.out.println("     DISTRIBUTED BANKING SYSTEM - CLIENT APPLICATION      ");
        System.out.println("═══════════════════════════════════════════════════════════");
        System.out.println();
    }
    
    private void printMenu() {
        System.out.println("\n┌─────────────────────────────────────────────────────────┐");
        System.out.println("│                      MAIN MENU                          │");
        System.out.println("├─────────────────────────────────────────────────────────┤");
        System.out.println("│  1. Open New Account                                    │");
        System.out.println("│  2. Close Account                                       │");
        System.out.println("│  3. Deposit Money                                       │");
        System.out.println("│  4. Withdraw Money                                      │");
        System.out.println("│  5. Transfer Money (Non-Idempotent)                     │");
        System.out.println("│  6. Check Balance (Idempotent)                          │");
        System.out.println("│  7. Monitor Account Updates                             │");
        System.out.println("│  8. Simulate Packet Loss (Testing)                      │");
        System.out.println("│  9. Exit                                                │");
        System.out.println("└─────────────────────────────────────────────────────────┘");
    }
    
    private void handleOpenAccount() throws Exception {
        System.out.println("\n─── OPEN NEW ACCOUNT ───");
        
        String name = getStringInput("Enter your name: ");
        String password = getPasswordInput("Enter password (max 8 characters): ");
        Protocol.Currency currency = getCurrencyInput("Select currency");
        float initialBalance = getFloatInput("Enter initial deposit amount: ");
        
        if (initialBalance < 0) {
            System.out.println("Initial balance cannot be negative!");
            return;
        }
        
        int accountNum = client.openAccount(name, password, currency, initialBalance);
        
        System.out.println("\n┌─────────────────────────────────────────────────────────┐");
        System.out.println("│           ACCOUNT CREATED SUCCESSFULLY!                 │");
        System.out.println("├─────────────────────────────────────────────────────────┤");
        System.out.printf("│  Account Number: %-38d │%n", accountNum);
        System.out.printf("│  Account Holder: %-38s │%n", name);
        System.out.printf("│  Currency:       %-38s │%n", currency);
        System.out.printf("│  Initial Balance: %-37.2f │%n", initialBalance);
        System.out.println("└─────────────────────────────────────────────────────────┘");
        System.out.println("\n⚠ IMPORTANT: Please save your account number!");
    }
    
    private void handleCloseAccount() throws Exception {
        System.out.println("\n─── CLOSE ACCOUNT ───");
        
        int accountNum = getIntInput("Enter account number: ");
        String name = getStringInput("Enter your name: ");
        String password = getPasswordInput("Enter password: ");
        
        String confirm = getStringInput("Are you sure you want to close this account? (yes/no): ");
        if (!confirm.equalsIgnoreCase("yes")) {
            System.out.println("Account closure cancelled.");
            return;
        }
        
        client.closeAccount(accountNum, name, password);
        
        System.out.println("\n✓ Account #" + accountNum + " has been closed successfully.");
    }
    
    private void handleDeposit() throws Exception {
        System.out.println("\n─── DEPOSIT MONEY ───");
        
        int accountNum = getIntInput("Enter account number: ");
        String name = getStringInput("Enter your name: ");
        String password = getPasswordInput("Enter password: ");
        Protocol.Currency currency = getCurrencyInput("Select currency");
        float amount = getFloatInput("Enter deposit amount: ");
        
        if (amount <= 0) {
            System.out.println("Deposit amount must be positive!");
            return;
        }
        
        float newBalance = client.deposit(accountNum, name, password, currency, amount);
        
        System.out.println("\n┌─────────────────────────────────────────────────────────┐");
        System.out.println("│              DEPOSIT SUCCESSFUL!                        │");
        System.out.println("├─────────────────────────────────────────────────────────┤");
        System.out.printf("│  Deposited:    %.2f %-35s │%n", amount, currency);
        System.out.printf("│  New Balance:  %.2f %-35s │%n", newBalance, currency);
        System.out.println("└─────────────────────────────────────────────────────────┘");
    }
    
    private void handleWithdraw() throws Exception {
        System.out.println("\n─── WITHDRAW MONEY ───");
        
        int accountNum = getIntInput("Enter account number: ");
        String name = getStringInput("Enter your name: ");
        String password = getPasswordInput("Enter password: ");
        Protocol.Currency currency = getCurrencyInput("Select currency");
        float amount = getFloatInput("Enter withdrawal amount: ");
        
        if (amount <= 0) {
            System.out.println("Withdrawal amount must be positive!");
            return;
        }
        
        float newBalance = client.withdraw(accountNum, name, password, currency, amount);
        
        System.out.println("\n┌─────────────────────────────────────────────────────────┐");
        System.out.println("│             WITHDRAWAL SUCCESSFUL!                      │");
        System.out.println("├─────────────────────────────────────────────────────────┤");
        System.out.printf("│  Withdrawn:    %.2f %-35s │%n", amount, currency);
        System.out.printf("│  New Balance:  %.2f %-35s │%n", newBalance, currency);
        System.out.println("└─────────────────────────────────────────────────────────┘");
    }
    
    private void handleTransfer() throws Exception {
        System.out.println("\n─── TRANSFER MONEY (Non-Idempotent) ───");
        System.out.println("This operation is NON-IDEMPOTENT:");
        System.out.println("Repeated execution will transfer money multiple times!");
        
        int senderAccountNum = getIntInput("Enter your account number (sender): ");
        String senderName = getStringInput("Enter your name: ");
        String senderPassword = getPasswordInput("Enter your password: ");
        int receiverAccountNum = getIntInput("Enter receiver account number: ");
        String receiverName = getStringInput("Enter receiver's name: ");
        Protocol.Currency currency = getCurrencyInput("Select currency");
        float amount = getFloatInput("Enter transfer amount: ");

        if (amount <= 0) {
            System.out.println("Transfer amount must be positive!");
            return;
        }

        String confirm = getStringInput("Confirm transfer of " + amount + " " + currency +
                                       " to account #" + receiverAccountNum + "? (yes/no): ");
        if (!confirm.equalsIgnoreCase("yes")) {
            System.out.println("Transfer cancelled.");
            return;
        }

        float newBalance = client.transfer(senderAccountNum, senderName, senderPassword,
                                          receiverAccountNum, currency, amount, receiverName);
        
        System.out.println("\n┌─────────────────────────────────────────────────────────┐");
        System.out.println("│             TRANSFER SUCCESSFUL!                        │");
        System.out.println("├─────────────────────────────────────────────────────────┤");
        System.out.printf("│  Transferred:       %.2f %-30s │%n", amount, currency);
        System.out.printf("│  To Account:        %-36d │%n", receiverAccountNum);
        System.out.printf("│  Your New Balance:  %.2f %-30s │%n", newBalance, currency);
        System.out.println("└─────────────────────────────────────────────────────────┘");
    }
    
    private void handleCheckBalance() throws Exception {
        System.out.println("\n─── CHECK BALANCE (Idempotent) ───");
        System.out.println("This operation is IDEMPOTENT:");
        System.out.println("Safe to repeat - always returns current balance.");
        
        int accountNum = getIntInput("Enter account number: ");
        String name = getStringInput("Enter your name: ");
        String password = getPasswordInput("Enter password: ");
        
        float balance = client.checkBalance(accountNum, name, password);
        
        System.out.println("\n┌─────────────────────────────────────────────────────────┐");
        System.out.println("│                ACCOUNT BALANCE                          │");
        System.out.println("├─────────────────────────────────────────────────────────┤");
        System.out.printf("│  Account #%-45d │%n", accountNum);
        System.out.printf("│  Balance:  %.2f                                      │%n", balance);
        System.out.println("└─────────────────────────────────────────────────────────┘");
    }
    
    private void handleMonitor() throws Exception {
        System.out.println("\n─── MONITOR ACCOUNT UPDATES ───");
        System.out.println("⚠ You will be blocked from making requests during monitoring!");
        
        int durationSeconds = getIntInput("Enter monitoring duration (seconds): ");
        
        if (durationSeconds <= 0) {
            System.out.println("Duration must be positive!");
            return;
        }
        
        System.out.println("\nStarting monitor mode...");
        System.out.println("Watching for account updates from the server.");
        System.out.println("This client will display all account changes in real-time.\n");
        
        client.monitor(durationSeconds, callback -> {
            // Callback is already printed by BankingClient, but we can add custom formatting
            System.out.println("┌─────────────────────────────────────────────────────────┐");
            System.out.println("│  Event: " + callback.eventDescription);
            System.out.println("│  Account: #" + callback.accountNum + " (" + callback.holderName + ")");
            System.out.println("│  Balance: " + callback.balance + " " + callback.currency);
            System.out.println("└─────────────────────────────────────────────────────────┘");
        });
    }
    
    private void handleSimulatePacketLoss() {
        System.out.println("\n─── SIMULATE PACKET LOSS (Testing) ───");
        System.out.println("This feature simulates network packet loss for testing");
        System.out.println("timeout/retry logic and invocation semantics.\n");
        
        double rate = getFloatInput("Enter packet loss rate (0.0 - 1.0, e.g., 0.3 for 30%): ");
        
        if (rate < 0.0 || rate > 1.0) {
            System.out.println("Invalid rate. Must be between 0.0 and 1.0");
            return;
        }
        
        client.setPacketLossRate(rate);
        
        if (rate > 0) {
            System.out.println("✓ Packet loss simulation enabled at " + (rate * 100) + "%");
            System.out.println("  Try making some requests to observe retry behavior!");
        } else {
            System.out.println("✓ Packet loss simulation disabled");
        }
    }
    
    // ─── Input Helper Methods ─────────────────────────────────────────────────────
    
    private String getStringInput(String prompt) {
        System.out.print(prompt);
        return scanner.nextLine().trim();
    }
    
    private int getIntInput(String prompt) {
        while (true) {
            System.out.print(prompt);
            try {
                String input = scanner.nextLine().trim();
                return Integer.parseInt(input);
            } catch (NumberFormatException e) {
                System.out.println("Invalid input. Please enter a number.");
            }
        }
    }
    
    private float getFloatInput(String prompt) {
        while (true) {
            System.out.print(prompt);
            try {
                String input = scanner.nextLine().trim();
                return Float.parseFloat(input);
            } catch (NumberFormatException e) {
                System.out.println("Invalid input. Please enter a number.");
            }
        }
    }
    
    private String getPasswordInput(String prompt) {
        String password = getStringInput(prompt);
        if (password.length() > Protocol.PASSWORD_LEN) {
            System.out.println("⚠ Warning: Password will be truncated to " + 
                             Protocol.PASSWORD_LEN + " bytes");
            password = password.substring(0, Protocol.PASSWORD_LEN);
        }
        return password;
    }
    
    private Protocol.Currency getCurrencyInput(String prompt) {
        System.out.println("\n" + prompt + ":");
        Protocol.Currency[] currencies = Protocol.Currency.values();
        for (int i = 0; i < currencies.length; i++) {
            System.out.printf("  %d. %s%n", i + 1, currencies[i]);
        }
        
        while (true) {
            int choice = getIntInput("Enter choice (1-" + currencies.length + "): ");
            if (choice >= 1 && choice <= currencies.length) {
                return currencies[choice - 1];
            }
            System.out.println("Invalid choice. Please try again.");
        }
    }
}
