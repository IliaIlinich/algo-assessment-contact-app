#include "Contact.h"

Contact::Contact(string name, string phone, string email)
    : name_(name), phone_(phone), email_(email) {}

void Contact::setName(string name) {
    name_ = name;
}

void Contact::setPhone(string phone) {
    phone_ = phone;
}

void Contact::setEmail(string email) {
    email_ = email;
}
