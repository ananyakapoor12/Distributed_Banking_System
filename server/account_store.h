#pragma once

#include <string>
#include "protocol.h"
#include <SQLiteCpp/SQLiteCpp.h>

// Currency codes — encoded as 1 byte on the wire.
// Using a map instead of an enum so that unknown byte values can be
// detected at runtime rather than silently becoming undefined behaviour.
// #include <unordered_map>
// inline const std::unordered_map<int, std::string> CURRENCY_NAMES = {
//     {0, "SGD"}, {1, "USD"}, {2, "INR"}, {3, "AUD"}, {4, "CNY"},
//     {5, "EUR"}, {6, "CAD"}, {7, "GBP"}, {8, "CHF"}
// };

// inline bool is_valid_currency(int raw) {
//     return CURRENCY_NAMES.count(raw) > 0;
// }

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
    Currency currency;
    float value;
};

class AccountStore {
public:
    explicit AccountStore(const std::string &db_path);

    // Returns the new account number (>= 1) on success.
    // Returns -1 if balance <= 0 or currency code is not in CURRENCY_NAMES.
    Result<int> open_account(const std::string &name, const std::string &password,
                             Currency currency, float balance);

    Result<BankAccountBalance> close_account(int account_num, const std::string &name, const std::string &password);

    Result<BankAccountBalance> deposit(int account_num, const std::string &name, const std::string &password, Currency currency, float amt);

    Result<BankAccountBalance> withdraw(int account_num, const std::string &name, const std::string &password, Currency currency, float amt);

    Result<BankAccountBalance> check_balance(int account_num, const std::string &name, const std::string &password);

    Result<BankAccountBalance> transfer(int sender_account_num, const std::string &sender_name, const std::string &password, int receiver_account_num, const std::string &receiver_name, Currency currency, float amt);

private:
    SQLite::Database db_;

    void init_schema();

    // Fetches a row by account_number. Returns false (sets out params to 0/"")
    // if not found. Caller checks the return value.
    bool fetch_account(int account_num,
                       std::string &out_holder_name,
                       std::string &out_password,
                       Currency&out_currency,
                       float &out_balance);

    bool auth_row(const std::string &stored_name, const std::string &stored_password,
                  const std::string &name, const std::string &password) const;
};
