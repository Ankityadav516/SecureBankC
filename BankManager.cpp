#include "BankManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>

using namespace std;

string BankManager::get_current_time()
{
    time_t raw_time = time(0);
    tm *local_time = localtime(&raw_time);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %I:%M%p", local_time);
    return string(buffer);
}

void BankManager::run_bank_session(SavingsAccount &active_account, User &active_user)
{
    int choice = 1;
    int amount = 0;
    int pin = 0;

    cout << "\n>>> Welcome, " << active_account.get_name() << " <<<\n";

    while (choice != 8)
    {
        cout << "\n1:Display Account | 2:Deposit | 3:Withdraw | 4:Change pin | 5:Apply interest | 6:Transfer Money | 7:Transaction History | 8:Exit\n";
        cin >> choice;

        switch (choice)
        {
        case 1:
            active_account.displayAccount();
            break;
        case 2:
        {
            cout << "Enter deposit amount: ";
            cin >> amount;
            active_account.deposit(amount);
            break;
        }
        case 3:
        {
            cout << "Enter the amount to be withdrawn :\n";
            cin >> amount;
            active_account.withdraw(amount);
            break;
        }
        case 4:
        {
            cout << "Enter your current pin :\n";
            cin >> pin;
            active_user.change_pin(pin);
            break;
        }
        case 5:
        {
            active_account.apply_interest();
            break;
        }
        case 6:
        {
            string target_acc_num;
            cout << "Enter the account number you want to transfer money to :\n";
            cin >> target_acc_num;
            if (account_vault.count(target_acc_num))
            {
                SavingsAccount &target = account_vault.at(target_acc_num);
                cout << "Enter the amount you want to tranfer: \n";
                int amount;
                cin >> amount;
                if (active_account.get_balance() < amount)
                    cout << "Not enough balance !\n";
                else
                {
                    active_account.withdraw(amount, true);
                    target.deposit(amount, true);

                    cout << ">>> Transfer of $" << amount << " to Account " << target_acc_num << " was successful.\n";
                    cout << ">>> Your current balance is : $" << active_account.get_balance() << "\n";
                }
            }
            else
            {
                cout << "Account doesn't exist\n";
            }
            break;
        }
        case 7:
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
            active_account.print_statement(date1, date2);
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
    ifstream file("bank_database.csv");

    if (file.is_open())
    {
        string line;
        while (getline(file, line))
        {
            if (line.empty())
                continue;
            stringstream ss(line);
            string name, num, balance_str, age_str, pin_str, history;

            getline(ss, num, ',');
            getline(ss, name, ',');
            getline(ss, age_str, ',');
            getline(ss, pin_str, ',');
            getline(ss, balance_str, ',');
            getline(ss, history);

            stringstream history_ss(history);
            string single;

            int temp_num = stoi(num);
            int temp_bal = stoi(balance_str);
            int temp_age = stoi(age_str);
            int temp_pin = stoi(pin_str);

            if (temp_num > highest_acc_found)
            {
                highest_acc_found = temp_num;
            }

            SavingsAccount temp_acc(name, num, 5, temp_bal);
            User temp_user(name, temp_age, temp_pin);

            while (getline(history_ss, single, '|'))
            {
                int next_id = temp_acc.ledger.size() + 1;
                temp_acc.ledger[next_id] = single;
            }

            account_vault.insert({num, temp_acc});
            user_vault.insert({num, temp_user});
        }
        file.close();
    }
    Account::sync_generator(highest_acc_found);
}

void BankManager::shutdown_server()
{
    ofstream myfile("bank_database.csv");
    if (myfile.is_open())
    {
        for (auto it : account_vault)
        {
            auto &temp = it.second;
            string acc_num = it.first;
            myfile << temp.get_acc_number() << ","
                   << temp.get_name() << ","
                   << user_vault.at(acc_num).get_age() << ","
                   << user_vault.at(acc_num).get_pin() << ","
                   << temp.get_balance() << ",";

            for (auto &iter : temp.ledger)
            {
                myfile << iter.second << "|";
            }
            myfile << "\n";
        }
    }
    else
    {
        cout << "Error in opening database file\n";
    }
    myfile.close();
    cout << "Closing the server.....\n";
}

void BankManager::show_main_menu()
{
    while (true)
    {
        cout << "\n=========================================\n";
        cout << "   WELCOME TO THE SECURE BANK PORTAL   \n";
        cout << "=========================================\n";
        cout << "Enter '1' to Login or '0' to Create Account (or '9' to shutdown): ";

        int option;
        cin >> option;

        if (option == 1)
        {
            string account_number;
            cout << "Enter your account number :\n";
            cin >> account_number;

            if (account_vault.count(account_number) > 0)
            {
                cout << ">>> Account found instantly in RAM!\n";
                SavingsAccount &session_account = account_vault.at(account_number);
                User &session_user = user_vault.at(account_number);
                run_bank_session(session_account, session_user);
            }
            else
            {
                cout << "\n>>> ERROR: Account Number '" << account_number << "' does not exist. Access Denied.\n";
            }
        }
        else if (option == 0)
        {
            string loaded_name;
            int age, pin, loaded_balance;

            cout << "Enter your name :\n";
            cin >> loaded_name;
            cout << "Enter your age :\n";
            cin >> age;
            cout << "Enter your amount you wanna deposit in account :\n";
            cin >> loaded_balance;
            cout << "Enter the pin you want to set for the account (4 digits):\n";
            cin >> pin;

            while (pin < 1000 || pin > 9999)
            {
                cout << ">>> ERROR: PIN must be exactly 4 digits. Try again:\n";
                cin >> pin;
            }

            SavingsAccount temp_account(loaded_name, 5, 0);
            User temp_user(loaded_name, age, pin);

            string new_id = temp_account.get_acc_number();
            account_vault.insert({new_id, temp_account});
            user_vault.insert({new_id, temp_user});

            SavingsAccount &live_account = account_vault.at(new_id);
            User &live_user = user_vault.at(new_id);

            if (loaded_balance > 0)
            {
                live_account.deposit(loaded_balance);
            }

            run_bank_session(live_account, live_user);
        }
        else if (option == 9)
        {
            shutdown_server();
            break;
        }
    }
}