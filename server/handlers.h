#pragma once

#include <cstdint>
#include <string>
#include "protocol.h"

struct RequestHeader {
    MessageType msg_type;
    uint8_t request_id[REQUEST_ID_LEN];
    Opcode opcode;
};

struct OpenAccountArgs {
    std::string name;
    std::string password;
    uint8_t currency;
    float balance;
};

struct CloseAccountArgs {
    uint32_t account_num;
    std::string name;
    std::string password;
};

struct DepositArgs {
    uint32_t account_num;
    std::string name;
    std::string password;
    uint8_t currency;
    float amount;
};

struct WithdrawArgs {
    uint32_t account_num;
    std::string name;
    std::string password;
    uint8_t currency;
    float amount;
};

struct MonitorArgs {
    uint32_t duration;   // seconds
};

struct TransferArgs {
    uint32_t sender_account_num;
    std::string sender_name;
    std::string receiver_name;
    std::string sender_password;
    uint32_t receiver_account_num;
    uint8_t currency;
    float amount;
};

struct CheckBalanceArgs {
    uint32_t account_num;
    std::string name;
    std::string password;
};

bool parse_header(const uint8_t* buf, int buf_len, int& offset, RequestHeader& out);
bool parse_open_account(const uint8_t* buf, int buf_len, int& offset, OpenAccountArgs& out);
bool parse_close_account(const uint8_t* buf, int buf_len, int& offset, CloseAccountArgs& out);
bool parse_deposit(const uint8_t* buf, int buf_len, int& offset, DepositArgs& out);
bool parse_withdraw(const uint8_t* buf, int buf_len, int& offset, WithdrawArgs& out);
bool parse_monitor(const uint8_t* buf, int buf_len, int& offset, MonitorArgs& out);
bool parse_transfer(const uint8_t* buf, int buf_len, int& offset, TransferArgs& out);
bool parse_check_balance(const uint8_t* buf, int buf_len, int& offset, CheckBalanceArgs& out);
