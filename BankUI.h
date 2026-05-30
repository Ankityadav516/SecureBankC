#ifndef BANKUI_H
#define BANKUI_H

#include "BankManager.h"
#include <string>

class BankUI {
private:
    BankManager* manager; 

public:
    BankUI(BankManager* backend_manager);

    int get_valid_int(std::string prompt);
    void show_main_menu();
    void run_admin_session();
    void run_bank_session(Account* active_account, User& active_user);
};

#endif