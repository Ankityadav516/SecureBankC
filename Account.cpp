#include "Account.h"
#include <iostream>
#include "BankManager.h"
#include <ctime>
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

std::map<int, std::string> Account::get_ledger()
{
    return this->ledger;
}

void Account::load_history(string record)
{
    int next_id = this->ledger.size() + 1;
    this->ledger[next_id] = record;
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

        time_t raw_time = time(0);
        tm *local_time = localtime(&raw_time);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %I:%M%p", local_time);
        string timeOfDeposit = string(buffer);

        string value = timeOfDeposit + "@+" + to_string(amount);

        int key = this->ledger.size() + 1;
        this->ledger[key] = value;
        if (!silent)
            cout << "Amount deposited successfully. Your current balance is : " << this->balance << "\n";
    }
    else
    {
        cout << "Invalid deposit amount.\n";
    }
}

void Account::print_statement(string start_date, string end_date)
{
    for (auto it : this->ledger)
    {
        string stamp = it.second;
        if (stamp.size() < 10)
            continue;
        string date = stamp.substr(0, 10);
        string toPrint = stamp.substr(0, 18);
        if (date >= start_date && date <= end_date)
        {
            cout << "[" << toPrint << "] | ";
            char sign = stamp[19];
            if (sign == '+')
                cout << "DEPOSIT | ";
            else if (sign == '-')
                cout << "WITHDRAWAL | ";
            string amount;
            int i = 20;
            while (i < stamp.size())
            {
                amount.push_back(stamp[i]);
                i++;
            }
            cout << "$" << amount << "\n";
        }
        else if (date > end_date)
            break;
    }
    cout << "-------------------------------------------------\n";
}
void Account::withdraw(int amount, bool silent)
{
    if (amount > 0 && amount <= this->balance)
    {
        this->balance -= amount;

        time_t raw_time = time(0);
        tm *local_time = localtime(&raw_time);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %I:%M%p", local_time);
        string timeOfWithdraw = string(buffer);

        string value = timeOfWithdraw + "@-" + to_string(amount);

        int key = this->ledger.size() + 1;
        this->ledger[key] = value;
        if (!silent)
            cout << "Amount withdrawn successfully. Your current balance is : " << this->balance << "\n";
    }
    else
    {
        cout << "Invalid amount or insufficient funds.\n";
    }
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

int SavingsAccount::apply_interest()
{
    int current_balance = get_balance();
    int interest = (current_balance * interest_rate) / 100;
    deposit(interest);
    cout << "Interest applied. Total interest earned: $" << interest << "\n";
    return interest;
}