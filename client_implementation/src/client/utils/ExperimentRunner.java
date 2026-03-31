package client.utils;

import client.core.BankingClient;
import client.protocol.Protocol;

/**
 * ExperimentRunner measures success rate and consistency for idempotent vs
 * non-idempotent operations under different packet loss rates.
 *
 * Run against ALO server first, then AMO server, to compare results.
 *
 * Usage:
 *   java -cp build client.utils.ExperimentRunner localhost 2222 at-least-once
 *   java -cp build client.utils.ExperimentRunner localhost 2222 at-most-once
 */
public class ExperimentRunner {

    private static final int N = 10;              // requests per experiment
    private static final float TRANSFER_AMT = 50.0f;
    private static final float INITIAL_BAL   = 5000.0f; // large enough for N transfers

    private final BankingClient client;
    private final String semantics;

    public ExperimentRunner(BankingClient client, String semantics) {
        this.client = client;
        this.semantics = semantics;
    }

    // ── Helpers ──────────────────────────────────────────────────────────────

    /** Run N check-balance calls. Returns success count. */
    private int runCheckBalance(int accountNum, String name, String password) {
        int successes = 0;
        for (int i = 0; i < N; i++) {
            try {
                client.checkBalance(accountNum, name, password);
                successes++;
            } catch (Exception ignored) {}
        }
        return successes;
    }

    /**
     * Run N transfer calls using response-only loss to force retries.
     * Under ALO: each retry re-executes the transfer on the server.
     * Under AMO: server returns cached reply, transfer executes exactly once per unique ID.
     * Returns {successes, senderFinalBalance}.
     */
    private float[] runTransfers(int senderAcct, int receiverAcct) throws Exception {
        int successes = 0;
        float lastBalance = -1;
        for (int i = 0; i < N; i++) {
            try {
                lastBalance = client.transfer(
                        senderAcct, "SenderUser", "sndpwd1",
                        receiverAcct, Protocol.Currency.SGD,
                        TRANSFER_AMT, "RecvUser");
                successes++;
            } catch (Exception ignored) {}
        }
        return new float[]{successes, lastBalance};
    }

    // ── Experiment 1: Idempotent (CHECK_BALANCE) ──────────────────────────────

    private void runIdempotentExperiment(double lossRate) throws Exception {
        // Fresh account
        int acct = client.openAccount("TestUser", "testpwd1",
                Protocol.Currency.SGD, INITIAL_BAL);

        client.setPacketLossRate(lossRate);
        int successes = runCheckBalance(acct, "TestUser", "testpwd1");
        client.setPacketLossRate(0.0);

        double successRate = (successes * 100.0) / N;
        // Idempotent — always consistent if response received
        System.out.printf("  %-18s  %3.0f%%     %5.1f%%          100%% (read-only)%n",
                "CHECK_BALANCE", lossRate * 100, successRate);
    }

    // ── Experiment 2: Non-Idempotent (TRANSFER) ───────────────────────────────

    private void runNonIdempotentExperiment(double lossRate) throws Exception {
        // Fresh sender + receiver accounts
        int senderAcct   = client.openAccount("SenderUser", "sndpwd1",
                Protocol.Currency.SGD, INITIAL_BAL);
        int receiverAcct = client.openAccount("RecvUser", "rcvpwd1",
                Protocol.Currency.SGD, 0.0f);

        // Use response-only loss: request always reaches server (executes),
        // reply is dropped → client retries same ID.
        // ALO: server re-executes transfer on every retry → extra money moved.
        // AMO: server returns cached reply → transfer happens exactly once.
        client.setResponseLossRate(lossRate);
        float[] result = runTransfers(senderAcct, receiverAcct);
        client.setResponseLossRate(0.0);

        int successes = (int) result[0];
        double successRate = (successes * 100.0) / N;

        // Get actual final balances
        float senderFinal   = client.checkBalance(senderAcct,   "SenderUser", "sndpwd1");
        float receiverFinal = client.checkBalance(receiverAcct, "RecvUser",   "rcvpwd1");

        // Expected: N transfers of TRANSFER_AMT each (one per unique request)
        float expectedSender   = INITIAL_BAL - (N * TRANSFER_AMT);
        float expectedReceiver = N * TRANSFER_AMT;

        boolean consistent = Math.abs(senderFinal - expectedSender) < 0.01f
                          && Math.abs(receiverFinal - expectedReceiver) < 0.01f;

        String consistencyStr;
        if (consistent) {
            consistencyStr = "100%";
        } else {
            // How many times was the transfer actually executed?
            int actualExecs = Math.round((INITIAL_BAL - senderFinal) / TRANSFER_AMT);
            consistencyStr = String.format("No (executed ~%dx, sender=%.0f expected=%.0f)",
                    actualExecs, senderFinal, expectedSender);
        }

        System.out.printf("  %-18s  %3.0f%%     %5.1f%%          %s%n",
                "TRANSFER", lossRate * 100, successRate, consistencyStr);
    }

    // ── Run all experiments ───────────────────────────────────────────────────

    public void run() throws Exception {
        System.out.println("\n╔══════════════════════════════════════════════════════════════════╗");
        System.out.println("║         EXPERIMENT RESULTS — " + semantics.toUpperCase());
        System.out.println("╠══════════════════════════════════════════════════════════════════╣");
        System.out.println("║  Setup: N=" + N + " requests per cell, response-loss for non-idempotent  ║");
        System.out.println("╚══════════════════════════════════════════════════════════════════╝");

        System.out.println("\n── Idempotent Operations (CHECK_BALANCE) ──────────────────────────");
        System.out.println("  Operation           Loss    Success Rate    Consistency");
        System.out.println("  ──────────────────────────────────────────────────────────────────");
        runIdempotentExperiment(0.30);
        runIdempotentExperiment(0.50);

        System.out.println("\n── Non-Idempotent Operations (TRANSFER) ───────────────────────────");
        System.out.println("  Operation           Loss    Success Rate    Consistency");
        System.out.println("  ──────────────────────────────────────────────────────────────────");
        runNonIdempotentExperiment(0.30);
        runNonIdempotentExperiment(0.50);

        System.out.println("\n── Summary ────────────────────────────────────────────────────────");
        System.out.println("  Semantics used: " + semantics);
        if (semantics.equalsIgnoreCase("at-most-once")) {
            System.out.println("  Expected: TRANSFER consistency = 100% (server caches, no double exec)");
        } else {
            System.out.println("  Expected: TRANSFER consistency < 100% (server re-executes retries)");
        }
        System.out.println("  ──────────────────────────────────────────────────────────────────\n");
    }

    // ── Main ─────────────────────────────────────────────────────────────────

    public static void main(String[] args) throws Exception {
        String host     = args.length > 0 ? args[0] : "localhost";
        int    port     = args.length > 1 ? Integer.parseInt(args[1]) : 2222;
        String semStr   = args.length > 2 ? args[2] : "at-most-once";

        Protocol.Semantics sem = semStr.equalsIgnoreCase("at-least-once")
                ? Protocol.Semantics.AT_LEAST_ONCE
                : Protocol.Semantics.AT_MOST_ONCE;

        System.out.println("Connecting to " + host + ":" + port + " [" + semStr + "]");
        BankingClient client = new BankingClient(host, port, sem);
        ExperimentRunner runner = new ExperimentRunner(client, semStr);
        runner.run();
        client.close();
    }
}
