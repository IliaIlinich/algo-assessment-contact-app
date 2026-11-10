// this file is made for contact struct methods and other functions related to work with contacts

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "Contact.h"
#include "Encryption.h"
#include "database.h"

using namespace std;

// getters return a message if the field is empty, setters don't accept empty values
string Contact::getFirstName() const {
    if (firstName.empty()) {
        return "Name field is empty";
    }
    return firstName;
}

void Contact::setFirstName(const string& newFirstName) {
    try {
        if (newFirstName.empty()) {
            throw invalid_argument("First name cannot be empty");
        }
        firstName = newFirstName;
        cout << "First name successfully changed" << endl;
    } catch (const invalid_argument& e) {
        cout << "Error: " << e.what() << endl;
    }
}

string Contact::getSurname() const {
    if (surname.empty()) {
        return "Surname field is empty";
    }
    return surname;
}

void Contact::setSurname(const string& newSurname) {
    try {
        if (newSurname.empty()) {
            throw invalid_argument("Surname cannot be empty");
        }
        surname = newSurname;
        cout << "Surname successfully changed" << endl;
    } catch (const invalid_argument& e) {
        cout << "Error: " << e.what() << endl;
    }
}

string Contact::getEmail() const {
    if (email.empty()) {
        return "Email is empty";
    }
    return email;
}

void Contact::setEmail(const string& newEmail) {
    try {
        if (newEmail.empty()) {
            throw invalid_argument("Email cannot be empty");
        }
        email = newEmail;
        cout << "Email successfully changed" << endl;
    } catch (const invalid_argument& e) {
        cout << "Error: " << e.what() << endl;
    }
}

string Contact::getPhone() const {
    if (phone.empty()) {
        return "Phone field is empty";
    }
    return phone;
}

void Contact::setPhone(const string& newPhone) {
    try {
        if (newPhone.empty()) {
            throw invalid_argument("Phone cannot be empty");
        }
        phone = newPhone;
        cout << "Phone successfully changed" << endl;
    } catch (const invalid_argument& e) {
        cout << "Error: " << e.what() << endl;
    }
}

string Contact::getAddress() const {
    if (address.empty()) {
        return "Address field is empty";
    }
    return address;
}

void Contact::setAddress(const string& newAddress) {
    try {
        if (newAddress.empty()) {
            throw invalid_argument("Address cannot be empty");
        }
        address = newAddress;
        cout << "Address successfully changed" << endl;
    } catch (const invalid_argument& e) {
        cout << "Error: " << e.what() << endl;
    }
}

// checks for empty fields and duplicates, then saves the encrypted contact to the file and the table
bool addContact(const Contact& contact, const string& path, const uc key[KEY_BYTES]) {
    if (contact.firstName.empty() || contact.surname.empty() || contact.email.empty() || contact.phone.empty() || contact.address.empty()) {
        cout << "Error: contact has empty fields, not added" << endl;
        return false;
    }
    string row = encryptContact(contact, key);
    string encPhone = getEncPhone(row);
    size_t bucket = generateHash(encPhone) % (TABLE_SIZE - 1);
    // checking that the phone isn't in the table already
    for (const string& r : table[bucket]) {
        if (getEncPhone(r) == encPhone) {
            cout << "Error: contact with phone " << contact.phone << " already exists" << endl;
            return false;
        }
    }
    ofstream out(path, ios::app);
    if (!out) {
        cout << "Error: could not open " << path << endl;
        return false;
    }
    out << row << endl;
    table[bucket].push_back(row);
    return true;
}

// removes the contact from the file first and then from the table, so they still match if the file can't be changed
bool removeContact(const string& phoneIn, const string& path, const uc key[KEY_BYTES]) {
    string encPhone = encryptPhone(normalisePhone(phoneIn), key);
    size_t bucket = generateHash(encPhone) % (TABLE_SIZE - 1);
    vector<string>& rows = table[bucket];
    size_t rowIndex = rows.size();
    for (size_t i = 0; i < rows.size(); i++) {
        if (getEncPhone(rows[i]) == encPhone) {
            rowIndex = i;
            break;
        }
    }
    if (rowIndex == rows.size()) {
        cout << "Error: no contact with phone " << phoneIn << endl;
        return false;
    }
    ifstream in(path);
    if (!in) {
        cout << "Error: could not open " << path << endl;
        return false;
    }
    // writing all rows except the removed one to a tmp file, which then replaces the original
    string tmpPath = path + ".tmp";
    ofstream out(tmpPath);
    if (!out) {
        cout << "Error: could not create " << tmpPath << endl;
        return false;
    }
    string line;
    getline(in, line); // copy header as it is
    out << line << endl;
    bool removedFromFile = false;
    while (getline(in, line)) {
        if (line.empty()) continue;
        // comparing encrypted phones, so no row has to be decrypted
        if (!removedFromFile && getEncPhone(line) == encPhone) {
            removedFromFile = true;
            continue;
        }
        out << line << endl;
    }
    in.close();
    out.close();
    if (!removedFromFile) {
        remove(tmpPath.c_str());
        rows.erase(rows.begin() + rowIndex);
        cout << "Warning: contact was in memory but not in " << path << endl;
        return true;
    }
    // std::filesystem::rename replaces the old file on every OS (C rename fails on Windows if it exists)
    error_code renameError;
    filesystem::rename(tmpPath, path, renameError);
    if (renameError) {
        cout << "Error: could not replace " << path << endl;
        return false;
    }
    rows.erase(rows.begin() + rowIndex);
    return true;
}

// encrypts the phone, finds its bucket and decrypts only the matching row
bool findContact(const string& phoneIn, const uc key[KEY_BYTES], Contact& contactOut) {
    string encPhone = encryptPhone(normalisePhone(phoneIn), key);
    size_t bucket = generateHash(encPhone) % (TABLE_SIZE - 1);
    for (const string& r : table[bucket]) {
        if (getEncPhone(r) == encPhone) {
            return decryptContact(r, key, contactOut);
        }
    }
    return false;
}

// turns a plain csv line into a contact
Contact contactFromLine(const string& line) {
    stringstream ss(line);
    Contact contact;
    getline(ss, contact.firstName, ',');
    getline(ss, contact.surname, ',');
    getline(ss, contact.email, ',');
    getline(ss, contact.phone, ',');
    getline(ss, contact.address); // last field, may contain commas
    return contact;
}

// row format: encPhone,nonce,encRest where rest is firstName,surname,email,address
string encryptContact(const Contact& contact, const uc key[KEY_BYTES]) {
    string rest = contact.firstName + "," + contact.surname + "," + contact.email + "," + contact.address;
    return encryptPhone(normalisePhone(contact.phone), key) + "," + encryptRow(rest, key);
}

// reverse of encryptContact, phone and the rest are encrypted separately so they get decrypted separately
bool decryptContact(const string& row, const uc key[KEY_BYTES], Contact& contactOut) {
    size_t comma = row.find(',');
    if (comma == string::npos) return false;
    string rest;
    if (!decryptPhone(row.substr(0, comma), key, contactOut.phone)) return false;
    if (!decryptRow(row.substr(comma + 1), key, rest)) return false;
    stringstream ss(rest);
    getline(ss, contactOut.firstName, ',');
    getline(ss, contactOut.surname, ',');
    getline(ss, contactOut.email, ',');
    getline(ss, contactOut.address); // last field, may contain commas
    return true;
}

// encrypted phone is the first field of the row, so it can be read without decrypting anything
string getEncPhone(const string& row) {
    return row.substr(0, row.find(','));
}

// rows stay encrypted in memory, only the encrypted phone is used for the hash
size_t loadEncryptedContactsFromFile(const string& encPath) {
    ifstream file(encPath);
    if (!file) {
        cout << "Error: could not open " << encPath << endl;
        return 0;
    }
    string line;
    getline(file, line); // skip header
    size_t count = 0;
    while (getline(file, line)) {
        if (line.empty()) continue;
        size_t bucket = generateHash(getEncPhone(line)) % (TABLE_SIZE - 1);
        table[bucket].push_back(line);
        count++;
    }
    return count;
}
