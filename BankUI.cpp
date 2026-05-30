#include "BankUI.h"
#include "BankCrypto.h"
#include <iostream>
#include <limits>

using namespace std;

// 1. The Constructor: Wires the UI to the Backend
BankUI::BankUI(BankManager* backend_manager) {
    this->manager = backend_manager;
}

// 2. The Input Helper
int BankUI::get_valid_int(string prompt) {
    int input;
    while (true) {
        cout << prompt;
        cin >> input;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << ">>> ERROR: Invalid input. Please enter a valid number.\n";
        } else {
            return input;
        }
    }
}

// 3. The Admin Menu
void BankUI::run_admin_session() {
    cout << "\n=========================================\n";
    cout << " >>> ADMIN OVERRIDE ACCEPTED. Welcome. <<<\n";
    cout << "=========================================\n";

    int choice = 0;
    while (choice != 4) {
        cout << "\n1: Total Vault Liquidity | 2: View User Roster | 3: Inspect Ledger | 4: Shutdown Server\n";
        choice = get_valid_int("Enter your choice:\n");

        switch (choice) {
        case 1: {
            long long total_liquidity = 0;
            int total_accounts = 0;
            cout << "\n>>> CALCULATING GLOBAL LIQUIDITY...\n";
            for (auto &it : manager->account_vault) { // USING MANAGER POINTER
                total_liquidity += it.second->get_balance();
                total_accounts++;
            }
            cout << "-----------------------------------------\n";
            cout << " TOTAL ACCOUNTS IN VAULT : " << total_accounts << "\n";
            cout << " TOTAL CASH LIQUIDITY    : $" << total_liquidity << "\n";
            cout << "-----------------------------------------\n";
            break;
        }
        case 2: {
            cout << "\n>>> VAULT ROSTER: ACTIVE ACCOUNTS <<<\n";
            cout << "---------------------------------------------------\n";
            cout << " ACC NUM    | ACCOUNT HOLDER NAME  | BALANCE \n";
            cout << "---------------------------------------------------\n";
            int user_count = 0;
            for (auto &it : manager->account_vault) {
                cout << " " << it.first << "     | " << it.second->get_name() << " | $" << it.second->get_balance() << "\n";
                user_count++;
            }
            if (user_count == 0) cout << " [!] Vault is entirely empty.\n";
            cout << "---------------------------------------------------\n";
            break;
        }
        case 3: {
            string target_acc;
            cout << "\n>>> ENTER ACCOUNT NUMBER TO INSPECT: ";
            cin >> target_acc;
            if (manager->account_vault.count(target_acc) > 0) {
                Account *target = manager->account_vault.at(target_acc).get();
                string date1, date2;
                cout << "Enter start date (YYYY-MM-DD): "; cin >> date1;
                cout << "Enter end date (YYYY-MM-DD): "; cin >> date2;
                cout << "\n>>> PULLING SECURE LEDGER FOR ACCOUNT " << target_acc << " <<<\n";
                target->print_statement(date1, date2, manager->db);
            } else {
                cout << ">>> ERROR: Account '" << target_acc << "' not found in vault.\n";
            }
            break;
        }
        case 4: {
            cout << ">>> INITIATING SECURE SERVER SHUTDOWN...\n";
            manager->shutdown_server();
            exit(0);
            break;
        }
        default:
            cout << "Invalid admin command.\n";
        }
    }
}

// 4. The User Banking Menu
void BankUI::run_bank_session(Account *active_account, User &active_user) {
    int choice = 1;
    int amount = 0;
    string pin;

    cout << "\n>>> Welcome, " << active_account->get_name() << " <<<\n";

    while (choice != 8) {
        cout << "\n1:Display Account | 2:Deposit | 3:Withdraw | 4:Change pin | 5:Transfer Money | 6:Transaction History";
        if (dynamic_cast<SavingsAccount *>(active_account) != nullptr) cout << " | 7:Apply Interest";
        cout << " | 8:Exit\n";

        choice = get_valid_int("Enter the choice:\n");
        switch (choice) {
        case 1:
            active_account->displayAccount();
            break;
        case 2: {
            amount = get_valid_int("Enter deposit amount:\n");
            active_account->deposit(amount);
            manager->update_database_balance(active_account);
            manager->log_transaction(active_account->get_acc_number(), "Deposit", amount);
            break;
        }
        case 3: {
            amount = get_valid_int("Enter the amount to be withdrawn :\n");
            int old_bal = active_account->get_balance();
            if (active_account->withdraw(amount)) {
                manager->update_database_balance(active_account);
                manager->log_transaction(active_account->get_acc_number(), "Withdrawal", amount);
                if (active_account->get_balance() < (old_bal - amount)) {
                    int penalty = (old_bal - amount) - active_account->get_balance();
                    manager->log_transaction(active_account->get_acc_number(), "Overdraft Penalty", penalty);
                }
            }
            break;
        }
        case 4: {
            cout << "Enter your current pin :\n";
            cin >> pin;
            if (BankCrypto::hash_password(pin, active_user.get_salt()) == active_user.get_hash()) {
                string new_salt = BankCrypto::generate_salt();
                active_user.set_salt(new_salt);
                string new_pin;
                cout << "Enter the new pin :\n";
                cin >> new_pin;
                while (new_pin.length() != 4) { cout << "PLease enter a valid pin:\n"; cin >> new_pin; }
                active_user.set_hash(BankCrypto::hash_password(new_pin, new_salt));
                cout << "Password changed successfully\n";
            } else {
                cout << "Entered pin is wrong:\n";
            }
            break;
        }
        case 5: {
            string target_acc_num;
            cout << "Enter the account number you want to transfer money to :\n";
            cin >> target_acc_num;

            if (target_acc_num == active_account->get_acc_number()) {
                cout << ">>> ERROR: You cannot transfer money to your own account.\n";
                break;
            }

            if (manager->account_vault.count(target_acc_num)) {
                Account *target = manager->account_vault.at(target_acc_num).get();
                amount = get_valid_int("Enter the amount you want to tranfer: \n");
                int old_bal = active_account->get_balance();

                if (active_account->withdraw(amount, true)) {
                    target->deposit(amount, true);
                    manager->update_database_balance(active_account);
                    manager->update_database_balance(target);
                    manager->log_transaction(active_account->get_acc_number(), "Transfer Out to " + target_acc_num, amount);
                    manager->log_transaction(target_acc_num, "Transfer In from " + active_account->get_acc_number(), amount);

                    if (active_account->get_balance() < (old_bal - amount)) {
                        int penalty = (old_bal - amount) - active_account->get_balance();
                        manager->log_transaction(active_account->get_acc_number(), "Overdraft Penalty", penalty);
                    }
                    cout << ">>> Transfer of $" << amount << " to Account " << target_acc_num << " was successful.\n";
                }
            } else {
                cout << ">>> ERROR: Target Account doesn't exist.\n";
            }
            break;
        }
        case 6: {
            string date1, date2;
            string date = manager->get_current_time().substr(0, 10);
            while (true) {
                cout << "Enter start date (YYYY-MM-DD):\n"; cin >> date1;
                if (date1.size() != 10 || date1 > date || date1[4] != '-' || date1[7] != '-') { cout << "Invalid format.\n"; continue; }
                cout << "Enter end date (YYYY-MM-DD):\n"; cin >> date2;
                if (date2.size() != 10 || date2 > date || date2[4] != '-' || date2[7] != '-') { cout << "Invalid format.\n"; continue; }
                if (date1 > date2) { cout << "Start date cannot be after end date!\n"; continue; }
                break;
            }
            active_account->print_statement(date1, date2, manager->db);
            cout << "\nWould you like to download this statement as a CSV file? (y/n): ";
            char dl_choice;
            cin >> dl_choice;
            if (dl_choice == 'y' || dl_choice == 'Y') active_account->export_statement(date1, date2, manager->db);
            break;
        }
        case 7: {
            SavingsAccount *savings_ptr = dynamic_cast<SavingsAccount *>(active_account);
            if (savings_ptr != nullptr) {
                int minutes_passed = manager->calculate_elapsed_minutes(manager->get_last_interest_timestamp(active_account->get_acc_number()));
                int earned = savings_ptr->apply_interest(minutes_passed);
                if (earned > 0) {
                    manager->update_database_balance(active_account);
                    manager->log_transaction(active_account->get_acc_number(), "Interest Applied", earned);
                }
            } else {
                cout << ">>> ERROR: Checking Accounts do not accumulate compound interest.\n";
            }
            break;
        }
        case 8:
            cout << "Logging out...\n";
            break;
        default:
            cout << "Press a valid choice :";
        }
    }
}

// 5. The Main Menu
void BankUI::show_main_menu() {
    while (true) {
        cout << "\n=========================================\n";
        cout << "   WELCOME TO THE SECURE BANK PORTAL   \n";
        cout << "=========================================\n";
        cout << "Enter '1' to Login or '0' to Create Account: ";

        string option;
        cin >> option;

        if (option == "ADMIN") {
            string password;
            cout << "\n>>> ENTER MASTER PASSWORD: ";
            cin >> password;
            if (BankCrypto::hash_password(password, "MASTER_SALT_123") == "c112ddb2414147ae5e6cdebc01f91a007f2b1ead887aab8c7694abf2aa4a9f89") {
                run_admin_session();
            } else {
                cout << ">>> ACCESS DENIED. INTRUDER LOGGED.\n";
            }
            continue;
        } else if (option == "1") {
            string account_number;
            cout << "Enter your account number :\n";
            cin >> account_number;

            if (manager->account_vault.count(account_number) > 0) {
                int i = 3;
                while (i) {
                    string pin;
                    cout << "Enter your pin :\n";
                    cin >> pin;
                    if (BankCrypto::hash_password(pin, manager->user_vault.at(account_number).get_salt()) == manager->user_vault.at(account_number).get_hash()) {
                        cout << ">>> Account found instantly in RAM!\n";
                        run_bank_session(manager->account_vault.at(account_number).get(), manager->user_vault.at(account_number));
                        break;
                    } else {
                        cout << "Invalid pin\n" << i - 1 << " turns left to login\n";
                    }
                    i--;
                }
            } else {
                cout << "\n>>> ERROR: Account Number '" << account_number << "' does not exist. Access Denied.\n";
            }
        } else if (option == "0") {
            string loaded_name; int age, loaded_balance; string pin;
            cout << "Enter your name :\n"; cin >> loaded_name;
            age = get_valid_int("Enter your age :\n");
            loaded_balance = get_valid_int("Enter your amount you wanna deposit in account :\n");
            while (loaded_balance < 0) {
                cout << ">>> ERROR: You cannot open an account with negative money.\n";
                loaded_balance = get_valid_int("Enter your amount you wanna deposit in account :\n");
            }
            cout << "Enter the pin you want to set for the account (4 digits):\n"; cin >> pin;
            while (pin.length() != 4) { cout << ">>> ERROR: PIN must be exactly 4 digits. Try again:\n"; cin >> pin; }

            string salt = BankCrypto::generate_salt();
            string hash_pass = BankCrypto::hash_password(pin, salt);

            int acc_type = 0;
            while (acc_type != 1 && acc_type != 2) {
                acc_type = get_valid_int("Press 1 for a Savings Account. Press 2 for a Checking Account:\n");
            }

            std::unique_ptr<Account> temp_account;
            if (acc_type == 1) temp_account = std::make_unique<SavingsAccount>(loaded_name, 5, 0);
            else temp_account = std::make_unique<CheckingAccount>(loaded_name, 0);

            User temp_user(loaded_name, age, hash_pass, salt);
            string new_id = temp_account->get_acc_number();

            manager->account_vault.emplace(new_id, std::move(temp_account));
            manager->user_vault.insert({new_id, temp_user});

            string acc_type_str = (acc_type == 1) ? "Savings" : "Checking";

            string insert_user_sql = "INSERT INTO Users (Account_Number, Name, Age, Hash, Salt) VALUES (?, ?, ?, ?, ?);";
            sqlite3_stmt *user_stmt;
            if (sqlite3_prepare_v2(manager->db, insert_user_sql.c_str(), -1, &user_stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(user_stmt, 1, new_id.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(user_stmt, 2, loaded_name.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(user_stmt, 3, age);
                sqlite3_bind_text(user_stmt, 4, hash_pass.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(user_stmt, 5, salt.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(user_stmt);
            }
            sqlite3_finalize(user_stmt);

            string insert_vault_sql = "INSERT INTO Vault (Account_Number, Balance, Account_Type) VALUES (?, ?, ?);";
            sqlite3_stmt *vault_stmt;
            if (sqlite3_prepare_v2(manager->db, insert_vault_sql.c_str(), -1, &vault_stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(vault_stmt, 1, new_id.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(vault_stmt, 2, loaded_balance);
                sqlite3_bind_text(vault_stmt, 3, acc_type_str.c_str(), -1, SQLITE_TRANSIENT);
                if (sqlite3_step(vault_stmt) == SQLITE_DONE) cout << ">>> Account physically secured in the 3-Table SQLite vault.\n";
            }
            sqlite3_finalize(vault_stmt);

            Account *live_account = manager->account_vault.at(new_id).get();
            if (loaded_balance > 0) {
                live_account->deposit(loaded_balance);
                manager->log_transaction(new_id, "Initial Deposit", loaded_balance);
            }
            run_bank_session(live_account, manager->user_vault.at(new_id));
        } else {
            cout << ">>> ERROR: Invalid input. Please type '1' or '0'.\n";
        }
    }
}