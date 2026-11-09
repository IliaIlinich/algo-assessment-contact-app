// Created by Ilia 21/10/2026
// Algorithms And Data Structures-26AUTHW assessment

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "Contact.h"
#include "Encryption.h"
#include "database.h"

using namespace std;

// the hash table with all encrypted rows, declared in Contact.h
vector<vector<string>> table(TABLE_SIZE);

int main(int argc, char* argv[]) {
    // cout optimisation
    cin.tie(nullptr);
    ios_base::sync_with_stdio(false);
    // initialise sodium
    if (sodium_init() < 0) {
        cout << "Error: libsodium failed to initialise" << '\n';
        return 1;
    }
    // loading the key (created on first run)
    // contacts.csv holds the encrypted rows, contacts_test.csv is plain test data to copy rows from
    string contactsPath = argc > 1 ? argv[1] : "../contacts.csv";
    string keyPath = contactsPath + ".key";
    uc key[KEY_BYTES];
    if (!loadOrCreateKey(keyPath, key)) return 1;
    // creating an empty contacts file (header only) if it doesn't exist yet
    if (!ifstream(contactsPath)) {
        ofstream(contactsPath) << "firstName,surname,email,phone,address" << '\n';
        cout << "Created empty " << contactsPath << '\n';
    }
    // checking for unencrypted records and encrypting them
    size_t encrypted = encryptPlainRows(contactsPath, key);
    if (encrypted > 0) {
        cout << "Found and encrypted " << encrypted << " unencrypted contacts in " << contactsPath << '\n';
    } else {
        cout << "No unencrypted contacts found in " << contactsPath << '\n';
    }
    // loading data from the encrypted file
    size_t loaded = loadEncryptedContactsFromFile(contactsPath);
    cout << "Loaded " << loaded << " contacts from " << contactsPath << '\n';
    
    // test interface for adding, finding and removing a contact
    Contact newContact;
    cout << "--- Add a contact ---" << '\n';
    cout << "First name: " << '\n';
    getline(cin, newContact.firstName);
    cout << "Surname: " << '\n';
    getline(cin, newContact.surname);
    cout << "Email: " << '\n';
    getline(cin, newContact.email);
    cout << "Phone: " << '\n';
    getline(cin, newContact.phone);
    cout << "Address: " << '\n';
    getline(cin, newContact.address);
    if (addContact(newContact, contactsPath, key)) {
        cout << "Contact added" << '\n';
        printBucket(newContact.phone, key);
    }

    cout << "--- Find a contact ---" << '\n';
    cout << "Phone of the contact to find: " << '\n';
    string phoneToFind;
    getline(cin, phoneToFind);
    printBucket(phoneToFind, key);

    cout << "--- Remove a contact ---" << '\n';
    cout << "Phone of the contact to remove: " << '\n';
    string phoneToRemove;
    getline(cin, phoneToRemove);
    if (removeContact(phoneToRemove, contactsPath, key)) {
        cout << "Contact removed" << '\n';
        // searching again to show that the contact is gone
        printBucket(phoneToRemove, key);
    }

    // removing the key from memory
    sodium_memzero(key, sizeof key);
    return 0;
}
