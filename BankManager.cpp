#include "BankManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <limits>
#include "BankCrypto.h"

using namespace std;

int BankManager::get_valid_int(string prompt)
{
    int input;
    while (true)
    {
        cout << prompt;
        cin >> input;

        if (cin.fail())
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << ">>> ERROR: Invalid input. Please enter a valid number.\n";
        }
        else
        {
            return input;
        }
    }
}

void BankManager::update_database_balance(Account *acc)
{
    // Construct the SQL UPDATE string
    string update_sql = "UPDATE Accounts SET Balance = " + to_string(acc->get_balance()) +
                        " WHERE Account_Number = '" + acc->get_acc_number() + "';";

    // Fire it to the database silently
    sqlite3_exec(db, update_sql.c_str(), NULL, 0, nullptr);
}

string BankManager::get_current_time()
{
    time_t raw_time = time(0);
    tm *local_time = localtime(&raw_time);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %I:%M%p", local_time);
    return string(buffer);
}

void BankManager::run_admin_session()
{
    cout << "\n=========================================\n";
    cout << " >>> ADMIN OVERRIDE ACCEPTED. Welcome. <<<\n";
    cout << "=========================================\n";

    int choice = 0;
    while (choice != 4)
    {
        cout << "\n1: Total Vault Liquidity | 2: View User Roster | 3: Inspect Ledger | 4: Shutdown Server\n";
        choice = get_valid_int("Enter your choice:\n");

        switch (choice)
        {
        case 1:
        {
            long long total_liquidity = 0;
            int total_accounts = 0;

            cout << "\n>>> CALCULATING GLOBAL LIQUIDITY...\n";

            for (auto &it : account_vault)
            {
                total_liquidity += it.second->get_balance();
                total_accounts++;
            }

            cout << "-----------------------------------------\n";
            cout << " TOTAL ACCOUNTS IN VAULT : " << total_accounts << "\n";
            cout << " TOTAL CASH LIQUIDITY    : $" << total_liquidity << "\n";
            cout << "-----------------------------------------\n";
            break;
        }
        case 2:
        {
            cout << "\n>>> VAULT ROSTER: ACTIVE ACCOUNTS <<<\n";
            cout << "---------------------------------------------------\n";
            cout << " ACC NUM    | ACCOUNT HOLDER NAME  | BALANCE \n";
            cout << "---------------------------------------------------\n";

            int user_count = 0;

            for (auto &it : account_vault)
            {
                string acc_num = it.first;
                string name = it.second->get_name();
                int current_balance = it.second->get_balance();

                cout << " " << acc_num << "     | " << name << " | $" << current_balance << "\n";
                user_count++;
            }

            if (user_count == 0)
            {
                cout << " [!] Vault is entirely empty.\n";
            }
            cout << "---------------------------------------------------\n";
            break;
        }
        case 3:
        {
            string target_acc;
            cout << "\n>>> ENTER ACCOUNT NUMBER TO INSPECT: ";
            cin >> target_acc;

            if (account_vault.count(target_acc) > 0)
            {
                Account *target = account_vault.at(target_acc);

                string date1, date2;
                cout << "Enter start date (YYYY-MM-DD): ";
                cin >> date1;
                cout << "Enter end date (YYYY-MM-DD): ";
                cin >> date2;

                cout << "\n>>> PULLING SECURE LEDGER FOR ACCOUNT " << target_acc << " <<<\n";

                target->print_statement(date1, date2);
            }
            else
            {
                cout << ">>> ERROR: Account '" << target_acc << "' not found in vault.\n";
            }
            break;
        }
        case 4:
        {
            cout << ">>> INITIATING SECURE SERVER SHUTDOWN...\n";
            shutdown_server();
            exit(0);
            break;
        }
        case 5:
        {
            break;
        }
        default:
            cout << "Invalid admin command.\n";
        }
    }
}

void BankManager::run_bank_session(Account *active_account, User &active_user)
{
    int choice = 1;
    int amount = 0;
    string pin;

    cout << "\n>>> Welcome, " << active_account->get_name() << " <<<\n";

    while (choice != 7)
    {
        cout << "\n1:Display Account | 2:Deposit | 3:Withdraw | 4:Change pin | 5:Transfer Money | 6:Transaction History | 7:Exit\n";
        choice = get_valid_int("Enter the choice:\n");

        switch (choice)
        {
        case 1:
            active_account->displayAccount();
            break;
        case 2:
        {
            amount = get_valid_int("Enter deposit amount:\n");
            active_account->deposit(amount);
            update_database_balance(active_account);
            break;
        }
        case 3:
        {
            amount = get_valid_int("Enter the amount to be withdrawn :\n");
            active_account->withdraw(amount);
            update_database_balance(active_account);
            break;
        }
        case 4:
        {
            cout << "Enter your current pin :\n";
            cin >> pin;
            string original_hash = active_user.get_hash();
            string salt = active_user.get_salt();
            string hash_pass = BankCrypto::hash_password(pin, salt);
            if (hash_pass == original_hash)
            {
                string new_salt = BankCrypto::generate_salt();
                active_user.set_salt(new_salt);
                string new_pin;
                cout << "Enter the new pin :\n";
                cin >> new_pin;
                while (new_pin.length() != 4)
                {
                    cout << "PLease enter a valid pin:\n";
                    cin >> new_pin;
                }
                string new_hash = BankCrypto::hash_password(new_pin, new_salt);
                active_user.set_hash(new_hash);
                cout << "Password changed successfully\n";
            }
            else
            {
                cout << "Entered pin is wrong:\n";
            }
            break;
        }
        case 5:
        {
            string target_acc_num;
            cout << "Enter the account number you want to transfer money to :\n";
            cin >> target_acc_num;
            if (account_vault.count(target_acc_num))
            {
                Account *target = account_vault.at(target_acc_num);
                int amount;
                amount = get_valid_int("Enter the amount you want to tranfer: \n");
                if (active_account->get_balance() < amount)
                    cout << "Not enough balance !\n";
                else
                {
                    active_account->withdraw(amount, true);
                    target->deposit(amount, true);
                    update_database_balance(active_account);
                    update_database_balance(target);
                    cout << ">>> Transfer of $" << amount << " to Account " << target_acc_num << " was successful.\n";
                    cout << ">>> Your current balance is : $" << active_account->get_balance() << "\n";
                }
            }
            else
            {
                cout << "Account doesn't exist\n";
            }
            break;
        }
        case 6:
        {
            string date1;
            string date2;
            while (true)
            {
                cout << "Enter start date (YYYY-MM-DD):\n";
                cin >> date1;
                string full_timestamp = get_current_time();
                string date = full_timestamp.substr(0, 10);
                if (date1.size() != 10 || date1 > date || date1[4] != '-' || date1[7] != '-')
                {
                    cout << "Invalid format or date, please enter again :\n";
                    continue;
                }
                cout << "Enter end date (YYYY-MM-DD):\n";
                cin >> date2;
                if (date2.size() != 10 || date2 > date || date2[4] != '-' || date2[7] != '-')
                {
                    cout << "Invalid format or date, please enter again :\n";
                    continue;
                }
                if (date1 > date2)
                {
                    cout << "Start date cannot be after end date!\n";
                    continue;
                }
                break;
            }
            active_account->print_statement(date1, date2);
        }
        case 7:
        {
            cout << "Logging out...\n";
            break;
        }
        default:
        {
            cout << "Press a valid choice :";
            break;
        }
        }
    }
}

void BankManager::boot_up_scanner()
{
    int highest_acc_found = 100000;

    // 1. Open Database
    int connection_status = sqlite3_open("secure_vault.db", &db);
    if (connection_status != SQLITE_OK)
    {
        std::cerr << "CRITICAL ERROR: Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    std::string sql_schema =
        "CREATE TABLE IF NOT EXISTS Accounts ("
        "Account_Number TEXT PRIMARY KEY, "
        "Name TEXT NOT NULL, "
        "Age INTEGER, "
        "Hash TEXT, "
        "Salt TEXT, "
        "Balance INTEGER NOT NULL, "
        "Account_Type TEXT);";

    char *error_message = nullptr;
    sqlite3_exec(db, sql_schema.c_str(), NULL, 0, &error_message);

    // 3. The Boot-Up Query (Disk to RAM)
    std::string select_query = "SELECT * FROM Accounts;";
    sqlite3_stmt *stmt; // A pointer to hold our compiled SQL query

    // Compile the query
    if (sqlite3_prepare_v2(db, select_query.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
    {
        // Loop through the database, row by row
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            // Extract data from the database columns
            string num = (const char *)sqlite3_column_text(stmt, 0);
            string name = (const char *)sqlite3_column_text(stmt, 1);
            int age = sqlite3_column_int(stmt, 2);
            string hash_str = (const char *)sqlite3_column_text(stmt, 3);
            string salt_str = (const char *)sqlite3_column_text(stmt, 4);
            int temp_bal = sqlite3_column_int(stmt, 5);

            // Track the highest account number for your generator
            int temp_num = stoi(num);
            if (temp_num > highest_acc_found)
            {
                highest_acc_found = temp_num;
            }

            // Rebuild the C++ objects and push them into the Vaults
            Account *temp_acc = new SavingsAccount(name, num, 5, temp_bal);
            User temp_user(name, age, hash_str, salt_str);

            account_vault.insert({num, temp_acc});
            user_vault.insert({num, temp_user});
        }
    }

    // Clean up the memory used by the query
    sqlite3_finalize(stmt);

    // Synchronize your generator so it doesn't assign duplicate numbers!
    Account::sync_generator(highest_acc_found);

    std::cout << "[SYSTEM] Database Booted. RAM Vault Loaded. Sync Level: " << highest_acc_found << std::endl;
}

void BankManager::shutdown_server()
{
    cout << ">>> SECURING VAULT AND INITIATING SHUTDOWN...\n";

    // 1. Free the Heap Memory (Prevent RAM Leaks)
    for (auto &it : account_vault)
    {
        delete it.second; 
    }
    
    // 2. Clear the RAM maps
    account_vault.clear();
    user_vault.clear();

    // 3. Lock the Vault (Close the SQLite connection)
    if (db) {
        sqlite3_close(db);
        cout << ">>> Database connection closed safely.\n";
    }

    cout << ">>> Server Offline.\n";
}

void BankManager::show_main_menu()
{
    while (true)
    {
        cout << "\n=========================================\n";
        cout << "   WELCOME TO THE SECURE BANK PORTAL   \n";
        cout << "=========================================\n";
        cout << "Enter '1' to Login or '0' to Create Account: ";

        string option;
        cin >> option;

        if (option == "ADMIN")
        {
            string password;
            cout << "\n>>> ENTER MASTER PASSWORD: ";
            cin >> password;

            // We use a hardcoded salt specifically for the Admin
            string master_salt = "MASTER_SALT_123";

            // Run the password through your 10,000x crypto engine
            string attempt_hash = BankCrypto::hash_password(password, master_salt);

            // TEMPORARY LINE: Uncomment this so the console prints the hash for you!
            // cout << ">>> [DEBUG] EXACT HASH: " << attempt_hash << "\n";

            // Replace "PASTE_HASH_HERE" with the string from the terminal
            if (attempt_hash == "c112ddb2414147ae5e6cdebc01f91a007f2b1ead887aab8c7694abf2aa4a9f89")
            {
                run_admin_session();
            }
            else
            {
                cout << ">>> ACCESS DENIED. INTRUDER LOGGED.\n";
            }
            continue;
        }
        else if (option == "1")
        {
            string account_number;
            cout << "Enter your account number :\n";
            cin >> account_number;

            if (account_vault.count(account_number) > 0)
            {
                int i = 3;
                while (i)
                {
                    string pin;
                    cout << "Enter your pin :\n";
                    cin >> pin;
                    string salt = user_vault.at(account_number).get_salt();
                    string original_hash = user_vault.at(account_number).get_hash();
                    string hash_pass = BankCrypto::hash_password(pin, salt);
                    if (hash_pass == original_hash)
                    {
                        cout << ">>> Account found instantly in RAM!\n";
                        Account *session_account = account_vault.at(account_number); // CATCH AS POINTER
                        User &session_user = user_vault.at(account_number);
                        run_bank_session(session_account, session_user);
                        break;
                    }
                    else
                    {
                        cout << "Invalid pin\n";
                        cout << i - 1 << " turns left to login\n";
                    }
                    i--;
                }
            }
            else
            {
                cout << "\n>>> ERROR: Account Number '" << account_number << "' does not exist. Access Denied.\n";
            }
        }
        else if (option == "0")
        {
            string loaded_name;
            int age, loaded_balance;
            string pin;
            cout << "Enter your name :\n";
            cin >> loaded_name;
            age = get_valid_int("Enter your age :\n");
            loaded_balance = get_valid_int("Enter your amount you wanna deposit in account :\n");
            cout << "Enter the pin you want to set for the account (4 digits):\n";
            cin >> pin;

            while (pin.length() != 4)
            {
                cout << ">>> ERROR: PIN must be exactly 4 digits. Try again:\n";
                cin >> pin;
            }
            string salt = BankCrypto::generate_salt();
            string hash_pass = BankCrypto::hash_password(pin, salt);
            // --- ACCOUNT TYPE SELECTION ---
            int acc_type = 0;
            while (acc_type != 1 && acc_type != 2)
            {
                acc_type = get_valid_int("Press 1 for a Savings Account. Press 2 for a Checking Account:\n");
            }

            Account *temp_account = nullptr;

            if (acc_type == 1)
            {
                temp_account = new SavingsAccount(loaded_name, 5, 0);
            }
            else
            {
                temp_account = new CheckingAccount(loaded_name, 0);
            }

            User temp_user(loaded_name, age, hash_pass, salt);

            string new_id = temp_account->get_acc_number();
            account_vault.insert({new_id, temp_account});
            user_vault.insert({new_id, temp_user});
            // --- DATABASE WRITE-THROUGH: SAVE NEW USER TO DISK ---

            // 1. Convert the integer account type to a string
            string acc_type_str = (acc_type == 1) ? "Savings" : "Checking";

            // 2. Construct the SQL string by stitching C++ variables into it
            string insert_sql =
                "INSERT INTO Accounts (Account_Number, Name, Age, Hash, Salt, Balance, Account_Type) "
                "VALUES ('" +
                new_id + "', '" + loaded_name + "', " + to_string(age) + ", '" +
                hash_pass + "', '" + salt + "', " + to_string(loaded_balance) + ", '" + acc_type_str + "');";

            // 3. Fire the command directly to the hard drive
            char *insert_error = nullptr;
            int insert_status = sqlite3_exec(db, insert_sql.c_str(), NULL, 0, &insert_error);

            if (insert_status != SQLITE_OK)
            {
                cerr << ">>> CRITICAL SQL ERROR: Could not save to disk: " << insert_error << endl;
                sqlite3_free(insert_error);
            }
            else
            {
                cout << ">>> Account physically secured in SQLite vault.\n";
            }
            // -----------------------------------------------------
            Account *live_account = account_vault.at(new_id);
            User &live_user = user_vault.at(new_id);

            if (loaded_balance > 0)
            {
                live_account->deposit(loaded_balance);
            }

            run_bank_session(live_account, live_user);
        }
        else
        {
            cout << ">>> ERROR: Invalid input. Please type '1' or '0'.\n";
        }
    }
}