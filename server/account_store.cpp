#include "account_store.h"
#include <iostream>

#ifdef DEBUG
#define DBG(x) std::cout << x << std::endl
#else
#define DBG(x)
#endif

Result<int> AccountStore::open_account(const std::string &name, const std::string &password, Currency currency, float balance)
{
    DBG("[OPEN_ACCOUNT] name=" << name << " currency=" << currency << " balance=" << balance);
    if (balance <= 0) {
        DBG("[OPEN_ACCOUNT] FAILED: invalid amount " << balance);
        return Result<int>::fail(ErrorCode::INVALID_AMOUNT);
    }

    int id = next_id_++;
    accounts_[id] = BankAccount{id, name, password, BankAccountBalance{currency, balance}};
    DBG("[OPEN_ACCOUNT] SUCCESS: account_num=" << id);
    return  Result<int>::success(id);
}

bool AccountStore::auth(BankAccount acc, int account_num, const std::string &name, const std::string &password) const
{
    DBG("[AUTH] account_num=" << acc.account_number
        << " holder_name=" << acc.holder_name
        << " passwd=" << acc.password
        << " currency=" << (int)acc.balance.currency
        << " balance=" << acc.balance.value);
    return acc.holder_name == name && acc.password == password;
}

Result<BankAccountBalance> AccountStore::close_account(int account_num, const std::string &name, const std::string &password)
{
    DBG("[CLOSE_ACCOUNT] account_num=" << account_num << " name=" << name);
    auto it = accounts_.find(account_num);
    if (it == accounts_.end()) {
        DBG("[CLOSE_ACCOUNT] FAILED: account not found");
        return Result<BankAccountBalance>::fail(ErrorCode::ACCOUNT_NOT_FOUND);
    }
    const BankAccount &acc = it->second;
    if (!auth(acc, account_num, name, password)) {
        DBG("[CLOSE_ACCOUNT] FAILED: auth failed");
        return Result<BankAccountBalance>::fail(ErrorCode::AUTH_FAILED);
    }
    accounts_.erase(account_num);
    DBG("[CLOSE_ACCOUNT] SUCCESS: account_num=" << account_num << " closed");
    return Result<BankAccountBalance>::success(acc.balance);
}

Result<BankAccountBalance> AccountStore::deposit(int account_num, const std::string &name, const std::string &password, Currency currency, float amt)
{
    DBG("[DEPOSIT] account_num=" << account_num << " name=" << name << " currency=" << currency << " amt=" << amt);
    auto it = accounts_.find(account_num);
    if (it == accounts_.end()) {
        DBG("[DEPOSIT] FAILED: account not found");
        return Result<BankAccountBalance>::fail(ErrorCode::ACCOUNT_NOT_FOUND);
    }
    BankAccount &acc = it->second;
    if (!auth(acc, account_num, name, password)) {
        DBG("[DEPOSIT] FAILED: auth failed");
        return Result<BankAccountBalance>::fail(ErrorCode::AUTH_FAILED);
    }
    if (amt <= 0) {
        DBG("[DEPOSIT] FAILED: invalid amount " << amt);
        return Result<BankAccountBalance>::fail(ErrorCode::INVALID_AMOUNT);
    }
    if (acc.balance.currency == currency){
        acc.balance.value = acc.balance.value + amt;
        DBG("[DEPOSIT] SUCCESS: new balance=" << acc.balance.value);
        return Result<BankAccountBalance>::success(acc.balance);
    }
    else {
        DBG("[DEPOSIT] FAILED: currency mismatch (account=" << acc.balance.currency << " requested=" << currency << ")");
        return Result<BankAccountBalance>::fail(ErrorCode::CURRENCY_MISMATCH);
    }
}

Result<BankAccountBalance> AccountStore::withdraw(int account_num, const std::string &name, const std::string &password, Currency currency, float amt)
{
    DBG("[WITHDRAW] account_num=" << account_num << " name=" << name << " currency=" << currency << " amt=" << amt);
    auto it = accounts_.find(account_num);
    if (it == accounts_.end()) {
        DBG("[WITHDRAW] FAILED: account not found");
        return Result<BankAccountBalance>::fail(ErrorCode::ACCOUNT_NOT_FOUND);
    }
    BankAccount &acc = it->second;
    if (!auth(acc, account_num, name, password)) {
        DBG("[WITHDRAW] FAILED: auth failed");
        return Result<BankAccountBalance>::fail(ErrorCode::AUTH_FAILED);
    }
    if (amt <= 0) {
        DBG("[WITHDRAW] FAILED: invalid amount " << amt);
        return Result<BankAccountBalance>::fail(ErrorCode::INVALID_AMOUNT);
    }
    if (acc.balance.currency == currency)
    {
        if (acc.balance.value >= amt){
            acc.balance.value = acc.balance.value - amt;
            DBG("[WITHDRAW] SUCCESS: new balance=" << acc.balance.value);
            return Result<BankAccountBalance>::success(acc.balance);
        }
        else {
            DBG("[WITHDRAW] FAILED: insufficient balance (balance=" << acc.balance.value << " requested=" << amt << ")");
            return Result<BankAccountBalance>::fail(ErrorCode::INSUFFICIENT_BALANCE);
        }
    }
    else {
        DBG("[WITHDRAW] FAILED: currency mismatch (account=" << acc.balance.currency << " requested=" << currency << ")");
        return Result<BankAccountBalance>::fail(ErrorCode::CURRENCY_MISMATCH);
    }
}

Result<BankAccountBalance> AccountStore::check_balance(int account_num, const std::string &name, const std::string &password)
{
    DBG("[CHECK_BALANCE] account_num=" << account_num << " name=" << name);
    auto it = accounts_.find(account_num);
    if (it == accounts_.end()) {
        DBG("[CHECK_BALANCE] FAILED: account not found");
        return Result<BankAccountBalance>::fail(ErrorCode::ACCOUNT_NOT_FOUND);
    }
    const BankAccount &acc = it->second;
    if (!auth(acc, account_num, name, password)) {
        DBG("[CHECK_BALANCE] FAILED: auth failed");
        return Result<BankAccountBalance>::fail(ErrorCode::AUTH_FAILED);
    }
    DBG("[CHECK_BALANCE] SUCCESS: balance=" << acc.balance.value << " currency=" << acc.balance.currency);
    return Result<BankAccountBalance>::success(acc.balance);
}

Result<BankAccountBalance> AccountStore::transfer(int sender_account_num, const std::string &sender_name, const std::string &password, int receiver_account_num, const std::string &receiver_name, Currency currency, float amt)
{
    DBG("[TRANSFER] sender_account_num=" << sender_account_num << " sender_name=" << sender_name
        << " receiver_account_num=" << receiver_account_num << " receiver_name=" << receiver_name
        << " currency=" << currency << " amt=" << amt);
    auto it = accounts_.find(sender_account_num);
    if (it == accounts_.end()) {
        DBG("[TRANSFER] FAILED: sender account not found");
        return Result<BankAccountBalance>::fail(ErrorCode::ACCOUNT_NOT_FOUND);
    }
    BankAccount &sender_acc = it->second;

    if (!auth(sender_acc, sender_account_num, sender_name, password)) {
        DBG("[TRANSFER] FAILED: auth failed");
        return Result<BankAccountBalance>::fail(ErrorCode::AUTH_FAILED);
    }
    if (amt <= 0) {
        DBG("[TRANSFER] FAILED: invalid amount " << amt);
        return Result<BankAccountBalance>::fail(ErrorCode::INVALID_AMOUNT);
    }
    if (sender_acc.balance.currency != currency) {
        DBG("[TRANSFER] FAILED: sender currency mismatch (account=" << sender_acc.balance.currency << " requested=" << currency << ")");
        return Result<BankAccountBalance>::fail(ErrorCode::SENDER_CURRENCY_MISMATCH);
    }
    if (sender_acc.balance.value < amt) {
        DBG("[TRANSFER] FAILED: insufficient balance (balance=" << sender_acc.balance.value << " requested=" << amt << ")");
        return Result<BankAccountBalance>::fail(ErrorCode::INSUFFICIENT_BALANCE);
    }

    it = accounts_.find(receiver_account_num);
    if (it == accounts_.end()) {
        DBG("[TRANSFER] FAILED: receiver account not found");
        return Result<BankAccountBalance>::fail(ErrorCode::RECEIVER_ACCOUNT_NOT_FOUND);
    }
    BankAccount &receiver_acc = it->second;
    if (receiver_acc.holder_name != receiver_name) {
        DBG("[TRANSFER] FAILED: receiver name mismatch (expected=" << receiver_acc.holder_name << " got=" << receiver_name << ")");
        return Result<BankAccountBalance>::fail(ErrorCode::RECEIVER_NAME_MISMATCH);
    }
    if (receiver_acc.balance.currency != currency) {
        DBG("[TRANSFER] FAILED: receiver currency mismatch (account=" << receiver_acc.balance.currency << " requested=" << currency << ")");
        return Result<BankAccountBalance>::fail(ErrorCode::RECEIVER_CURRENCY_MISMATCH);
    }

    sender_acc.balance.value = sender_acc.balance.value - amt;
    receiver_acc.balance.value = receiver_acc.balance.value + amt;
    DBG("[TRANSFER] SUCCESS: sender new balance=" << sender_acc.balance.value
        << " receiver new balance=" << receiver_acc.balance.value);
    return Result<BankAccountBalance>::success(sender_acc.balance);

}
