// this file shows functions related to database operation

#include <iostream>
#include <string>
#include "Contact.h"
#include "Encryption.h"
#include "database.h"

using namespace std;

// generating hash using FNV-1a from the encrypted phone
ull generateHash(const string& encPhone) {
    ull hash = FNVoffset;
    for (char c : encPhone) {
        hash ^= static_cast<unsigned char>(c);
        hash *= FNVprime;
    }
    return hash;
}

// strips '+', ' ' and '-' so the same phone always encrypts to the same encPhone
string normalisePhone(const string& phone) {
    string out;
    for (char c : phone) {
        if (c != '+' && c != ' ' && c != '-') out += c;
    }
    return out;
}

// prints the bucket info and the contact with this phone upon searching for a user in the database
void printBucket(const string& phone, const uc key[KEY_BYTES]) {
    ull hash = generateHash(encryptPhone(normalisePhone(phone), key));
    size_t bucket = hash % (TABLE_SIZE - 1);
    cout << "Hash: " << hash << " Bucket: " << bucket << " (" << table[bucket].size() << " contacts)" << '\n';
    // only the matching row gets decrypted
    Contact c;
    if (findContact(phone, key, c)) {
        cout << c.getFirstName() << " " << c.getSurname() << " "
                << c.getEmail() << " " << c.getPhone() << " " << c.getAddress() << '\n';
    } else {
        cout << "No contact with phone " << phone << '\n';
    }
}
