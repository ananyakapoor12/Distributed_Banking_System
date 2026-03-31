#include "account_store.h"
#include <iostream>

#ifdef DEBUG
#define DBG(x) std::cout << x << std::endl
#else
#define DBG(x)
#endif

AccountStore::AccountStore(const std::string &db_path)
    : db_(db_path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
    init_schema();
    DBG("[DB] Opened database: " << db_path);
}

void AccountStore::init_schema()
{
    db_.exec(
        "CREATE TABLE IF NOT EXISTS accounts ("
        "  account_number INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  holder_name    TEXT    NOT NULL,"
        "  password       TEXT    NOT NULL,"
        "  currency       INTEGER NOT NULL CHECK (currency BETWEEN 0 AND 8),"
        "  balance        REAL    NOT NULL CHECK (balance >= 0)"
        ")"
    );
}

bool AccountStore::fetch_account(int account_num, std::string &out_holder_name, std::string &out_password, Currency &out_currency, float &out_balance)
{
    SQLite::Statement stmt(db_,
        "SELECT holder_name, password, currency, balance "
        "FROM accounts WHERE account_number = ?");
    stmt.bind(1, account_num);
    if (!stmt.executeStep())
        return false;
    out_holder_name = stmt.getColumn(0).getText();
    out_password = stmt.getColumn(1).getText();
    out_currency = static_cast<Currency>(stmt.getColumn(2).getInt());
    out_balance = (float)stmt.getColumn(3).getDouble();
    return true;
}

bool AccountStore::auth_row(const std::string &stored_name, const std::string &stored_password, const std::string &name, const std::string &password) const
{
    return stored_name == name && stored_password == password;
}

Result<int> AccountStore::open_account(const std::string &name, const std::string &password, Currency currency, float balance)
{
    DBG("[OPEN_ACCOUNT] name=" << name << " currency=" << currency << " balance=" << balance);
    if (balance <= 0) {
        DBG("[OPEN_ACCOUNT] FAILED: invalid amount " << balance);
        return Result<int>::fail(ErrorCode::INVALID_AMOUNT);
    }

    try {
        SQLite::Statement stmt(db_,
            "INSERT INTO accounts (holder_name, password, currency, balance) "
            "VALUES (?, ?, ?, ?)");
        stmt.bind(1, name);
        stmt.bind(2, password);
        stmt.bind(3, static_cast<int>(currency));
        stmt.bind(4, (double)balance);
        stmt.exec();
        int id = (int)db_.getLastInsertRowid();
        DBG("[OPEN_ACCOUNT] SUCCESS: account_num=" << id);
        return Result<int>::success(id);
    } catch (const SQLite::Exception &e) {
        std::cerr << "[OPEN_ACCOUNT] DB error: " << e.what() << std::endl;
        return Result<int>::fail(ErrorCode::UNKNOWN_OPERATION);
    }
}

Result<BankAccountBalance> AccountStore::close_account(int account_num,
                                                        const std::string &name,
                                                        const std::string &password)
{
    DBG("[CLOSE_ACCOUNT] account_num=" << account_num << " name=" << name);

    std::string stored_name, stored_password;
    Currency stored_currency;
    float stored_balance;

    if (!fetch_account(account_num, stored_name, stored_password, stored_currency, stored_balance)) {
        DBG("[CLOSE_ACCOUNT] FAILED: account not found");
        return Result<BankAccountBalance>::fail(ErrorCode::ACCOUNT_NOT_FOUND);
    }
    if (!auth_row(stored_name, stored_password, name, password)) {
        DBG("[CLOSE_ACCOUNT] FAILED: auth failed");
        return Result<BankAccountBalance>::fail(ErrorCode::AUTH_FAILED);
    }

    try {
        SQLite::Statement stmt(db_, "DELETE FROM accounts WHERE account_number = ?");
        stmt.bind(1, account_num);
        stmt.exec();
        DBG("[CLOSE_ACCOUNT] SUCCESS: account_num=" << account_num << " closed");
        return Result<BankAccountBalance>::success({static_cast<Currency>(stored_currency), stored_balance});
    } catch (const SQLite::Exception &e) {
        std::cerr << "[CLOSE_ACCOUNT] DB error: " << e.what() << std::endl;
        return Result<BankAccountBalance>::fail(ErrorCode::UNKNOWN_OPERATION);
    }
}

Result<BankAccountBalance> AccountStore::deposit(int account_num, const std::string &name,
                                                  const std::string &password,
                                                  Currency currency, float amt)
{
    DBG("[DEPOSIT] account_num=" << account_num << " name=" << name
        << " currency=" << currency << " amt=" << amt);

    std::string stored_name, stored_password;
    Currency stored_currency;
    float stored_balance;

    if (!fetch_account(account_num, stored_name, stored_password, stored_currency, stored_balance)) {
        DBG("[DEPOSIT] FAILED: account not found");
        return Result<BankAccountBalance>::fail(ErrorCode::ACCOUNT_NOT_FOUND);
    }
    if (!auth_row(stored_name, stored_password, name, password)) {
        DBG("[DEPOSIT] FAILED: auth failed");
        return Result<BankAccountBalance>::fail(ErrorCode::AUTH_FAILED);
    }
    if (amt <= 0) {
        DBG("[DEPOSIT] FAILED: invalid amount " << amt);
        return Result<BankAccountBalance>::fail(ErrorCode::INVALID_AMOUNT);
    }
    if (stored_currency != currency) {
        DBG("[DEPOSIT] FAILED: currency mismatch (account=" << stored_currency << " requested=" << currency << ")");
        return Result<BankAccountBalance>::fail(ErrorCode::CURRENCY_MISMATCH);
    }

    try {
        SQLite::Statement upd(db_,
            "UPDATE accounts SET balance = balance + ? WHERE account_number = ?");
        upd.bind(1, (double)amt);
        upd.bind(2, account_num);
        upd.exec();

        float new_balance = stored_balance + amt;
        DBG("[DEPOSIT] SUCCESS: new balance=" << new_balance);
        return Result<BankAccountBalance>::success({static_cast<Currency>(stored_currency), new_balance});
    } catch (const SQLite::Exception &e) {
        std::cerr << "[DEPOSIT] DB error: " << e.what() << std::endl;
        return Result<BankAccountBalance>::fail(ErrorCode::UNKNOWN_OPERATION);
    }
}

Result<BankAccountBalance> AccountStore::withdraw(int account_num, const std::string &name,
                                                   const std::string &password,
                                                   Currency currency, float amt)
{
    DBG("[WITHDRAW] account_num=" << account_num << " name=" << name
        << " currency=" << currency << " amt=" << amt);

    std::string stored_name, stored_password;
    Currency stored_currency;
    float stored_balance;

    if (!fetch_account(account_num, stored_name, stored_password, stored_currency, stored_balance)) {
        DBG("[WITHDRAW] FAILED: account not found");
        return Result<BankAccountBalance>::fail(ErrorCode::ACCOUNT_NOT_FOUND);
    }
    if (!auth_row(stored_name, stored_password, name, password)) {
        DBG("[WITHDRAW] FAILED: auth failed");
        return Result<BankAccountBalance>::fail(ErrorCode::AUTH_FAILED);
    }
    if (amt <= 0) {
        DBG("[WITHDRAW] FAILED: invalid amount " << amt);
        return Result<BankAccountBalance>::fail(ErrorCode::INVALID_AMOUNT);
    }
    if (stored_currency != currency) {
        DBG("[WITHDRAW] FAILED: currency mismatch (account=" << stored_currency << " requested=" << currency << ")");
        return Result<BankAccountBalance>::fail(ErrorCode::CURRENCY_MISMATCH);
    }
    if (stored_balance < amt) {
        DBG("[WITHDRAW] FAILED: insufficient balance (balance=" << stored_balance << " requested=" << amt << ")");
        return Result<BankAccountBalance>::fail(ErrorCode::INSUFFICIENT_BALANCE);
    }

    try {
        SQLite::Statement upd(db_,
            "UPDATE accounts SET balance = balance - ? WHERE account_number = ?");
        upd.bind(1, (double)amt);
        upd.bind(2, account_num);
        upd.exec();

        float new_balance = stored_balance - amt;
        DBG("[WITHDRAW] SUCCESS: new balance=" << new_balance);
        return Result<BankAccountBalance>::success({static_cast<Currency>(stored_currency), new_balance});
    } catch (const SQLite::Exception &e) {
        std::cerr << "[WITHDRAW] DB error: " << e.what() << std::endl;
        return Result<BankAccountBalance>::fail(ErrorCode::UNKNOWN_OPERATION);
    }
}

Result<BankAccountBalance> AccountStore::check_balance(int account_num,
                                                        const std::string &name,
                                                        const std::string &password)
{
    DBG("[CHECK_BALANCE] account_num=" << account_num << " name=" << name);

    std::string stored_name, stored_password;
    Currency stored_currency;
    float stored_balance;

    if (!fetch_account(account_num, stored_name, stored_password, stored_currency, stored_balance)) {
        DBG("[CHECK_BALANCE] FAILED: account not found");
        return Result<BankAccountBalance>::fail(ErrorCode::ACCOUNT_NOT_FOUND);
    }
    if (!auth_row(stored_name, stored_password, name, password)) {
        DBG("[CHECK_BALANCE] FAILED: auth failed");
        return Result<BankAccountBalance>::fail(ErrorCode::AUTH_FAILED);
    }

    DBG("[CHECK_BALANCE] SUCCESS: balance=" << stored_balance << " currency=" << stored_currency);
    return Result<BankAccountBalance>::success({static_cast<Currency>(stored_currency), stored_balance});
}

Result<BankAccountBalance> AccountStore::transfer(int sender_account_num,
                                                   const std::string &sender_name,
                                                   const std::string &password,
                                                   int receiver_account_num,
                                                   const std::string &receiver_name,
                                                   Currency currency, float amt)
{
    DBG("[TRANSFER] sender_account_num=" << sender_account_num << " sender_name=" << sender_name
        << " receiver_account_num=" << receiver_account_num << " receiver_name=" << receiver_name
        << " currency=" << currency << " amt=" << amt);

    std::string sender_name_s, sender_pass_s, receiver_name_s, receiver_pass_s;
    Currency sender_currency, receiver_currency;
    float sender_balance, receiver_balance;

    if (!fetch_account(sender_account_num, sender_name_s, sender_pass_s, sender_currency, sender_balance)) {
        DBG("[TRANSFER] FAILED: sender account not found");
        return Result<BankAccountBalance>::fail(ErrorCode::ACCOUNT_NOT_FOUND);
    }
    if (!auth_row(sender_name_s, sender_pass_s, sender_name, password)) {
        DBG("[TRANSFER] FAILED: auth failed");
        return Result<BankAccountBalance>::fail(ErrorCode::AUTH_FAILED);
    }
    if (amt <= 0) {
        DBG("[TRANSFER] FAILED: invalid amount " << amt);
        return Result<BankAccountBalance>::fail(ErrorCode::INVALID_AMOUNT);
    }
    if (sender_currency != currency) {
        DBG("[TRANSFER] FAILED: sender currency mismatch (account=" << sender_currency << " requested=" << currency << ")");
        return Result<BankAccountBalance>::fail(ErrorCode::SENDER_CURRENCY_MISMATCH);
    }
    if (sender_balance < amt) {
        DBG("[TRANSFER] FAILED: insufficient balance (balance=" << sender_balance << " requested=" << amt << ")");
        return Result<BankAccountBalance>::fail(ErrorCode::INSUFFICIENT_BALANCE);
    }

    if (!fetch_account(receiver_account_num, receiver_name_s, receiver_pass_s, receiver_currency, receiver_balance)) {
        DBG("[TRANSFER] FAILED: receiver account not found");
        return Result<BankAccountBalance>::fail(ErrorCode::RECEIVER_ACCOUNT_NOT_FOUND);
    }
    if (receiver_name_s != receiver_name) {
        DBG("[TRANSFER] FAILED: receiver name mismatch (expected=" << receiver_name_s << " got=" << receiver_name << ")");
        return Result<BankAccountBalance>::fail(ErrorCode::RECEIVER_NAME_MISMATCH);
    }
    if (receiver_currency != currency) {
        DBG("[TRANSFER] FAILED: receiver currency mismatch (account=" << receiver_currency << " requested=" << currency << ")");
        return Result<BankAccountBalance>::fail(ErrorCode::RECEIVER_CURRENCY_MISMATCH);
    }

    try {
        SQLite::Transaction txn(db_);

        SQLite::Statement debit(db_,
            "UPDATE accounts SET balance = balance - ? WHERE account_number = ?");
        debit.bind(1, (double)amt);
        debit.bind(2, sender_account_num);
        debit.exec();

        SQLite::Statement credit(db_,
            "UPDATE accounts SET balance = balance + ? WHERE account_number = ?");
        credit.bind(1, (double)amt);
        credit.bind(2, receiver_account_num);
        credit.exec();

        txn.commit();

        float new_sender_balance = sender_balance - amt;
        DBG("[TRANSFER] SUCCESS: sender new balance=" << new_sender_balance
            << " receiver new balance=" << (receiver_balance + amt));
        return Result<BankAccountBalance>::success({currency, new_sender_balance});
    } catch (const SQLite::Exception &e) {
        std::cerr << "[TRANSFER] DB error: " << e.what() << std::endl;
        return Result<BankAccountBalance>::fail(ErrorCode::UNKNOWN_OPERATION);
    }
}
