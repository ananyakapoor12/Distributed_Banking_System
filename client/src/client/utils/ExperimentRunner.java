package client.utils;

import client.core.BankingClient;
import client.protocol.Protocol;

/**
 * ExperimentRunner generates experimental results for the lab report.
 *
 * Matches the table format from the sample report:
 *   - Idempotent table:     Operation | Loss | ALO Success | AMO Success
 *   - Non-Idempotent table: Operation | Loss | ALO Success | AMO Success | ALO Consistency | AMO Consistency
 *
 * Run TWICE — once per server mode:
 *   Terminal 1:  ./server 2222 alo
 *   Terminal 2:  java -cp build client.utils.ExperimentRunner <host> 2222 at-least-once
 *
 *   Terminal 1:  ./server 2222 amo
 *   Terminal 2:  java -cp build client.utils.ExperimentRunner <host> 2222 at-most-once
 *
 * Copy the printed numbers into the combined tables below.
 */
public class ExperimentRunner {

    private static final int    N             = 20;      // requests per cell (higher = smoother %)
    private static final float  TRANSFER_AMT  = 50.0f;
    private static final float  INITIAL_BAL   = 5000.0f; // enough for N transfers
    private static final float  RECEIVER_BAL  = 100.0f;  // receiver needs non-zero opening balance

    private final BankingClient client;
    private final String        semantics;    // "at-least-once" or "at-most-once"

    // Stored results for final table print
    private double idem30, idem50;                        // idempotent success rates
    private double nonSucc30, nonSucc50;                  // non-idempotent success rates
    private double nonCons30, nonCons50;                  // non-idempotent consistency rates

    public ExperimentRunner(BankingClient client, String semantics) {
        this.client    = client;
        this.semantics = semantics;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Idempotent: CHECK_BALANCE
    // Uses regular packetLossRate (drops request OR response randomly).
    // Read-only → always consistent if response received.
    // ─────────────────────────────────────────────────────────────────────────

    private double runIdempotent(double lossRate) throws Exception {
        int acct = client.openAccount("TestUser", "testpass", Protocol.Currency.SGD, INITIAL_BAL);
        client.setPacketLossRate(lossRate);
        int successes = 0;
        for (int i = 0; i < N; i++) {
            try { client.checkBalance(acct, "TestUser", "testpass"); successes++; }
            catch (Exception ignored) {}
        }
        client.setPacketLossRate(0.0);
        return (successes * 100.0) / N;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Non-Idempotent: TRANSFER
    // Uses response-only loss → request always reaches server (executes),
    // reply dropped → client retries same ID.
    // ALO: server re-executes on retry  → more money moved than intended → inconsistent.
    // AMO: server returns cached reply  → executes exactly once per ID  → consistent.
    // ─────────────────────────────────────────────────────────────────────────

    private double[] runNonIdempotent(double lossRate) throws Exception {
        int sender   = client.openAccount("SenderUser", "sndpass", Protocol.Currency.SGD, INITIAL_BAL);
        int receiver = client.openAccount("RecvUser",   "rcvpass", Protocol.Currency.SGD, RECEIVER_BAL);

        client.setResponseLossRate(lossRate);
        int successes = 0;
        for (int i = 0; i < N; i++) {
            try {
                client.transfer(sender, "SenderUser", "sndpass",
                                receiver, Protocol.Currency.SGD,
                                TRANSFER_AMT, "RecvUser");
                successes++;
            } catch (Exception ignored) {}
        }
        client.setResponseLossRate(0.0);

        double successRate = (successes * 100.0) / N;

        // Check final balances with no loss
        float senderFinal   = client.checkBalance(sender,   "SenderUser", "sndpass");
        float receiverFinal = client.checkBalance(receiver, "RecvUser",   "rcvpass");

        // Expected if each unique request executed exactly once
        float expectedSender   = INITIAL_BAL - (N * TRANSFER_AMT);
        float expectedReceiver = RECEIVER_BAL + (N * TRANSFER_AMT);

        boolean consistent = Math.abs(senderFinal   - expectedSender)   < 0.01f
                          && Math.abs(receiverFinal - expectedReceiver) < 0.01f;

        double consistencyRate;
        if (consistent) {
            consistencyRate = 100.0;
        } else {
            // Estimate how many times transfer actually executed
            int actualExecs = Math.round((INITIAL_BAL - senderFinal) / TRANSFER_AMT);
            // Consistency = fraction of requests that were NOT double-executed
            // (requests that got response on first try → no retry → correct)
            consistencyRate = (successes > 0)
                    ? Math.max(0, (N - (actualExecs - N)) * 100.0 / N)
                    : 0.0;
            System.out.printf("    [DEBUG] loss=%.0f%% sender=%.0f expected=%.0f actualExecs=%d%n",
                    lossRate * 100, senderFinal, expectedSender, actualExecs);
        }

        return new double[]{successRate, consistencyRate};
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Run all experiments
    // ─────────────────────────────────────────────────────────────────────────

    public void run() throws Exception {
        String label = semantics.equalsIgnoreCase("at-least-once") ? "AT-LEAST-ONCE" : "AT-MOST-ONCE";

        System.out.println("\n╔══════════════════════════════════════════════════════════════╗");
        System.out.printf( "║  EXPERIMENT RESULTS  ──  %-36s║%n", label);
        System.out.printf( "║  N = %-3d requests per cell                                  ║%n", N);
        System.out.println("╚══════════════════════════════════════════════════════════════╝\n");

        // ── Idempotent ──────────────────────────────────────────────────────
        System.out.println("Running idempotent experiments (CHECK_BALANCE)...");
        idem30 = runIdempotent(0.30);
        idem50 = runIdempotent(0.50);

        // ── Non-Idempotent ──────────────────────────────────────────────────
        System.out.println("Running non-idempotent experiments (TRANSFER)...");
        double[] r30 = runNonIdempotent(0.30);
        double[] r50 = runNonIdempotent(0.50);
        nonSucc30 = r30[0];  nonCons30 = r30[1];
        nonSucc50 = r50[0];  nonCons50 = r50[1];

        printTables(label);
    }

    private void printTables(String label) {
        // ── Table 1: Idempotent ─────────────────────────────────────────────
        System.out.println("\n┌─────────────────────────────────────────────────────────────────┐");
        System.out.println("│           IDEMPOTENT OPERATIONS RESULTS                         │");
        System.out.println("├─────────────────┬───────────┬────────────────────────────────────┤");
        System.out.println("│   Operation     │ Loss Rate │  " + label + " (Success Rate)  │");
        System.out.println("├─────────────────┼───────────┼────────────────────────────────────┤");
        System.out.printf( "│ CHECK_BALANCE   │    30%%    │              %5.1f%%              │%n", idem30);
        System.out.printf( "│ CHECK_BALANCE   │    50%%    │              %5.1f%%              │%n", idem50);
        System.out.println("└─────────────────┴───────────┴────────────────────────────────────┘");

        // ── Table 2: Non-Idempotent ─────────────────────────────────────────
        System.out.println("\n┌───────────────────────────────────────────────────────────────────────────────┐");
        System.out.println("│                  NON-IDEMPOTENT OPERATIONS RESULTS                           │");
        System.out.println("├──────────┬───────────┬──────────────────────┬─────────────────────────────────┤");
        System.out.println("│Operation │ Loss Rate │  " + label + " (Success) │  " + label + " (Consistency)  │");
        System.out.println("├──────────┼───────────┼──────────────────────┼─────────────────────────────────┤");
        System.out.printf( "│ TRANSFER │    30%%    │        %5.1f%%        │           %5.1f%%            │%n", nonSucc30, nonCons30);
        System.out.printf( "│ TRANSFER │    50%%    │        %5.1f%%        │           %5.1f%%            │%n", nonSucc50, nonCons50);
        System.out.println("└──────────┴───────────┴──────────────────────┴─────────────────────────────────┘");

        // ── Combined template ───────────────────────────────────────────────
        System.out.println("\n╔══════════════════════════════════════════════════════════════════════════════════════╗");
        System.out.println("║  COMBINED TABLE (fill in after running both ALO and AMO)                            ║");
        System.out.println("╠══════════════════════════════════════════════════════════════════════════════════════╣");
        System.out.println("║  IDEMPOTENT:                                                                        ║");
        System.out.println("║  Operation     | Loss | AT-LEAST-ONCE (Success) | AT-MOST-ONCE (Success)           ║");
        System.out.printf( "║  CHECK_BALANCE |  30%% |         ??.?%%          |         ??.?%%                  ║%n");
        System.out.printf( "║  CHECK_BALANCE |  50%% |         ??.?%%          |         ??.?%%                  ║%n");
        System.out.println("║                                                                                     ║");
        System.out.println("║  NON-IDEMPOTENT:                                                                    ║");
        System.out.println("║  Operation | Loss | ALO Success | AMO Success | ALO Consistency | AMO Consistency  ║");
        System.out.printf( "║  TRANSFER  |  30%% |    ??.?%%   |    ??.?%%   |     ??.?%%      |     100%%        ║%n");
        System.out.printf( "║  TRANSFER  |  50%% |    ??.?%%   |    ??.?%%   |     ??.?%%      |     100%%        ║%n");
        System.out.println("╚══════════════════════════════════════════════════════════════════════════════════════╝");

        System.out.println("\n  → This run (" + label + ") numbers:");
        System.out.printf( "     Idempotent    30%%: success=%.1f%%%n", idem30);
        System.out.printf( "     Idempotent    50%%: success=%.1f%%%n", idem50);
        System.out.printf( "     Non-Idempotent 30%%: success=%.1f%%, consistency=%.1f%%%n", nonSucc30, nonCons30);
        System.out.printf( "     Non-Idempotent 50%%: success=%.1f%%, consistency=%.1f%%%n", nonSucc50, nonCons50);
        System.out.println();
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Main
    // ─────────────────────────────────────────────────────────────────────────

    public static void main(String[] args) throws Exception {
        String host   = args.length > 0 ? args[0] : "localhost";
        int    port   = args.length > 1 ? Integer.parseInt(args[1]) : 2222;
        String semStr = args.length > 2 ? args[2] : "at-least-once";

        Protocol.Semantics sem = semStr.equalsIgnoreCase("at-least-once")
                ? Protocol.Semantics.AT_LEAST_ONCE
                : Protocol.Semantics.AT_MOST_ONCE;

        System.out.println("Connecting to " + host + ":" + port + " [" + semStr + "]");
        BankingClient client = new BankingClient(host, port, sem);
        new ExperimentRunner(client, semStr).run();
        client.close();
    }
}
