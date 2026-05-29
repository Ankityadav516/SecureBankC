#include "Account.h"
#include <iostream>
#include "BankManager.h"
#include <ctime>
#include <iomanip>
#include "sqlite3.h"
#include <cmath>
using namespace std;

int Account::next_acc_number = 100001;

Account::Account(string name, int amount)
    : acc_holder{name}, balance{amount}, acc_number{to_string(next_acc_number++)}
{
}

Account::Account(string name, string existing_acc_num, int amount)
    : acc_holder{name}, balance{amount}, acc_number{existing_acc_num}
{
}

void Account::sync_generator(int highest_found)
{
    next_acc_number = highest_found + 1;
}

int Account::get_balance()
{
    return this->balance;
}

string Account::get_name()
{
    return this->acc_holder;
}

string Account::get_acc_number()
{
    return this->acc_number;
}

void Account::deposit(int amount, bool silent)
{
    if (amount > 0)
    {
        this->balance += amount;

        if (!silent)
            cout << "Amount deposited successfully. Your current balance is : $" << this->balance << "\n";
    }
    else
    {
        cout << "Invalid deposit amount.\n";
    }
}

CheckingAccount::CheckingAccount(std::string name, int amount)
    : Account(name, amount) {}

CheckingAccount::CheckingAccount(std::string name, std::string existing_acc_num, int amount)
    : Account(name, existing_acc_num, amount) {}

void Account::print_statement(string start_date, string end_date, sqlite3 *db)
{
    cout << "\n=================== TRANSACTION LEDGER ===================\n";
    cout << " ACC NUM: " << this->acc_number << " | HOLDER: " << this->acc_holder << "\n";
    cout << " RANGE  : " << start_date << " to " << end_date << "\n";
    cout << "----------------------------------------------------------\n";
    cout << " TIMESTAMP          | ACTION               | AMOUNT       \n";
    cout << "----------------------------------------------------------\n";

    // Query using SQL's substr() to isolate the YYYY-MM-DD part of timestamp
    string statement_sql =
        "SELECT Timestamp, Action, Amount FROM Transactions "
        "WHERE Account_Number = '" +
        this->acc_number + "' "
                           "AND substr(Timestamp, 1, 10) >= '" +
        start_date + "' "
                     "AND substr(Timestamp, 1, 10) <= '" +
        end_date + "' "
                   "ORDER BY Transaction_ID ASC;";

    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, statement_sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
    {
        int row_count = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            string timestamp = (const char *)sqlite3_column_text(stmt, 0);
            string action = (const char *)sqlite3_column_text(stmt, 1);
            int amount = sqlite3_column_int(stmt, 2);

            cout << " " << timestamp << " | "
                 << left << setw(20) << action << " | "
                 << "$" << amount << "\n";
            row_count++;
        }

        if (row_count == 0)
        {
            cout << " [!] No transactions recorded within this time window.\n";
        }
    }
    else
    {
        cerr << ">>> SQL ERROR: Failed to fetch transaction statement history.\n";
    }

    sqlite3_finalize(stmt);
    cout << "==========================================================\n";
}
bool SavingsAccount::withdraw(int amount, bool silent)
{
    if (amount > 0 && amount <= this->balance)
    {
        if (amount > 10000 && !silent)
        {
            cout << ">>> ERROR: Transaction denied. Maximum physical withdrawal is $10,000.\n";
            return false;
        }
        this->balance -= amount;

        if (!silent)
            cout << "Amount withdrawn successfully. Your current balance is : $" << this->balance << "\n";
        return true;
    }
    else
    {
        cout << "Invalid amount or insufficient funds.\n";
        return false;
    }
}

bool CheckingAccount::withdraw(int amount, bool silent)
{
    if (amount <= 0)
    {
        if (!silent)
            cout << ">>> ERROR: Withdrawal amount must be strictly greater than zero.\n";
        return false;
    }
    if (balance - amount < -500)
    {
        if (!silent)
            std::cout << "Error: Transaction declined. Exceeds -$500 overdraft limit.\n";
        return false;
    }
    balance -= amount;

    if (!silent)
        std::cout << "Withdrawn: $" << amount << " | New Balance: $" << balance << "\n";

    if (balance < 0)
    {
        balance -= 35;

        if (!silent)
            std::cout << "WARNING: Account overdrawn! $35 penalty fee applied. New Balance: $" << balance << "\n";
    }
    return true;
}

void Account::displayAccount()
{
    cout << "\n--- Account Details ---\n";
    cout << "Holder Name: " << this->acc_holder << "\n";
    cout << "Account No: " << this->acc_number << "\n";
    cout << "Balance: $" << this->balance << "\n-----------------------\n";
}

SavingsAccount::SavingsAccount(string name, double interest_rate, int amount)
    : Account(name, amount), interest_rate{interest_rate}
{
}

SavingsAccount::SavingsAccount(string name, string existing_acc_num, double interest_rate, int amount)
    : Account(name, existing_acc_num, amount), interest_rate{interest_rate}
{
}
int SavingsAccount::apply_interest(int periods_passed)
{
    if (periods_passed <= 0)
    {
        cout << ">>> No interest accumulated yet. Please wait for the next compounding period.\n";
        return 0;
    }

    int original_balance = get_balance();

    // Scaled rate: 0.01% per minute (0.0001) for safe live testing
    double rate = 0.0001;

    // The Algebraic Engine: A = P(1 + r)^t
    double compounded_balance = original_balance * pow(1.0 + rate, periods_passed);

    // Convert the float back to an integer
    int total_interest_earned = (int)compounded_balance - original_balance;

    if (total_interest_earned > 0)
    {
        deposit(total_interest_earned, true);
        cout << ">>> Compound Interest triggered for " << periods_passed << " periods.\n";
        cout << ">>> Total interest earned: $" << total_interest_earned << "\n";
        cout << ">>> New Balance: $" << get_balance() << "\n";
    }
    else
    {
        cout << ">>> " << periods_passed << " periods passed, but balance is too low to generate a full $1 yet.\n";
    }

    return total_interest_earned;
}