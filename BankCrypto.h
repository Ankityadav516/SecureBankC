#ifndef BANK_CRYPTO_H
#define BANK_CRYPTO_H

#include <string>

using namespace std;

class BankCrypto {
public:
    // Generates a truly random 16-character string
    static string generate_salt();

    // Takes the raw PIN and the Salt, runs SHA-256 10,000 times
    static string hash_password(string raw_password, string salt);
};

#endif