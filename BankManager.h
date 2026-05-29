#pragma once
#include "Account.h"
#include "User.h"
#include <map>
#include "sqlite3.h"
#include <string>
#include <memory>

class BankManager
{
private:
    std::map<std::string, std::unique_ptr<Account>> account_vault;
    std::map<std::string, User> user_vault;
    void run_admin_session();
    int get_valid_int(std::string prompt);
    std::string get_current_time();
    sqlite3 *db;
    std::string get_last_interest_timestamp(std::string acc_num);
    int calculate_elapsed_minutes(std::string past_time_str);
    void update_database_balance(Account *acc);
    void log_transaction(std::string acc_num, std::string action, int amount);
    void run_bank_session(Account *active_account, User &active_user);

public:
    void boot_up_scanner();
    void shutdown_server();
    void show_main_menu();
};