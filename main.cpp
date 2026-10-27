// Created by Ilia 21/10/2026
// Algorithms And Data Structures-26AUTHW assessment

#include <iostream>
#include <string>
#include <vector>
#include "contact.h"

#define ll long long
#define ull unsigned long long
#define FNVoffset 14695981039346656037ULL
#define FNVprime 1099511628211ULL

const size_t TABLE_SIZE = 1009;

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

int main() {
    std::cin.tie(nullptr);
    std::ios_base::sync_with_stdio(false);

    std::vector<std::vector<Contact>> table(TABLE_SIZE);

    Contact testContact;
    testContact.setFirstName("Eli");
    testContact.setSurname("Smith");
    testContact.setEmail("eli@example.com");
    testContact.setPhone("+44 7700 900123");

    ull hash = generateHash(testContact);
    size_t bucket = hash % TABLE_SIZE;
    table[bucket].push_back(testContact);

    std::cout << "Hash: " << hash << " Bucket: " << bucket << std::endl;
    for (const Contact& c : table[bucket]) {
        std::cout << c.getFirstName() << " " << c.getSurname() << " "
                  << c.getEmail() << " " << c.getPhone() << std::endl;
    }
    return 0;
}
