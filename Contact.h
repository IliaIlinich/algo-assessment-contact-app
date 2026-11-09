// this file is made to show all variables and methods of contact struct as well as other functions related to work with contacts

#pragma once

#include <string>
#include <vector>
#include "Encryption.h"

struct Contact {
    std::string firstName = "";
    std::string surname = "";
    std::string email = "";
    std::string phone = "";
    std::string address = "";

    // getters return a message if the field is empty, setters don't accept empty values
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

// checks for empty fields and duplicates, then saves the encrypted contact to the file and the table
bool addContact(const Contact& contact, const string& path, const uc key[KEY_BYTES]);
// removes the contact from the file first and then from the table, so they still match if the file can't be changed
bool removeContact(const string& phoneIn, const string& path, const uc key[KEY_BYTES]);
// encrypts the phone, finds its bucket and decrypts only the matching row
bool findContact(const string& phoneIn, const uc key[KEY_BYTES], Contact& contactOut);
// turns a plain csv line into a contact
Contact contactFromLine(const string& line);
// row format: encPhone,nonce,encRest where rest is firstName,surname,email,address
string encryptContact(const Contact& contact, const uc key[KEY_BYTES]);
// reverse of encryptContact, phone and the rest are encrypted separately so they get decrypted separately
bool decryptContact(const string& row, const uc key[KEY_BYTES], Contact& contactOut);
// encrypted phone is the first field of the row, so it can be read without decrypting anything
string getEncPhone(const string& row);
// rows stay encrypted in memory, only the encrypted phone is used for the hash
size_t loadEncryptedContactsFromFile(const string& encPath);

// amount of buckets in the table
constexpr size_t TABLE_SIZE = 1ULL << 20;
// each bucket holds encrypted rows (encPhone,nonce,encRest), hashed by encPhone
extern std::vector<std::vector<std::string>> table;
