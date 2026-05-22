#pragma once
#include <string>
#include <map>

class Account
{
private:
    static int next_acc_number;
    int balance;
    std::string acc_number;
    std::string acc_holder;
    std::map<int, std::string> ledger;

public:
    Account(std::string name, int amount = 0);
    Account(std::string name, std::string existing_acc_num, int amount);

    // Static function
    static void sync_generator(int highest_found);

    std::map<int, std::string> get_ledger(); 
    void load_history(std::string record);
    int get_balance();
    std::string get_name();
    std::string get_acc_number();
    void print_statement(std::string start_date, std::string end_date);
    void deposit(int amount,bool silent = false);
    void withdraw(int amount, bool silent = false);
    void displayAccount();
};

class SavingsAccount : public Account
{
private:
    double interest_rate;

public:
    SavingsAccount(std::string name, double interest_rate, int amount = 0);
    SavingsAccount(std::string name, std::string existing_acc_num, double interest_rate, int amount);

    int apply_interest();
};