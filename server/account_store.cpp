#include "account_store.h"
#include <iostream>

Result<int> AccountStore::open_account(const std::string &name, const std::string &password, int currency, float balance)
{
    std::cout << "[OPEN_ACCOUNT] name=" << name << " currency=" << currency << " balance=" << balance << std::endl;
    if (!is_valid_currency(currency)) {
        std::cout << "[OPEN_ACCOUNT] FAILED: invalid currency " << currency << std::endl;
        return Result<int>::fail(ErrorCode::INVALID_CURRENCY);
    }
    if (balance <= 0) {
        std::cout << "[OPEN_ACCOUNT] FAILED: invalid amount " << balance << std::endl;
        return Result<int>::fail(ErrorCode::INVALID_AMOUNT);
    }

    int id = next_id_++;
    accounts_[id] = BankAccount{id, name, password, BankAccountBalance{currency, balance}};
    std::cout << "[OPEN_ACCOUNT] SUCCESS: account_num=" << id << std::endl;
    return  Result<int>::success(id);
}

bool AccountStore::auth(BankAccount acc, int account_num, const std::string &name, const std::string &password) const
{
    std::cout << "[AUTH] account_num=" << acc.account_number
              << " holder_name=" << acc.holder_name
              << " passwd=" << acc.password
              << " currency=" << (int)acc.balance.currency
              << " balance=" << acc.balance.value << std::endl;
    return acc.holder_name == name && acc.password == password;
}

Result<bool> AccountStore::close_account(int account_num, const std::string &name, const std::string &password)
{
    std::cout << "[CLOSE_ACCOUNT] account_num=" << account_num << " name=" << name << std::endl;
    auto it = accounts_.find(account_num);
    if (it == accounts_.end()) {
        std::cout << "[CLOSE_ACCOUNT] FAILED: account not found" << std::endl;
        return Result<bool>::fail(ErrorCode::ACCOUNT_NOT_FOUND);
    }
    const BankAccount &acc = it->second;
    if (!auth(acc, account_num, name, password)) {
        std::cout << "[CLOSE_ACCOUNT] FAILED: auth failed" << std::endl;
        return Result<bool>::fail(ErrorCode::AUTH_FAILED);
    }
    accounts_.erase(account_num);
    std::cout << "[CLOSE_ACCOUNT] SUCCESS: account_num=" << account_num << " closed" << std::endl;
    return Result<bool>::success(true);
}

Result<BankAccountBalance> AccountStore::deposit(int account_num, const std::string &name, const std::string &password, int currency, float amt)
{
    std::cout << "[DEPOSIT] account_num=" << account_num << " name=" << name << " currency=" << currency << " amt=" << amt << std::endl;
    auto it = accounts_.find(account_num);
    if (it == accounts_.end()) {
        std::cout << "[DEPOSIT] FAILED: account not found" << std::endl;
        return Result<BankAccountBalance>::fail(ErrorCode::ACCOUNT_NOT_FOUND);
    }
    BankAccount &acc = it->second;
    if (!auth(acc, account_num, name, password)) {
        std::cout << "[DEPOSIT] FAILED: auth failed" << std::endl;
        return Result<BankAccountBalance>::fail(ErrorCode::AUTH_FAILED);
    }
    if (amt <= 0) {
        std::cout << "[DEPOSIT] FAILED: invalid amount " << amt << std::endl;
        return Result<BankAccountBalance>::fail(ErrorCode::INVALID_AMOUNT);
    }
    if (acc.balance.currency == currency){
        acc.balance.value = acc.balance.value + amt;
        std::cout << "[DEPOSIT] SUCCESS: new balance=" << acc.balance.value << std::endl;
        return Result<BankAccountBalance>::success(acc.balance);
    }
    else {
        std::cout << "[DEPOSIT] FAILED: currency mismatch (account=" << acc.balance.currency << " requested=" << currency << ")" << std::endl;
        return Result<BankAccountBalance>::fail(ErrorCode::CURRENCY_MISMATCH);
    }
}

Result<BankAccountBalance> AccountStore::withdraw(int account_num, const std::string &name, const std::string &password, int currency, float amt)
{
    std::cout << "[WITHDRAW] account_num=" << account_num << " name=" << name << " currency=" << currency << " amt=" << amt << std::endl;
    auto it = accounts_.find(account_num);
    if (it == accounts_.end()) {
        std::cout << "[WITHDRAW] FAILED: account not found" << std::endl;
        return Result<BankAccountBalance>::fail(ErrorCode::ACCOUNT_NOT_FOUND);
    }
    BankAccount &acc = it->second;
    if (!auth(acc, account_num, name, password)) {
        std::cout << "[WITHDRAW] FAILED: auth failed" << std::endl;
        return Result<BankAccountBalance>::fail(ErrorCode::AUTH_FAILED);
    }
    if (amt <= 0) {
        std::cout << "[WITHDRAW] FAILED: invalid amount " << amt << std::endl;
        return Result<BankAccountBalance>::fail(ErrorCode::INVALID_AMOUNT);
    }
    if (acc.balance.currency == currency)
    {
        if (acc.balance.value >= amt){
            acc.balance.value = acc.balance.value - amt;
            std::cout << "[WITHDRAW] SUCCESS: new balance=" << acc.balance.value << std::endl;
            return Result<BankAccountBalance>::success(acc.balance);
        }
        else {
            std::cout << "[WITHDRAW] FAILED: insufficient balance (balance=" << acc.balance.value << " requested=" << amt << ")" << std::endl;
            return Result<BankAccountBalance>::fail(ErrorCode::INSUFFICIENT_BALANCE);
        }
    }
    else {
        std::cout << "[WITHDRAW] FAILED: currency mismatch (account=" << acc.balance.currency << " requested=" << currency << ")" << std::endl;
        return Result<BankAccountBalance>::fail(ErrorCode::CURRENCY_MISMATCH);
    }
}

Result<BankAccountBalance> AccountStore::check_balance(int account_num, const std::string &name, const std::string &password)
{
    std::cout << "[CHECK_BALANCE] account_num=" << account_num << " name=" << name << std::endl;
    auto it = accounts_.find(account_num);
    if (it == accounts_.end()) {
        std::cout << "[CHECK_BALANCE] FAILED: account not found" << std::endl;
        return Result<BankAccountBalance>::fail(ErrorCode::ACCOUNT_NOT_FOUND);
    }
    const BankAccount &acc = it->second;
    if (!auth(acc, account_num, name, password)) {
        std::cout << "[CHECK_BALANCE] FAILED: auth failed" << std::endl;
        return Result<BankAccountBalance>::fail(ErrorCode::AUTH_FAILED);
    }
    std::cout << "[CHECK_BALANCE] SUCCESS: balance=" << acc.balance.value << " currency=" << acc.balance.currency << std::endl;
    return Result<BankAccountBalance>::success(acc.balance);
}

Result<BankAccountBalance> AccountStore::transfer(int sender_account_num, const std::string &sender_name, const std::string &password, int receiver_account_num, const std::string &receiver_name, int currency, float amt)
{
    std::cout << "[TRANSFER] sender_account_num=" << sender_account_num << " sender_name=" << sender_name
              << " receiver_account_num=" << receiver_account_num << " receiver_name=" << receiver_name
              << " currency=" << currency << " amt=" << amt << std::endl;
    auto it = accounts_.find(sender_account_num);
    if (it == accounts_.end()) {
        std::cout << "[TRANSFER] FAILED: sender account not found" << std::endl;
        return Result<BankAccountBalance>::fail(ErrorCode::ACCOUNT_NOT_FOUND);
    }
    BankAccount &sender_acc = it->second;

    if (!auth(sender_acc, sender_account_num, sender_name, password)) {
        std::cout << "[TRANSFER] FAILED: auth failed" << std::endl;
        return Result<BankAccountBalance>::fail(ErrorCode::AUTH_FAILED);
    }
    if (amt <= 0) {
        std::cout << "[TRANSFER] FAILED: invalid amount " << amt << std::endl;
        return Result<BankAccountBalance>::fail(ErrorCode::INVALID_AMOUNT);
    }
    if (sender_acc.balance.currency != currency) {
        std::cout << "[TRANSFER] FAILED: sender currency mismatch (account=" << sender_acc.balance.currency << " requested=" << currency << ")" << std::endl;
        return Result<BankAccountBalance>::fail(ErrorCode::SENDER_CURRENCY_MISMATCH);
    }
    if (sender_acc.balance.value < amt)
        return Result<BankAccountBalance>::fail(ErrorCode::INSUFFICIENT_BALANCE);

    it = accounts_.find(receiver_account_num);
    if (it == accounts_.end()) {
        std::cout << "[TRANSFER] FAILED: receiver account not found" << std::endl;
        return Result<BankAccountBalance>::fail(ErrorCode::RECEIVER_ACCOUNT_NOT_FOUND);
    }
    BankAccount &receiver_acc = it->second;
    if (receiver_acc.holder_name != receiver_name) {
        std::cout << "[TRANSFER] FAILED: receiver name mismatch (expected=" << receiver_acc.holder_name << " got=" << receiver_name << ")" << std::endl;
        return Result<BankAccountBalance>::fail(ErrorCode::RECEIVER_NAME_MISMATCH);
    }
    if (receiver_acc.balance.currency != currency) {
        std::cout << "[TRANSFER] FAILED: receiver currency mismatch (account=" << receiver_acc.balance.currency << " requested=" << currency << ")" << std::endl;
        return Result<BankAccountBalance>::fail(ErrorCode::RECEIVER_CURRENCY_MISMATCH);
    }

    sender_acc.balance.value = sender_acc.balance.value - amt;
    receiver_acc.balance.value = receiver_acc.balance.value + amt;
    std::cout << "[TRANSFER] SUCCESS: sender new balance=" << sender_acc.balance.value
              << " receiver new balance=" << receiver_acc.balance.value << std::endl;
    return Result<BankAccountBalance>::success(sender_acc.balance);

}
