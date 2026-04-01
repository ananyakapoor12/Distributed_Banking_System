package client;

import client.core.BankingClient;
import client.protocol.Protocol;
import client.ui.ConsoleUI;

/**
 * Main entry point for the Distributed Banking System Client.
 * 
 * Usage:
 *   java client.Main <server_host> <server_port> <semantics>
 * 
 * Example:
 *   java client.Main localhost 2222 at-least-once
 *   java client.Main 192.168.1.100 2222 at-most-once
 * 
 * Arguments:
 *   server_host  : IP address or hostname of the server
 *   server_port  : Port number the server is listening on
 *   semantics    : Invocation semantics - either "at-least-once" or "at-most-once"
 */
public class Main {
    
    public static void main(String[] args) {
        // Default configuration
        //String serverHost = "localhost";
        String serverHost = "10.91.181.242";
        int serverPort = 2222;
        Protocol.Semantics semantics = Protocol.Semantics.AT_LEAST_ONCE;
        
        // Parse command-line arguments
        if (args.length >= 1) {
            serverHost = args[0];
        }
        
        if (args.length >= 2) {
            try {
                serverPort = Integer.parseInt(args[1]);
            } catch (NumberFormatException e) {
                System.err.println("Error: Invalid port number '" + args[1] + "'");
                printUsage();
                System.exit(1);
            }
        }
        
        if (args.length >= 3) {
            String semanticsArg = args[2].toLowerCase();
            if (semanticsArg.equals("at-least-once")) {
                semantics = Protocol.Semantics.AT_LEAST_ONCE;
            } else if (semanticsArg.equals("at-most-once")) {
                semantics = Protocol.Semantics.AT_MOST_ONCE;
            } else {
                System.err.println("Error: Invalid semantics '" + args[2] + "'");
                System.err.println("       Must be 'at-least-once' or 'at-most-once'");
                printUsage();
                System.exit(1);
            }
        }
        
        // Display configuration
        printBanner();
        System.out.println("Starting client with configuration:");
        System.out.println("  Server Host: " + serverHost);
        System.out.println("  Server Port: " + serverPort);
        System.out.println("  Invocation Semantics: " + semantics);
        System.out.println();
        
        // Initialize and start client
        try {
            BankingClient client = new BankingClient(serverHost, serverPort, semantics);
            ConsoleUI ui = new ConsoleUI(client);
            ui.start();
            
        } catch (Exception e) {
            System.err.println("\nFatal Error: Failed to initialize client");
            System.err.println("  " + e.getMessage());
            if (e.getCause() != null) {
                System.err.println("  Caused by: " + e.getCause().getMessage());
            }
            System.err.println("\nPlease check:");
            System.err.println("  1. Server is running and accessible");
            System.err.println("  2. Server address and port are correct");
            System.err.println("  3. Network/firewall settings allow UDP traffic");
            System.exit(1);
        }
    }
    
    private static void printBanner() {
        System.out.println("╔═══════════════════════════════════════════════════════════╗");
        System.out.println("║                                                           ║");
        System.out.println("║       DISTRIBUTED BANKING SYSTEM - Java Client            ║");
        System.out.println("║                                                           ║");
        System.out.println("║  Course: CE4013/CZ4013/SC4051 Distributed Systems         ║");
        System.out.println("║  NTU - College of Computing and Data Science              ║");
        System.out.println("║                                                           ║");
        System.out.println("╚═══════════════════════════════════════════════════════════╝");
        System.out.println();
    }
    
    private static void printUsage() {
        System.out.println("\nUsage:");
        System.out.println("  java client.Main [server_host] [server_port] [semantics]");
        System.out.println();
        System.out.println("Arguments:");
        System.out.println("  server_host : IP address or hostname (default: localhost)");
        System.out.println("  server_port : Port number (default: 2222)");
        System.out.println("  semantics   : 'at-least-once' or 'at-most-once' (default: at-least-once)");
        System.out.println();
        System.out.println("Examples:");
        System.out.println("  java client.Main");
        System.out.println("  java client.Main localhost 2222 at-least-once");
        System.out.println("  java client.Main 192.168.1.100 2222 at-most-once");
    }
}
