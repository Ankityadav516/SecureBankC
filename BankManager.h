#pragma once
#include "Account.h"
#include "User.h"
#include <map>
#include <string>

class BankManager
{
private:
    std::map<std::string, SavingsAccount> account_vault;
    std::map<std::string, User> user_vault;

    std::string get_current_time();
    void run_bank_session(SavingsAccount &active_account, User &active_user);

public:
    void boot_up_scanner();  
    void shutdown_server();  
    void show_main_menu();   
};