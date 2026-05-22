#include "User.h"
#include <iostream>

using namespace std;

User::User(string name, int age, int pin) : name{name}, age{age}, pin{pin} 
{
}

bool User::verify_pin(int entered_pin) 
{
    return (this->pin == entered_pin);
}

void User::change_pin(int current_pin) 
{
    if (verify_pin(current_pin)) 
    {
        int new_pin;
        cout << "\n>>> PIN Verified. Please enter your new 4-digit PIN: ";
        cin >> new_pin;

        if (new_pin >= 1000 && new_pin <= 9999) 
        {
            this->pin = new_pin;
            cout << ">>> PIN successfully updated!\n";
        } 
        else 
        {
            cout << ">>> Error: PIN must be exactly 4 digits.\n";
        }
    } 
    else 
    {
        cout << ">>> Entered PIN doesn't match. Unauthorized access.\n";
    }
}

int User::get_age() 
{ 
    return this->age; 
}

int User::get_pin() 
{ 
    return this->pin; 
}