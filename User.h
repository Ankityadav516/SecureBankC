#pragma once
#include <string>

class User
{
private:
    int age;
    int pin;
    std::string name;

public:
    User(std::string name, int age, int pin);

    bool verify_pin(int entered_pin);
    void change_pin(int current_pin);
    int get_age();
    int get_pin();
};