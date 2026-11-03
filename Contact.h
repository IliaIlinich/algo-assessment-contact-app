// Contact class its variables and methods for inclusion

#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "Contact.h"
#include "Encryption.h"

struct Contact {
    std::string firstName = "";
    std::string surname = "";
    std::string email = "";
    std::string phone = "";
    std::string address = "";

    std::string getFirstName() const;
    void setFirstName(const std::string& newFirstName);
    std::string getSurname() const;
    void setSurname(const std::string& newSurname);
    std::string getEmail() const;
    void setEmail(const std::string& newEmail);
    std::string getPhone() const;
    void setPhone(const std::string& newPhone);
    std::string getAddress() const;
    void setAddress(const std::string& newAddress);
};

bool addContact(const Contact& contact, const string& path, const uc key[KEY_BYTES]);
bool removeContact(const string& phoneIn, const string& path, const uc key[KEY_BYTES]);
bool findContact(const string& phoneIn, const uc key[KEY_BYTES], Contact& contactOut);
Contact contactFromLine(const string& line);
string contactToLine(const Contact& contact);
string encryptContact(const Contact& contact, const uc key[KEY_BYTES]);
bool decryptContact(const string& row, const uc key[KEY_BYTES], Contact& contactOut);
string getEncPhone(const string& row);
size_t encryptContactsFile(const string& plainPath, const string& encPath, const uc key[KEY_BYTES]);
size_t loadEncryptedContactsFromFile(const string& encPath);

constexpr size_t TABLE_SIZE = 1ULL << 20;
// each bucket holds encrypted rows (encPhone,nonce,encRest), hashed by encPhone
extern std::vector<std::vector<std::string>> table;
unsigned long long generateHash(const std::string& encPhone);
std::string normalisePhone(const std::string& phone);
std::string contactToLine(const Contact& contact);
