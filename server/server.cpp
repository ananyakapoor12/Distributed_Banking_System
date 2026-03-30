#include <iostream>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <ctime>

#include "udp_socket.cpp"   // included directly since UDPSocket is defined as a class inside the .cpp
#include "handlers.h"
#include "marshaller.h"
#include "account_store.h"

#ifdef DEBUG
#define DBG(x) std::cout << x << std::endl
#else
#define DBG(x)
#endif


int main(int argc, char* argv[])
{
    int port = 8014;
    if (argc >= 2) port = std::atoi(argv[1]);
    AccountStore bank;

    DBG("Starting server on port " << port);

    UDPSocket sock(port);

    // maintain cache for past requests (at-most-once): ip -> (request_id -> reply bytes)
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> prevRequestData;

    // monitoring clients: ip -> {sockaddr_in, expiry time}
    struct MonitorEntry {
        sockaddr_in addr;
        std::chrono::steady_clock::time_point expiry;
    };

    std::unordered_map<std::string, MonitorEntry> monitorClients;

    auto notify_monitors = [&](Opcode trigger, int account_num,
                               const std::string &holder_name,
                               float balance, int currency,
                               const std::string &description)
    {
        auto now = std::chrono::steady_clock::now();
        uint8_t cb[MAX_BUF_SIZE];
        int cb_off = 0;
        write_byte(cb, cb_off, (uint8_t)MessageType::CALLBACK);
        write_byte(cb, cb_off, (uint8_t)trigger);
        write_uint(cb, cb_off, (uint32_t)account_num);
        write_string(cb, cb_off, holder_name);
        write_float(cb, cb_off, balance);
        write_byte(cb, cb_off, (uint8_t)currency);
        write_string(cb, cb_off, description);

        for (auto it = monitorClients.begin(); it != monitorClients.end();)
        {
            if (it->second.expiry <= now)
            {
                std::time_t now_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                std::string now_str(std::ctime(&now_t));
                now_str.pop_back();
                DBG("[MONITOR] Expired: " << it->first << " at " << now_str);
                it = monitorClients.erase(it);
                continue;
            }
            sock.send_to(cb, cb_off, it->second.addr);
            DBG("[MONITOR] Notified " << it->first);
            ++it;
        }
    };

    while(true){


        // ── Receive one packet ────────────────────────────────────────────────────
        uint8_t buf[MAX_BUF_SIZE];
        sockaddr_in client_addr;

        // std::cout << "Waiting for a packet..." << std::endl;
        ssize_t req_buf_len = sock.receive(buf, sizeof(buf), client_addr);
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), ipStr, INET_ADDRSTRLEN);

        if (req_buf_len < 0)
        {
            std::cerr << "Failed to receive packet." << std::endl;
            return 1;
        }

        // parse header and get the opcode
        if (req_buf_len < (ssize_t)HEADER_SIZE) {
            std::cerr << "Packet too short (" << req_buf_len << " bytes), dropping." << std::endl;
            continue;
        }
        int offset = 0;
        MessageType msg_type = static_cast<MessageType>(buf[offset++]);
        uint8_t request_id[REQUEST_ID_LEN];
        memcpy(request_id, buf + offset, REQUEST_ID_LEN);

        // check cache for requestID
        std::string req_id_key(reinterpret_cast<const char*>(request_id), REQUEST_ID_LEN);

        bool cache_hit = false;
        auto ip_it = prevRequestData.find(std::string(ipStr));
        if (ip_it != prevRequestData.end()) {
            auto cached = ip_it->second.find(req_id_key);
            if (cached != ip_it->second.end()) {
                const std::string& cached_reply = cached->second;
                sock.send_to(reinterpret_cast<const uint8_t*>(cached_reply.data()), cached_reply.size(), client_addr);
#ifdef DEBUG
                std::cout << "[CACHE HIT] ip=" << ipStr << " bytes=";
                for (unsigned char c : cached_reply) std::cout << std::hex << (int)c << " ";
                std::cout << std::dec << std::endl;
#endif
                cache_hit = true;
            }
        }
        if (cache_hit) continue;


        offset += REQUEST_ID_LEN;
        Opcode opcode = static_cast<Opcode>(buf[offset++]);
        
        // parse and execute the request
        switch (opcode) {
            case Opcode::OPEN_ACCOUNT: { 
                OpenAccountArgs args;
                bool succ = parse_open_account(buf, req_buf_len, offset, args);
                
                // response message header
                uint8_t reply[MAX_BUF_SIZE];
                int res_offset = 0;
                write_byte(reply, res_offset, (uint8_t)MessageType::RESPONSE);
                write_request_id(reply, res_offset, request_id);
                write_byte(reply, res_offset, (uint8_t)Opcode::OPEN_ACCOUNT);

                if (succ) {
                    Result<int> res = bank.open_account(args.name, args.password, args.currency, args.balance);
                    if (res.code == ErrorCode::SUCCESS){
                        write_byte(reply, res_offset, (uint8_t)Status::SUCCESS);
                        write_uint(reply, res_offset, (uint32_t)res.value);
                        notify_monitors(Opcode::OPEN_ACCOUNT, res.value, args.name, args.balance, args.currency, "New account opened.");
                    }
                    else if (res.code == ErrorCode::INVALID_CURRENCY)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::INVALID_CURRENCY);
                        write_string(reply, res_offset, "Invalid Currency chosen");
                    }
                    else if (res.code == ErrorCode::INVALID_AMOUNT)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::INVALID_AMOUNT);
                        write_string(reply, res_offset, "Invalid balance amount");
                    }
                    
                } else {
                    write_byte(reply, res_offset, (uint8_t)Status::MALFORMED_REQUEST);
                    write_string(reply, res_offset, "Request malformed. Content cannot be parsed.");
                }
                sock.send_to(reply, res_offset, client_addr);

                prevRequestData[std::string(ipStr)][req_id_key] = std::string(reinterpret_cast<char*>(reply), res_offset);
#ifdef DEBUG
                std::cout << "[CACHED] ip=" << ipStr << " bytes=";
                for (int i = 0; i < res_offset; i++) std::cout << std::hex << (int)reply[i] << " ";
                std::cout << std::dec << std::endl;
#endif

                break;
            }

            case Opcode::CLOSE_ACCOUNT: {
                CloseAccountArgs args;
                int succ = parse_close_account(buf, req_buf_len, offset, args);

                // response message header
                uint8_t reply[MAX_BUF_SIZE];
                int res_offset = 0;
                write_byte(reply, res_offset, (uint8_t)MessageType::RESPONSE);
                write_request_id(reply, res_offset, request_id);
                write_byte(reply, res_offset, (uint8_t)Opcode::CLOSE_ACCOUNT);

                if (succ)
                {
                    Result<BankAccountBalance> res = bank.close_account(args.account_num, args.name, args.password);
                    if (res.code == ErrorCode::SUCCESS) {
                        write_byte(reply, res_offset, (uint8_t)Status::SUCCESS);
                        notify_monitors(Opcode::CLOSE_ACCOUNT, args.account_num, args.name, res.value.value, res.value.currency, "Account closed");
                    } 
                    else if (res.code == ErrorCode::AUTH_FAILED) {
                        write_byte(reply, res_offset, (uint8_t)Status::AUTH_FAILED);
                        write_string(reply, res_offset, "Authentication failed: wrong name or password.");
                    }
                    else if (res.code == ErrorCode::ACCOUNT_NOT_FOUND)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::ACCOUNT_NOT_FOUND);
                        write_string(reply, res_offset, "Account does not exist.");
                    }
                }
                else
                {
                    write_byte(reply, res_offset, (uint8_t)Status::MALFORMED_REQUEST);
                    write_string(reply, res_offset, "Request malformed. Content cannot be parsed.");
                }
                sock.send_to(reply, res_offset, client_addr);

                prevRequestData[std::string(ipStr)][req_id_key] = std::string(reinterpret_cast<char*>(reply), res_offset);
#ifdef DEBUG
                std::cout << "[CACHED] ip=" << ipStr << " bytes=";
                for (int i = 0; i < res_offset; i++) std::cout << std::hex << (int)reply[i] << " ";
                std::cout << std::dec << std::endl;
#endif

                break;
            }

            case Opcode::DEPOSIT: {
                DepositArgs args;
                int succ = parse_deposit(buf, req_buf_len, offset, args);

                // response message header
                uint8_t reply[MAX_BUF_SIZE];
                int res_offset = 0;
                write_byte(reply, res_offset, (uint8_t)MessageType::RESPONSE);
                write_request_id(reply, res_offset, request_id);
                write_byte(reply, res_offset, (uint8_t)Opcode::DEPOSIT);

                if (succ)
                {
                    Result<BankAccountBalance> res = bank.deposit(args.account_num, args.name, args.password, args.currency, args.amount);
                    if (res.code == ErrorCode::SUCCESS)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::SUCCESS);
                        write_float(reply, res_offset, res.value.value);
                        write_byte(reply, res_offset, (uint8_t)res.value.currency);
                        notify_monitors(Opcode::DEPOSIT, args.account_num, args.name, res.value.value, res.value.currency, "Deposit complete.");
                    }
                    else if (res.code == ErrorCode::AUTH_FAILED)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::AUTH_FAILED);
                        write_string(reply, res_offset, "Authentication failed: wrong name or password.");
                    }
                    else if (res.code == ErrorCode::ACCOUNT_NOT_FOUND)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::ACCOUNT_NOT_FOUND);
                        write_string(reply, res_offset, "Account does not exist.");
                    }
                    else if (res.code == ErrorCode::CURRENCY_MISMATCH)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::CURRENCY_MISMATCH);
                        write_string(reply, res_offset, "This currency does not match the currency of the bank account");
                    }
                    else if (res.code == ErrorCode::INVALID_AMOUNT)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::INVALID_AMOUNT);
                        write_string(reply, res_offset, "Depositing an invalid amount. Cannot deposit amounts less than or equal to zero.");
                    }
                }
                else
                {
                    write_byte(reply, res_offset, (uint8_t)Status::MALFORMED_REQUEST);
                    write_string(reply, res_offset, "Request malformed. Content cannot be parsed.");
                }
                sock.send_to(reply, res_offset, client_addr);
                
                prevRequestData[std::string(ipStr)][req_id_key] = std::string(reinterpret_cast<char*>(reply), res_offset);
#ifdef DEBUG
                std::cout << "[CACHED] ip=" << ipStr << " bytes=";
                for (int i = 0; i < res_offset; i++) std::cout << std::hex << (int)reply[i] << " ";
                std::cout << std::dec << std::endl;
#endif
                
                break;
            }

            case Opcode::WITHDRAW: {
                WithdrawArgs args;
                int succ = parse_withdraw(buf, req_buf_len, offset, args);

                // response message header
                uint8_t reply[MAX_BUF_SIZE];
                int res_offset = 0;
                write_byte(reply, res_offset, (uint8_t)MessageType::RESPONSE);
                write_request_id(reply, res_offset, request_id);
                write_byte(reply, res_offset, (uint8_t)Opcode::WITHDRAW);

                if (succ)
                {
                    Result<BankAccountBalance> res = bank.withdraw(args.account_num, args.name, args.password, args.currency, args.amount);
                    if (res.code == ErrorCode::SUCCESS)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::SUCCESS);
                        write_float(reply, res_offset, res.value.value);
                        write_byte(reply, res_offset, (uint8_t)res.value.currency);
                        notify_monitors(Opcode::WITHDRAW, args.account_num, args.name, res.value.value, res.value.currency, "Withdrawal complete.");
                    }
                    else if (res.code == ErrorCode::AUTH_FAILED)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::AUTH_FAILED);
                        write_string(reply, res_offset, "Authentication failed: wrong name or password.");
                    }
                    else if (res.code == ErrorCode::ACCOUNT_NOT_FOUND)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::ACCOUNT_NOT_FOUND);
                        write_string(reply, res_offset, "Account does not exist.");
                    }
                    else if (res.code == ErrorCode::CURRENCY_MISMATCH)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::CURRENCY_MISMATCH);
                        write_string(reply, res_offset, "This currency does not match the currency of the bank account");
                    }
                    else if (res.code == ErrorCode::INVALID_AMOUNT)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::INVALID_AMOUNT);
                        write_string(reply, res_offset, "Withdrawing an invalid amount. Cannot withdraw amounts less than or equal to zero.");
                    }
                    else if (res.code == ErrorCode::INSUFFICIENT_BALANCE)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::INSUFFICIENT_BALANCE);
                        write_string(reply, res_offset, "Cannot withdraw amount. Insufficient balance in the account.");
                    }
                }
                else
                {
                    write_byte(reply, res_offset, (uint8_t)Status::MALFORMED_REQUEST);
                    write_string(reply, res_offset, "Request malformed. Content cannot be parsed.");
                }
                sock.send_to(reply, res_offset, client_addr);
                
                prevRequestData[std::string(ipStr)][req_id_key] = std::string(reinterpret_cast<char*>(reply), res_offset);
#ifdef DEBUG
                std::cout << "[CACHED] ip=" << ipStr << " bytes=";
                for (int i = 0; i < res_offset; i++) std::cout << std::hex << (int)reply[i] << " ";
                std::cout << std::dec << std::endl;
#endif
                
                break;
            }

            case Opcode::MONITOR: {
                MonitorArgs args;
                int succ = parse_monitor(buf, req_buf_len, offset, args);

                // response message header
                uint8_t reply[MAX_BUF_SIZE];
                int res_offset = 0;
                write_byte(reply, res_offset, (uint8_t)MessageType::RESPONSE);
                write_request_id(reply, res_offset, request_id);
                write_byte(reply, res_offset, (uint8_t)Opcode::MONITOR);
                if (succ)
                {
                    if (args.duration <= 0){
                        write_byte(reply, res_offset, (uint8_t)Status::MONITOR_INTERVAL_INVALID);
                        write_string(reply, res_offset, "Monitor interval cannot be less than or equal to zero.");
                    } else {
                        monitorClients[std::string(ipStr)] = {
                            client_addr,
                            std::chrono::steady_clock::now() + std::chrono::seconds(args.duration)
                        };
                        auto start_sys  = std::chrono::system_clock::now();
                        std::time_t start_t  = std::chrono::system_clock::to_time_t(start_sys);
                        std::string start_str(std::ctime(&start_t));
                        start_str.pop_back();   // remove trailing newline added by ctime
                        DBG("[MONITOR] Registered " << ipStr << ":"
                            << ntohs(client_addr.sin_port)
                            << " for " << args.duration << "s"
                            << " | start=" << start_str);
                        write_byte(reply, res_offset, (uint8_t)Status::SUCCESS);
                    }
                }
                else
                {
                    write_byte(reply, res_offset, (uint8_t)Status::MALFORMED_REQUEST);
                    write_string(reply, res_offset, "Request malformed. Content cannot be parsed.");
                }
                sock.send_to(reply, res_offset, client_addr);
                
                prevRequestData[std::string(ipStr)][req_id_key] = std::string(reinterpret_cast<char*>(reply), res_offset);
                
                break;
            }

            case Opcode::TRANSFER: {
                TransferArgs args;
                int succ = parse_transfer(buf, req_buf_len, offset, args);

                // response message header
                uint8_t reply[MAX_BUF_SIZE];
                int res_offset = 0;
                write_byte(reply, res_offset, (uint8_t)MessageType::RESPONSE);
                write_request_id(reply, res_offset, request_id);
                write_byte(reply, res_offset, (uint8_t)Opcode::TRANSFER);
                if (succ)
                {
                    Result<BankAccountBalance> res = bank.transfer(args.sender_account_num, args.sender_name, args.sender_password, args.receiver_account_num, args.receiver_name, args.currency, args.amount);
                    if (res.code == ErrorCode::SUCCESS)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::SUCCESS);
                        write_float(reply, res_offset, res.value.value);
                        write_byte(reply, res_offset, (uint8_t)res.value.currency);
                        notify_monitors(Opcode::TRANSFER, args.sender_account_num, args.sender_name, res.value.value, res.value.currency, "Transfer completed.");
                    }
                    else if (res.code == ErrorCode::AUTH_FAILED)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::AUTH_FAILED);
                        write_string(reply, res_offset, "Authentication failed: wrong name or password.");
                    }
                    else if (res.code == ErrorCode::ACCOUNT_NOT_FOUND)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::ACCOUNT_NOT_FOUND);
                        write_string(reply, res_offset, "Account does not exist.");
                    }
                    else if (res.code == ErrorCode::RECEIVER_ACCOUNT_NOT_FOUND)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::ACCOUNT_NOT_FOUND);
                        write_string(reply, res_offset, "The receiver account number does not exist.");
                    }
                    else if (res.code == ErrorCode::RECEIVER_NAME_MISMATCH)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::ACCOUNT_NOT_FOUND);
                        write_string(reply, res_offset, "The receiver account number and name do not mismatch.");
                    }
                    else if (res.code == ErrorCode::SENDER_CURRENCY_MISMATCH)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::CURRENCY_MISMATCH);
                        write_string(reply, res_offset, "This currency does not match the currency of the bank account");
                    }
                    else if (res.code == ErrorCode::RECEIVER_CURRENCY_MISMATCH)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::CURRENCY_MISMATCH);
                        write_string(reply, res_offset, "This currency does not match the currency of the receiever's bank account");
                    }
                    else if (res.code == ErrorCode::INVALID_AMOUNT)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::INVALID_AMOUNT);
                        write_string(reply, res_offset, "Transfering an invalid amount. Cannot transfer amounts less than or equal to zero.");
                    }
                    else if (res.code == ErrorCode::INSUFFICIENT_BALANCE)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::INSUFFICIENT_BALANCE);
                        write_string(reply, res_offset, "Cannot transfer amount. Insufficient balance in the account.");
                    }
                }
                else
                {
                    write_byte(reply, res_offset, (uint8_t)Status::MALFORMED_REQUEST);
                    write_string(reply, res_offset, "Request malformed. Content cannot be parsed.");
                }
                sock.send_to(reply, res_offset, client_addr);
                
                prevRequestData[std::string(ipStr)][req_id_key] = std::string(reinterpret_cast<char*>(reply), res_offset);
#ifdef DEBUG
                std::cout << "[CACHED] ip=" << ipStr << " bytes=";
                for (int i = 0; i < res_offset; i++) std::cout << std::hex << (int)reply[i] << " ";
                std::cout << std::dec << std::endl;
#endif
                
                break;
            }

            case Opcode::CHECK_BALANCE: {
                CheckBalanceArgs args;
                int succ = parse_check_balance(buf, req_buf_len, offset, args);

                // response message header
                uint8_t reply[MAX_BUF_SIZE];
                int res_offset = 0;
                write_byte(reply, res_offset, (uint8_t)MessageType::RESPONSE);
                write_request_id(reply, res_offset, request_id);
                write_byte(reply, res_offset, (uint8_t)Opcode::CHECK_BALANCE);

                if (succ)
                {
                    Result<BankAccountBalance> res = bank.check_balance(args.account_num, args.name, args.password);
                    if (res.code == ErrorCode::SUCCESS)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::SUCCESS);
                        write_float(reply, res_offset, res.value.value);
                        write_byte(reply, res_offset, (uint8_t)res.value.currency);
                        notify_monitors(Opcode::CHECK_BALANCE, args.account_num, args.name, res.value.value, res.value.currency, "Balance checked.");
                    }
                    else if (res.code == ErrorCode::AUTH_FAILED)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::AUTH_FAILED);
                        write_string(reply, res_offset, "Authentication failed: wrong name or password.");
                    }
                    else if (res.code == ErrorCode::ACCOUNT_NOT_FOUND)
                    {
                        write_byte(reply, res_offset, (uint8_t)Status::ACCOUNT_NOT_FOUND);
                        write_string(reply, res_offset, "Account does not exist.");
                    }
                }
                else
                {
                    write_byte(reply, res_offset, (uint8_t)Status::MALFORMED_REQUEST);
                    write_string(reply, res_offset, "Request malformed. Content cannot be parsed.");
                }
                sock.send_to(reply, res_offset, client_addr);
                prevRequestData[std::string(ipStr)][req_id_key] = std::string(reinterpret_cast<char*>(reply), res_offset);
#ifdef DEBUG
                std::cout << "[CACHED] ip=" << ipStr << " bytes=";
                for (int i = 0; i < res_offset; i++) std::cout << std::hex << (int)reply[i] << " ";
                std::cout << std::dec << std::endl;
#endif
                break;
            }
            default:
                std::cerr << "Unknown opcode: " << (int)buf[17] << ", dropping." << std::endl;
                continue;
        }

        DBG("");


    }
}
