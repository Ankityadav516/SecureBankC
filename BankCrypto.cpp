#include "BankCrypto.h"
#include "picosha2.h"
#include <random>

using namespace std;

string BankCrypto::generate_salt()
{
    const string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    random_device rd;
    mt19937 generator(rd());

    uniform_int_distribution<int> distribution(0, charset.length() - 1);

    string salt = "";
    for (int i = 0; i < 16; ++i)
    {
        salt += charset[distribution(generator)];
    }

    return salt;
}

string BankCrypto::hash_password(string raw_password, string salt)
{
    raw_password = raw_password + salt;
    for (int i = 0; i < 10000; i++)
    {
        raw_password = picosha2::hash256_hex_string(raw_password);
    }
    return raw_password;
}