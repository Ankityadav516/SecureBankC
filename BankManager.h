#pragma once
#include "Account.h"
#include "User.h"
#include <map>
#include "sqlite3.h"
#include <string>

class BankManager
{
private:
    std::map<std::string, Account *> account_vault;
    std::map<std::string, User> user_vault;
    void run_admin_session();
    int get_valid_int(std::string prompt);
    std::string get_current_time();
    sqlite3* db;
    void run_bank_session(Account *active_account, User &active_user);

public:
    void boot_up_scanner();
    void shutdown_server();
    void show_main_menu();
};