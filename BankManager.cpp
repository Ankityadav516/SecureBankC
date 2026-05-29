#include "BankManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <limits>
#include <iomanip>
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
    string update_sql = "UPDATE Vault SET Balance = " + to_string(acc->get_balance()) +
                        " WHERE Account_Number = '" + acc->get_acc_number() + "';";

    sqlite3_exec(db, update_sql.c_str(), NULL, 0, nullptr);
}

string BankManager::get_last_interest_timestamp(string acc_num)
{
    string query =
        "SELECT Timestamp FROM Transactions "
        "WHERE Account_Number = '" +
        acc_num + "' "
                  "AND (Action = 'Interest Applied' OR Action = 'Initial Deposit') "
                  "ORDER BY Transaction_ID DESC LIMIT 1;";

    sqlite3_stmt *stmt;
    string last_time = "";

    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
    {
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            last_time = (const char *)sqlite3_column_text(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);

    if (last_time == "")
        last_time = get_current_time();

    return last_time;
}

void BankManager::log_transaction(string acc_num, string action, int amount)
{
    string timestamp = get_current_time();

    string insert_sql =
        "INSERT INTO Transactions (Account_Number, Timestamp, Amount, Action) "
        "VALUES ('" +
        acc_num + "', '" + timestamp + "', " +
        to_string(amount) + ", '" + action + "');";

    char *error_message = nullptr;
    if (sqlite3_exec(db, insert_sql.c_str(), nullptr, nullptr, &error_message) != SQLITE_OK)
    {
        cerr << ">>> CRITICAL LEDGER ERROR: Could not write to Transactions: " << error_message << endl;
        sqlite3_free(error_message);
    }
}

string BankManager::get_current_time()
{
    time_t raw_time = time(0);
    tm *local_time = localtime(&raw_time);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %I:%M %p", local_time);
    return string(buffer);
}

int BankManager::calculate_elapsed_minutes(string past_time_str)
{
    time_t now = time(0);
    struct tm past_tm = {};

    try
    {
        // Slice the exact characters using their string index positions
        past_tm.tm_year = stoi(past_time_str.substr(0, 4)) - 1900; // YYYY
        past_tm.tm_mon = stoi(past_time_str.substr(5, 2)) - 1;     // MM
        past_tm.tm_mday = stoi(past_time_str.substr(8, 2));        // DD

        int hour = stoi(past_time_str.substr(11, 2));       // HH
        past_tm.tm_min = stoi(past_time_str.substr(14, 2)); // MM
        past_tm.tm_sec = 0;
        past_tm.tm_isdst = -1;

        // Manually adjust for AM/PM into 24-hour time
        if (past_time_str.find("PM") != string::npos && hour != 12)
        {
            hour += 12;
        }
        else if (past_time_str.find("AM") != string::npos && hour == 12)
        {
            hour = 0;
        }
        past_tm.tm_hour = hour;

        // Convert back to a C++ time object and calculate the difference
        time_t past_time = mktime(&past_tm);
        double seconds_passed = difftime(now, past_time);

        return seconds_passed / 60;
    }
    catch (...)
    {
        cerr << ">>> TIME ERROR: Failed to parse timestamp manually.\n";
        return 0;
    }
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
                Account *target = account_vault.at(target_acc).get();

                string date1, date2;
                cout << "Enter start date (YYYY-MM-DD): ";
                cin >> date1;
                cout << "Enter end date (YYYY-MM-DD): ";
                cin >> date2;

                cout << "\n>>> PULLING SECURE LEDGER FOR ACCOUNT " << target_acc << " <<<\n";

                target->print_statement(date1, date2, db);
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

    while (choice != 8)
    {
        cout << "\n1:Display Account | 2:Deposit | 3:Withdraw | 4:Change pin | 5:Transfer Money | 6:Transaction History";

        if (dynamic_cast<SavingsAccount *>(active_account) != nullptr)
        {
            cout << " | 7:Apply Interest";
        }

        cout << " | 8:Exit\n";

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

            log_transaction(active_account->get_acc_number(), "Deposit", amount);
            break;
        }
        case 3:
        {
            amount = get_valid_int("Enter the amount to be withdrawn :\n");

            int old_bal = active_account->get_balance();

            if (active_account->withdraw(amount))
            {
                update_database_balance(active_account);
                log_transaction(active_account->get_acc_number(), "Withdrawal", amount);

                int expected_bal = old_bal - amount;
                if (active_account->get_balance() < expected_bal)
                {
                    int penalty = expected_bal - active_account->get_balance();
                    log_transaction(active_account->get_acc_number(), "Overdraft Penalty", penalty);
                }
            }
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

            if (target_acc_num == active_account->get_acc_number())
            {
                cout << ">>> ERROR: You cannot transfer money to your own account.\n";
                break;
            }

            if (account_vault.count(target_acc_num))
            {
                Account *target = account_vault.at(target_acc_num).get();
                amount = get_valid_int("Enter the amount you want to tranfer: \n");

                int old_bal = active_account->get_balance();

                if (active_account->withdraw(amount, true))
                {
                    target->deposit(amount, true);

                    update_database_balance(active_account);
                    update_database_balance(target);

                    log_transaction(active_account->get_acc_number(), "Transfer Out to " + target_acc_num, amount);
                    log_transaction(target_acc_num, "Transfer In from " + active_account->get_acc_number(), amount);

                    int expected_bal = old_bal - amount;
                    if (active_account->get_balance() < expected_bal)
                    {
                        int penalty = expected_bal - active_account->get_balance();
                        log_transaction(active_account->get_acc_number(), "Overdraft Penalty", penalty);
                    }

                    cout << ">>> Transfer of $" << amount << " to Account " << target_acc_num << " was successful.\n";
                    cout << ">>> Your current balance is : $" << active_account->get_balance() << "\n";
                }
            }
            else
            {
                cout << ">>> ERROR: Target Account doesn't exist.\n";
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
            active_account->print_statement(date1, date2, db);
            break;
        }
        case 7:
        {
            SavingsAccount *savings_ptr = dynamic_cast<SavingsAccount *>(active_account);

            if (savings_ptr != nullptr)
            {
                string last_time = get_last_interest_timestamp(active_account->get_acc_number());

                int minutes_passed = calculate_elapsed_minutes(last_time);

                int earned = savings_ptr->apply_interest(minutes_passed);

                if (earned > 0)
                {
                    update_database_balance(active_account);
                    log_transaction(active_account->get_acc_number(), "Interest Applied", earned);
                }
            }
            else
            {
                cout << ">>> ERROR: Checking Accounts do not accumulate compound interest.\n";
            }
            break;
        }
        case 8:
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

    string pragma_query = "PRAGMA foreign_keys = ON;";
    sqlite3_exec(db, pragma_query.c_str(), nullptr, nullptr, nullptr);

    string sql_schema =
        // Table 1: Users
        "CREATE TABLE IF NOT EXISTS Users ("
        "Account_Number TEXT PRIMARY KEY, "
        "Name TEXT NOT NULL, "
        "Age INTEGER NOT NULL, "
        "Hash TEXT NOT NULL, "
        "Salt TEXT NOT NULL"
        ");"

        // Table 2: Vault
        "CREATE TABLE IF NOT EXISTS Vault ("
        "Account_Number TEXT PRIMARY KEY, "
        "Balance INTEGER NOT NULL, "
        "Account_Type TEXT NOT NULL, "
        "FOREIGN KEY(Account_Number) REFERENCES Users(Account_Number) ON DELETE CASCADE"
        ");"

        // Table 3: Transactions
        "CREATE TABLE IF NOT EXISTS Transactions ("
        "Transaction_ID INTEGER PRIMARY KEY AUTOINCREMENT, "
        "Account_Number TEXT NOT NULL, "
        "Timestamp TEXT NOT NULL, "
        "Amount INTEGER NOT NULL, "
        "Action TEXT NOT NULL, "
        "FOREIGN KEY(Account_Number) REFERENCES Users(Account_Number) ON DELETE CASCADE"
        ");";

    char *error_message = nullptr;
    int rc = sqlite3_exec(db, sql_schema.c_str(), nullptr, nullptr, &error_message);

    if (rc != SQLITE_OK)
    {
        cerr << ">>> CRITICAL ERROR: Database Split Failed: " << error_message << endl;
        sqlite3_free(error_message);
    }
    else
    {
        cout << "[SYSTEM] Relational 3-Table Schema Initialized Successfully." << endl;
    }

    // 3. The Boot-Up Query (Disk to RAM using an INNER JOIN)
    string select_query =
        "SELECT u.Account_Number, u.Name, u.Age, u.Hash, u.Salt, v.Balance, v.Account_Type "
        "FROM Users u "
        "JOIN Vault v ON u.Account_Number = v.Account_Number;";

    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, select_query.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            string num = (const char *)sqlite3_column_text(stmt, 0);
            string name = (const char *)sqlite3_column_text(stmt, 1);
            int age = sqlite3_column_int(stmt, 2);
            string hash_str = (const char *)sqlite3_column_text(stmt, 3);
            string salt_str = (const char *)sqlite3_column_text(stmt, 4);
            int temp_bal = sqlite3_column_int(stmt, 5);
            string type_str = (const char *)sqlite3_column_text(stmt, 6);

            int temp_num = stoi(num);
            if (temp_num > highest_acc_found)
            {
                highest_acc_found = temp_num;
            }

            // Dynamically allocate the correct account type using unique_ptr
            std::unique_ptr<Account> temp_acc;
            if (type_str == "Savings")
            {
                temp_acc = std::make_unique<SavingsAccount>(name, num, 5, temp_bal);
            }
            else
            {
                temp_acc = std::make_unique<CheckingAccount>(name, num, temp_bal);
            }

            User temp_user(name, age, hash_str, salt_str);

            account_vault.emplace(num, std::move(temp_acc));
            user_vault.insert({num, temp_user});
        }
    }
    sqlite3_finalize(stmt);

    Account::sync_generator(highest_acc_found);

    std::cout << "[SYSTEM] Database Booted. RAM Vault Loaded. Sync Level: " << highest_acc_found << std::endl;
}

void BankManager::shutdown_server()
{
    cout << ">>> SECURING VAULT AND INITIATING SHUTDOWN...\n";

    // 2. Clear the RAM maps
    account_vault.clear();
    user_vault.clear();

    // 3. Lock the Vault (Close the SQLite connection)
    if (db)
    {
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
                        Account *session_account = account_vault.at(account_number).get();
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
            while (loaded_balance < 0)
            {
                cout << ">>> ERROR: You cannot open an account with negative money.\n";
                loaded_balance = get_valid_int("Enter your amount you wanna deposit in account :\n");
            }
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

            // --- ACCOUNT TYPE SELECTION ---

            std::unique_ptr<Account> temp_account;

            if (acc_type == 1)
            {
                temp_account = std::make_unique<SavingsAccount>(loaded_name, 5, 0);
            }
            else
            {
                temp_account = std::make_unique<CheckingAccount>(loaded_name, 0);
            }

            User temp_user(loaded_name, age, hash_pass, salt);

            string new_id = temp_account->get_acc_number();

            account_vault.emplace(new_id, std::move(temp_account));
            user_vault.insert({new_id, temp_user});

            // --- DATABASE WRITE-THROUGH: ANTI-HACK PARAMETERIZATION ---
            string acc_type_str = (acc_type == 1) ? "Savings" : "Checking";

            // 1. Secure the Users Table Insert
            string insert_user_sql = "INSERT INTO Users (Account_Number, Name, Age, Hash, Salt) VALUES (?, ?, ?, ?, ?);";
            sqlite3_stmt *user_stmt;

            if (sqlite3_prepare_v2(db, insert_user_sql.c_str(), -1, &user_stmt, nullptr) == SQLITE_OK)
            {
                // Bind variables safely to the ? placeholders
                sqlite3_bind_text(user_stmt, 1, new_id.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(user_stmt, 2, loaded_name.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(user_stmt, 3, age);
                sqlite3_bind_text(user_stmt, 4, hash_pass.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(user_stmt, 5, salt.c_str(), -1, SQLITE_TRANSIENT);

                if (sqlite3_step(user_stmt) != SQLITE_DONE)
                {
                    cerr << ">>> CRITICAL SQL ERROR (Users Table)\n";
                }
            }
            sqlite3_finalize(user_stmt);

            // 2. Secure the Vault Table Insert
            string insert_vault_sql = "INSERT INTO Vault (Account_Number, Balance, Account_Type) VALUES (?, ?, ?);";
            sqlite3_stmt *vault_stmt;

            if (sqlite3_prepare_v2(db, insert_vault_sql.c_str(), -1, &vault_stmt, nullptr) == SQLITE_OK)
            {
                sqlite3_bind_text(vault_stmt, 1, new_id.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(vault_stmt, 2, loaded_balance);
                sqlite3_bind_text(vault_stmt, 3, acc_type_str.c_str(), -1, SQLITE_TRANSIENT);

                if (sqlite3_step(vault_stmt) != SQLITE_DONE)
                {
                    cerr << ">>> CRITICAL SQL ERROR (Vault Table)\n";
                }
                else
                {
                    cout << ">>> Account physically secured in the 3-Table SQLite vault.\n";
                }
            }
            sqlite3_finalize(vault_stmt);
            // -----------------------------------------------------
            Account *live_account = account_vault.at(new_id).get();
            User &live_user = user_vault.at(new_id);

            if (loaded_balance > 0)
            {
                live_account->deposit(loaded_balance);
                log_transaction(new_id, "Initial Deposit", loaded_balance);
            }

            run_bank_session(live_account, live_user);
        }
        else
        {
            cout << ">>> ERROR: Invalid input. Please type '1' or '0'.\n";
        }
    }
}