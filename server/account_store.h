#pragma once

#include <unordered_map>
#include <string>

// Currency codes — encoded as 1 byte on the wire.
// Using a map instead of an enum so that unknown byte values can be
// detected at runtime rather than silently becoming undefined behaviour.
inline const std::unordered_map<int, std::string> CURRENCY_NAMES = {
    {0, "SGD"}, {1, "USD"}, {2, "INR"}, {3, "AUD"}, {4, "CNY"},
    {5, "EUR"}, {6, "CAD"}, {7, "GBP"}, {8, "CHF"}
};

inline bool is_valid_currency(int raw) {
    return CURRENCY_NAMES.count(raw) > 0;
}

enum class ErrorCode : uint8_t
{
    // General / Protocol-Level
    SUCCESS = 0,
    UNKNOWN_OPERATION = 1,
    MALFORMED_REQUEST = 2, // arguments aren't in the right order
    CORRUPT_REQUEST = 3,   // checksum didn't match
    DUPLICATE_REQUEST = 4, // for at-most-once filtering

    // Authentication / Identity
    AUTH_FAILED = 10, // wrong password
    ACCOUNT_NOT_FOUND = 11,
    NAME_ACCOUNT_MISMATCH = 12, // account number not under given name

    // Account State
    INSUFFICIENT_BALANCE = 20,
    ACCOUNT_ALREADY_EXISTS = 21,
    ACCOUNT_ALREADY_CLOSED = 22,
    ACCOUNT_NOT_CLOSED = 23, // closing fails due to rule

    // Operation-Specific
    INVALID_CURRENCY = 30,
    CURRENCY_MISMATCH = 31,
    INVALID_AMOUNT = 32, // negative or zero amount where not allowed

    // Monitoring / Callback
    MONITOR_REGISTRATION_FAILED = 40,
    MONITOR_INTERVAL_INVALID = 41,
    NOT_REGISTERED_FOR_MONITOR = 42,

    SENDER_CURRENCY_MISMATCH = 50,
    RECEIVER_CURRENCY_MISMATCH = 51,
    RECEIVER_ACCOUNT_NOT_FOUND = 52,
    RECEIVER_NAME_MISMATCH = 53,

};

template <typename T>
struct Result
{
    ErrorCode code;
    T value;

    // Factory helpers
    static Result success(T val) { return {ErrorCode::SUCCESS, std::move(val)}; }
    static Result fail(ErrorCode err) { return {err, T{}}; }
};

struct BankAccountBalance
{
    int currency;
    float value;
};

struct BankAccount
{
    int account_number;
    std::string holder_name;
    std::string password;
    BankAccountBalance balance;
};

class AccountStore {
public:
    // Returns the new account number (>= 1) on success.
    // Returns -1 if balance <= 0 or currency code is not in CURRENCY_NAMES.
    Result<int> open_account(const std::string &name, const std::string &password,
                             int currency, float balance);

    // Returns true if account_num exists, holder_name matches, and password matches.
    bool auth(BankAccount acc, int account_num, const std::string &name, const std::string &password) const;

    Result<bool> close_account(int account_num, const std::string &name, const std::string &password);

    Result<BankAccountBalance> deposit(int account_num, const std::string &name, const std::string &password, int currency, float amt);
    
    Result<BankAccountBalance> withdraw(int account_num, const std::string &name, const std::string &password, int currency, float amt);

    Result<BankAccountBalance> check_balance(int account_num, const std::string &name, const std::string &password);

    Result<BankAccountBalance> transfer(int sender_account_num, const std::string &sender_name, const std::string &password, int receiver_account_num, const std::string &receiver_name, int currency, float amt);

private:
    std::unordered_map<int, BankAccount> accounts_;
    int next_id_{1};
};
