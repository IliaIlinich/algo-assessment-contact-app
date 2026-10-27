// Created by Ilia 21/10/2026
// Algorithms And Data Structures-26AUTHW assessment

#include <iostream>
#include "contact.h"

int main() {
    std::cin.tie(nullptr);
    std::ios_base::sync_with_stdio(false);

    contact testContact;
    testContact.setFirstName("Eli");
    std::cout << testContact.getFirstName() << std::endl;

    return 0;
}
