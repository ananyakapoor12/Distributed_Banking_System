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

    out.msg_type = static_cast<MessageType>(read_byte(buf, offset, buf_len));

    if (out.msg_type != MessageType::REQUEST)
    {
        std::cerr << "parse_header: unexpected message type "
                  << (int)out.msg_type << std::endl;
        return false;
    }

    read_request_id(buf, offset, buf_len, out.request_id);
    out.opcode = static_cast<Opcode>(read_byte(buf, offset, buf_len));
    return true;
}

// ─── Per-opcode argument parsers ──────────────────────────────────────────────
// Each function assumes offset already points past the header (byte 18).

bool parse_open_account(const uint8_t* buf, int buf_len, int& offset, OpenAccountArgs& out)
{
    try {
        out.password = read_fixed_string(buf, offset, buf_len, PASSWORD_LEN);
        out.currency = read_byte(buf, offset, buf_len);
        out.balance  = read_float(buf, offset, buf_len);
        out.name     = read_string(buf, offset, buf_len);
    } catch (const std::out_of_range& e) {
        std::cerr << "parse_open_account: " << e.what() << std::endl;
        return false;
    }
    DBG("[OPEN_ACCOUNT] name=" << out.name
        << " password=" << out.password
        << " currency=" << (int)out.currency
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
    DBG("[CLOSE_ACCOUNT] account_num=" << out.account_num
        << " name=" << out.name
        << " password=" << out.password);
    return true;
}

bool parse_deposit(const uint8_t* buf, int buf_len, int& offset, DepositArgs& out)
{
    try {
        out.account_num = read_uint(buf, offset, buf_len);
        out.password    = read_fixed_string(buf, offset, buf_len, PASSWORD_LEN);
        out.currency    = read_byte(buf, offset, buf_len);
        out.amount      = read_float(buf, offset, buf_len);
        out.name        = read_string(buf, offset, buf_len);
    } catch (const std::out_of_range& e) {
        std::cerr << "parse_deposit: " << e.what() << std::endl;
        return false;
    }
    DBG("[DEPOSIT] account_num=" << out.account_num
        << " name=" << out.name
        << " password=" << out.password
        << " currency=" << (int)out.currency
        << " amount=" << out.amount);
    return true;
}

bool parse_withdraw(const uint8_t* buf, int buf_len, int& offset, WithdrawArgs& out)
{
    try {
        out.account_num = read_uint(buf, offset, buf_len);
        out.password    = read_fixed_string(buf, offset, buf_len, PASSWORD_LEN);
        out.currency    = read_byte(buf, offset, buf_len);
        out.amount      = read_float(buf, offset, buf_len);
        out.name        = read_string(buf, offset, buf_len);
    } catch (const std::out_of_range& e) {
        std::cerr << "parse_withdraw: " << e.what() << std::endl;
        return false;
    }
    DBG("[WITHDRAW] account_num=" << out.account_num
        << " name=" << out.name
        << " password=" << out.password
        << " currency=" << (int)out.currency
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
    DBG("[MONITOR] duration=" << out.duration << "s");
    return true;
}

bool parse_transfer(const uint8_t* buf, int buf_len, int& offset, TransferArgs& out)
{
    try {
        out.sender_account_num   = read_uint(buf, offset, buf_len);
        out.sender_password      = read_fixed_string(buf, offset, buf_len, PASSWORD_LEN);
        out.receiver_account_num = read_uint(buf, offset, buf_len);
        out.amount               = read_float(buf, offset, buf_len);
        out.currency             = read_byte(buf, offset, buf_len);
        out.sender_name          = read_string(buf, offset, buf_len);
        out.receiver_name        = read_string(buf, offset, buf_len);
    } catch (const std::out_of_range& e) {
        std::cerr << "parse_transfer: " << e.what() << std::endl;
        return false;
    }
    DBG("[TRANSFER] sender_account_num=" << out.sender_account_num
        << " sender_name=" << out.sender_name
        << " sender_password=" << out.sender_password
        << " receiver_account_num=" << out.receiver_account_num
        << " currency=" << (int)out.currency
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
    DBG("[CHECK_BALANCE] account_num=" << out.account_num
        << " name=" << out.name
        << " password=" << out.password);
    return true;
}
