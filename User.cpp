#include "User.h"

using namespace std;

User::User(string name, int age, string hash, string s) : name{name}, age{age}, password_hash{hash}, salt{s} 
{
}

string User::get_name() 
{
    return name;
}

int User::get_age() 
{ 
    return age; 
}

string User::get_hash() 
{
    return password_hash;
}

string User::get_salt() 
{
    return salt;
}

void User::set_hash(string new_hash)
{
    password_hash = new_hash;
}

void User::set_salt(string new_salt)
{
    salt = new_salt;
}