#include "handlers.h"
#include "marshaller.h"
#include "protocol.h"

#include <cstring>   // memcmp
#include <iostream>

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

    out.msg_type = static_cast<MessageType>(read_byte(buf, offset));

    if (out.msg_type != MessageType::REQUEST)
    {
        std::cerr << "parse_header: unexpected message type "
                  << (int)out.msg_type << std::endl;
        return false;
    }

    read_request_id(buf, offset, out.request_id);
    out.opcode = static_cast<Opcode>(read_byte(buf, offset));
    return true;
}

// ─── Per-opcode argument parsers ──────────────────────────────────────────────
// Each function assumes offset already points past the header (byte 18).

bool parse_open_account(const uint8_t* buf, int buf_len, int& offset, OpenAccountArgs& out)
{
    out.password = read_fixed_string(buf, offset, PASSWORD_LEN);
    out.currency = read_byte(buf, offset);
    out.balance  = read_float(buf, offset);
    out.name = read_string(buf, offset);
    std::cout << "[OPEN_ACCOUNT] name=" << out.name
              << " password=" << out.password
              << " currency=" << (int)out.currency
              << " balance=" << out.balance << std::endl;
    return true;
}

bool parse_close_account(const uint8_t* buf, int buf_len, int& offset, CloseAccountArgs& out)
{
    out.account_num = read_uint(buf, offset);
    out.password = read_fixed_string(buf, offset, PASSWORD_LEN);
    out.name        = read_string(buf, offset);
    std::cout << "[CLOSE_ACCOUNT] account_num=" << out.account_num
              << " name=" << out.name
              << " password=" << out.password << std::endl;
    return true;
}

bool parse_deposit(const uint8_t* buf, int buf_len, int& offset, DepositArgs& out)
{
    out.account_num = read_uint(buf, offset);
    out.password    = read_fixed_string(buf, offset, PASSWORD_LEN);
    out.currency    = read_byte(buf, offset);
    out.amount      = read_float(buf, offset);
    out.name = read_string(buf, offset);

    std::cout << "[DEPOSIT] account_num=" << out.account_num
              << " name=" << out.name
              << " password=" << out.password
              << " currency=" << (int)out.currency
              << " amount=" << out.amount << std::endl;
    return true;
}

bool parse_withdraw(const uint8_t* buf, int buf_len, int& offset, WithdrawArgs& out)
{
    out.account_num = read_uint(buf, offset);
    out.password    = read_fixed_string(buf, offset, PASSWORD_LEN);
    out.currency    = read_byte(buf, offset);
    out.amount      = read_float(buf, offset);
    out.name = read_string(buf, offset);

    std::cout << "[WITHDRAW] account_num=" << out.account_num
              << " name=" << out.name
              << " password=" << out.password
              << " currency=" << (int)out.currency
              << " amount=" << out.amount << std::endl;
    return true;
}

bool parse_monitor(const uint8_t* buf, int buf_len, int& offset, MonitorArgs& out)
{
    out.duration = read_uint(buf, offset);
    std::cout << "[MONITOR] duration=" << out.duration << "s" << std::endl;
    return true;
}

bool parse_transfer(const uint8_t* buf, int buf_len, int& offset, TransferArgs& out)
{
    out.sender_account_num = read_uint(buf, offset);
    out.sender_password = read_fixed_string(buf, offset, PASSWORD_LEN);
    out.receiver_account_num = read_uint(buf, offset);
    out.amount = read_float(buf, offset);
    out.currency = read_byte(buf, offset);
    out.sender_name = read_string(buf, offset);
    out.receiver_name = read_string(buf, offset);

    std::cout << "[TRANSFER] sender_account_num=" << out.sender_account_num
              << " sender_name=" << out.sender_name
              << " sender_password=" << out.sender_password
              << " receiver_account_num=" << out.receiver_account_num
              << " currency=" << (int)out.currency
              << " amount=" << out.amount << std::endl;
    return true;
}

bool parse_check_balance(const uint8_t* buf, int buf_len, int& offset, CheckBalanceArgs& out)
{
    out.account_num = read_uint(buf, offset);
    out.password = read_fixed_string(buf, offset, PASSWORD_LEN);
    out.name        = read_string(buf, offset);

    std::cout << "[CHECK_BALANCE] account_num=" << out.account_num
              << " name=" << out.name
              << " password=" << out.password << std::endl;
    return true;
}
