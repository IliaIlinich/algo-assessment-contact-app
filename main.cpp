// Created by Ilia 21/10/2026
// Algorithms And Data Structures-26AUTHW assessment

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "Contact.h"

#define ll long long
#define ull unsigned long long
#define FNVoffset 14695981039346656037ULL
#define FNVprime 1099511628211ULL

constexpr size_t TABLE_SIZE = 1ULL << 20;

std::vector<std::vector<Contact>> table(TABLE_SIZE);

// Generating hash using FNV-1a
ull generateHash(const Contact& contactIn) {
    const std::string& phone = contactIn.phone;
    ull hash = FNVoffset;
    for (char c : phone) {
        if (c != '+' && c != ' ' && c != '-') {
            hash ^= static_cast<unsigned char>(c);
            hash *= FNVprime;
        }
    }
    return hash;
}

// Reads contacts from a CSV file (firstName,surname,email,phone,address) into the table.
// Fields are assigned directly because the setters print a message per call.
size_t loadContactsFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cout << "Error: could not open " << path << '\n';
        return 0;
    }

    std::string line;
    std::getline(file, line); // skip header
    size_t count = 0;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        Contact contact;
        std::getline(ss, contact.firstName, ',');
        std::getline(ss, contact.surname, ',');
        std::getline(ss, contact.email, ',');
        std::getline(ss, contact.phone, ',');
        std::getline(ss, contact.address); // last field, may contain commas

        size_t bucket = generateHash(contact) % (TABLE_SIZE - 1);
        table[bucket].push_back(contact);
        count++;
    }
    return count;
}

// TODO: make it so new contacts are being put into the .csv file and the one removed are removed from .csv file as well

int main(int argc, char* argv[]) {
    // cout optimisation
    std::cin.tie(nullptr);
    std::ios_base::sync_with_stdio(false);
    // loading data from contacts.csv
    std::string contactsPath = argc > 1 ? argv[1] : "../contacts.csv";
    size_t loaded = loadContactsFromFile(contactsPath);
    std::cout << "Loaded " << loaded << " contacts from " << contactsPath << '\n';
    // adding a test contact for testing
    Contact testContact;
    testContact.setFirstName("Eli");
    testContact.setSurname("Smith");
    testContact.setEmail("eli@example.com");
    testContact.setPhone("+44 7700 900123");
    testContact.setAddress("Test city, Test street, AA11 1AA");
    // hash generation for test contact and adding it to database
    ull hash = generateHash(testContact);
    size_t bucket = hash % (TABLE_SIZE - 1);
    table[bucket].push_back(testContact);
    // finding the added test contact in the database
    std::cout << "Hash: " << hash << " Bucket: " << bucket << '\n';
    for (const Contact& c : table[bucket]) {
        std::cout << c.getFirstName() << " " << c.getSurname() << " "
                << c.getEmail() << " " << c.getPhone() << '\n';
    }
    
    return 0;
}

// TODO add encryption library to encrypt the user data in the file, keep FNV-1a for quick data access.