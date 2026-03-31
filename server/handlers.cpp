#include "handlers.h"
#include "marshaller.h"
#include "protocol.h"

#include <cstring>   // memcmp
#include <iostream>
#include <stdexcept>

#ifdef DEBUG
#define DBG(x) std::cout << x << std::endl
#else
#define DBG(x)
#endif

// Reads the 18-byte fixed header from buf, advances offset past it.
// Returns false if the message type is not REQUEST or the buffer is too short.
bool parse_header(const uint8_t* buf, int buf_len, int& offset, RequestHeader& out)
{
    if (buf_len < (int)HEADER_SIZE)
    {
        std::cerr << "parse_header: packet too short ("
                  << buf_len << " bytes)" << std::endl;
        return false;
    }

    uint8_t raw_msg_type = read_byte(buf, offset, buf_len);
    if (raw_msg_type > 2)
    {
        std::cerr << "parse_header: unknown message type " << (int)raw_msg_type << std::endl;
        return false;
    }
    out.msg_type = static_cast<MessageType>(raw_msg_type);

    if (out.msg_type != MessageType::REQUEST)
    {
        std::cerr << "parse_header: unexpected message type "
                  << (int)raw_msg_type << std::endl;
        return false;
    }

    read_request_id(buf, offset, buf_len, out.request_id);
    uint8_t raw_opcode = read_byte(buf, offset, buf_len);
    if (raw_opcode < 1 || raw_opcode > 7)
    {
        std::cerr << "parse_header: unknown opcode " << (int)raw_opcode << std::endl;
        return false;
    }
    out.opcode = static_cast<Opcode>(raw_opcode);
    return true;
}

// ─── Per-opcode argument parsers ──────────────────────────────────────────────
// Each function assumes offset already points past the header (byte 18).

bool parse_open_account(const uint8_t* buf, int buf_len, int& offset, OpenAccountArgs& out)
{
    try {
        out.password = read_fixed_string(buf, offset, buf_len, PASSWORD_LEN);
        uint8_t raw_currency = read_byte(buf, offset, buf_len);
        if (raw_currency > 8) throw std::out_of_range("invalid currency: " + std::to_string(raw_currency));
        out.currency = static_cast<Currency>(raw_currency);
        out.balance  = read_float(buf, offset, buf_len);
        out.name     = read_string(buf, offset, buf_len);
    } catch (const std::out_of_range& e) {
        std::cerr << "parse_open_account: " << e.what() << std::endl;
        return false;
    }
    DBG("[OPEN_ACCOUNT PARSER] name=" << out.name
        << " password=" << out.password
        << " currency=" << out.currency
        << " balance=" << out.balance);
    return true;
}

bool parse_close_account(const uint8_t* buf, int buf_len, int& offset, CloseAccountArgs& out)
{
    try {
        out.account_num = read_uint(buf, offset, buf_len);
        out.password    = read_fixed_string(buf, offset, buf_len, PASSWORD_LEN);
        out.name        = read_string(buf, offset, buf_len);
    } catch (const std::out_of_range& e) {
        std::cerr << "parse_close_account: " << e.what() << std::endl;
        return false;
    }
    DBG("[CLOSE_ACCOUNT PARSER] account_num=" << out.account_num
        << " name=" << out.name
        << " password=" << out.password);
    return true;
}

bool parse_deposit(const uint8_t* buf, int buf_len, int& offset, DepositArgs& out)
{
    try {
        out.account_num = read_uint(buf, offset, buf_len);
        out.password    = read_fixed_string(buf, offset, buf_len, PASSWORD_LEN);
        uint8_t raw_currency = read_byte(buf, offset, buf_len);
        if (raw_currency > 8) throw std::out_of_range("invalid currency: " + std::to_string(raw_currency));
        out.currency    = static_cast<Currency>(raw_currency);
        out.amount      = read_float(buf, offset, buf_len);
        out.name        = read_string(buf, offset, buf_len);
    } catch (const std::out_of_range& e) {
        std::cerr << "parse_deposit: " << e.what() << std::endl;
        return false;
    }
    DBG("[DEPOSIT PARSER] account_num=" << out.account_num
        << " name=" << out.name
        << " password=" << out.password
        << " currency=" << out.currency
        << " amount=" << out.amount);
    return true;
}

bool parse_withdraw(const uint8_t* buf, int buf_len, int& offset, WithdrawArgs& out)
{
    try {
        out.account_num = read_uint(buf, offset, buf_len);
        out.password    = read_fixed_string(buf, offset, buf_len, PASSWORD_LEN);
        uint8_t raw_currency = read_byte(buf, offset, buf_len);
        if (raw_currency > 8) throw std::out_of_range("invalid currency: " + std::to_string(raw_currency));
        out.currency    = static_cast<Currency>(raw_currency);
        out.amount      = read_float(buf, offset, buf_len);
        out.name        = read_string(buf, offset, buf_len);
    } catch (const std::out_of_range& e) {
        std::cerr << "parse_withdraw: " << e.what() << std::endl;
        return false;
    }
    DBG("[WITHDRAW PARSER] account_num=" << out.account_num
        << " name=" << out.name
        << " password=" << out.password
        << " currency=" << out.currency
        << " amount=" << out.amount);
    return true;
}

bool parse_monitor(const uint8_t* buf, int buf_len, int& offset, MonitorArgs& out)
{
    try {
        out.duration = read_uint(buf, offset, buf_len);
    } catch (const std::out_of_range& e) {
        std::cerr << "parse_monitor: " << e.what() << std::endl;
        return false;
    }
    DBG("[MONITOR PARSER] duration=" << out.duration << "s");
    return true;
}

bool parse_transfer(const uint8_t* buf, int buf_len, int& offset, TransferArgs& out)
{
    try {
        out.sender_account_num   = read_uint(buf, offset, buf_len);
        out.sender_password      = read_fixed_string(buf, offset, buf_len, PASSWORD_LEN);
        out.receiver_account_num = read_uint(buf, offset, buf_len);
        out.amount               = read_float(buf, offset, buf_len);
        uint8_t raw_currency = read_byte(buf, offset, buf_len);
        if (raw_currency > 8) throw std::out_of_range("invalid currency: " + std::to_string(raw_currency));
        out.currency             = static_cast<Currency>(raw_currency);
        out.sender_name          = read_string(buf, offset, buf_len);
        out.receiver_name        = read_string(buf, offset, buf_len);
    } catch (const std::out_of_range& e) {
        std::cerr << "parse_transfer: " << e.what() << std::endl;
        return false;
    }
    DBG("[TRANSFER PARSER] sender_account_num=" << out.sender_account_num
        << " sender_name=" << out.sender_name
        << " sender_password=" << out.sender_password
        << " receiver_account_num=" << out.receiver_account_num
        << " currency=" << out.currency
        << " amount=" << out.amount);
    return true;
}

bool parse_check_balance(const uint8_t* buf, int buf_len, int& offset, CheckBalanceArgs& out)
{
    try {
        out.account_num = read_uint(buf, offset, buf_len);
        out.password    = read_fixed_string(buf, offset, buf_len, PASSWORD_LEN);
        out.name        = read_string(buf, offset, buf_len);
    } catch (const std::out_of_range& e) {
        std::cerr << "parse_check_balance: " << e.what() << std::endl;
        return false;
    }
    DBG("[CHECK_BALANCE PARSER] account_num=" << out.account_num
        << " name=" << out.name
        << " password=" << out.password);
    return true;
}
