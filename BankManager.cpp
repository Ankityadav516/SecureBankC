#include "BankManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>
#include <limits>
#include <iomanip>
#include "BankCrypto.h"

using namespace std;

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

