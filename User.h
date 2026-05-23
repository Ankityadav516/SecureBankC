#pragma once
#include <string>

class User
{
private:
    std::string name;
    int age;
    std::string password_hash;
    std::string salt;

public:
    User(std::string name, int age, std::string hash, std::string s);

    std::string get_name();
    int get_age();
    std::string get_hash();
    std::string get_salt();

    void set_hash(std::string new_hash);
    void set_salt(std::string new_salt);
};