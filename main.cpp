// Created by Ilia 21/10/2026
// Algorithms And Data Structures-26AUTHW assessment

#include "Contact.h"
#include <iostream>
using namespace std;

int main() {
    string name, phone, email;
    cout << "Enter name, phone and email: ";
    cin >> name >> phone >> email;

    Contact c1(name, phone, email);

    cout << "Name:  " << c1.name() << endl;
    cout << "Phone: " << c1.phone() << endl;
    cout << "Email: " << c1.email() << endl;

    c1.setEmail("1234@1234.com");
    cout << c1.email() << endl;

    return 0;
}