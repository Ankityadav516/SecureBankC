#pragma once
#include <string>
struct sqlite3;

class Account
{
protected:
    static int next_acc_number;
    int balance;
    std::string acc_number;
    std::string acc_holder;

public:
    Account(std::string name, int amount = 0);
    Account(std::string name, std::string existing_acc_num, int amount);
    virtual ~Account() {}
    // Static function
    static void sync_generator(int highest_found);

    int get_balance();
    std::string get_name();
    std::string get_acc_number();
    void print_statement(std::string start_date, std::string end_date, sqlite3 *db);
    void deposit(int amount, bool silent = false);
    virtual bool withdraw(int amount, bool silent = false) = 0;
    void displayAccount();
};

class CheckingAccount : public Account
{
public:
    CheckingAccount(std::string name, int amount = 0);
    CheckingAccount(std::string name, std::string existing_acc_num, int amount);

    // 2. Virtual Destructor
    ~CheckingAccount() override {}

    bool withdraw(int amount, bool silent = false) override;
};

class SavingsAccount : public Account
{
private:
    double interest_rate;

public:
    SavingsAccount(std::string name, double interest_rate, int amount = 0);
    SavingsAccount(std::string name, std::string existing_acc_num, double interest_rate, int amount);
    bool withdraw(int amount, bool silent = false) override;
    int apply_interest(int periods_passed);
};