// Created by Ilia 21/10/2026
// Algorithms And Data Structures-26AUTHW assessment

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "Contact.h"
#include "Encryption.h"

#define ll long long
#define ull unsigned long long
#define FNVoffset 14695981039346656037ULL
#define FNVprime 1099511628211ULL

using namespace std;

vector<vector<string>> table(TABLE_SIZE);

// Generating hash using FNV-1a from the encrypted phone
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

bool loadOrCreateKey(const string& keyPath, uc key[KEY_BYTES]) {
    ifstream in(keyPath, ios::binary);
    if (in) {
        in.read(reinterpret_cast<char*>(key), KEY_BYTES);
        if (in.gcount() != static_cast<streamsize>(KEY_BYTES)) {
            cout << "Error: key file " << keyPath << " is corrupted" << '\n';
            return false;
        }
        return true;
    }
    crypto_aead_xchacha20poly1305_ietf_keygen(key);
    ofstream out(keyPath, ios::binary);
    if (!out) {
        cout << "Error: could not create key file " << keyPath << '\n';
        return false;
    }
    out.write(reinterpret_cast<const char*>(key), KEY_BYTES);
    cout << "New key generated and saved to " << keyPath << '\n';
    return true;
}

bool looksLikePlainRow(const string& line) {
    size_t commas = 0;
    for (char c : line) {
        if (c == ',') commas++;
    }
    return commas >= 4;
}

// encrypted rows are encPhone,nonce,encRest (base64 has no commas)
bool looksLikeEncryptedRow(const string& line) {
    size_t commas = 0;
    for (char c : line) {
        if (c == ',') commas++;
    }
    return commas == 2;
}

size_t encryptPlainRows(const string& path, const uc key[KEY_BYTES]) {
    ifstream in(path);
    if (!in) {
        cout << "Error: could not open " << path << '\n';
        return 0;
    }
    string tmpPath = path + ".tmp";
    ofstream out(tmpPath);
    if (!out) {
        cout << "Error: could not create " << tmpPath << '\n';
        return 0;
    }
    string line;
    getline(in, line);
    out << line << '\n';
    size_t encrypted = 0, broken = 0;
    while (getline(in, line)) {
        if (line.empty()) continue;
        if (looksLikePlainRow(line)) {
            out << encryptContact(contactFromLine(line), key) << '\n';
            encrypted++;
        } else if (looksLikeEncryptedRow(line)) {
            out << line << '\n';
        } else {
            out << line << '\n';
            broken++;
        }
    }
    in.close();
    out.close();
    if (broken > 0) cout << "Warning: " << broken << " rows are not valid encrypted or plain rows" << '\n';
    if (encrypted == 0) {
        remove(tmpPath.c_str());
        return 0;
    }
    if (rename(tmpPath.c_str(), path.c_str()) != 0) {
        cout << "Error: could not replace " << path << '\n';
        return 0;
    }
    return encrypted;
}

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

int main(int argc, char* argv[]) {
    // cout optimisation
    cin.tie(nullptr);
    ios_base::sync_with_stdio(false);
    // initialize sodium
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
        printBucket(phoneToRemove, key);
    }

    // removing the key from memory
    sodium_memzero(key, sizeof key);
    return 0;
}
