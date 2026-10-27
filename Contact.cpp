#include "contact.h"

#include <iostream>
#include <stdexcept>

std::string Contact::getFirstName() const {
    if (firstName.empty()) {
        return "Name field is empty";
    }
    return firstName;
}

void Contact::setFirstName(const std::string& newFirstName) {
    try {
        if (newFirstName.empty()) {
            throw std::invalid_argument("First name cannot be empty");
        }
        firstName = newFirstName;
        std::cout << "First name successfully changed" << std::endl;
    } catch (const std::invalid_argument& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

std::string Contact::getSurname() const {
    if (surname.empty()) {
        return "Surname field is empty";
    }
    return surname;
}

void Contact::setSurname(const std::string& newSurname) {
    try {
        if (newSurname.empty()) {
            throw std::invalid_argument("Surname cannot be empty");
        }
        surname = newSurname;
        std::cout << "Surname successfully changed" << std::endl;
    } catch (const std::invalid_argument& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

std::string Contact::getEmail() const {
    if (email.empty()) {
        return "Email is empty";
    }
    return email;
}

void Contact::setEmail(const std::string& newEmail) {
    try {
        if (newEmail.empty()) {
            throw std::invalid_argument("Email cannot be empty");
        }
        email = newEmail;
        std::cout << "Email successfully changed" << std::endl;
    } catch (const std::invalid_argument& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

std::string Contact::getPhone() const {
    if (phone.empty()) {
        return "Phone field is empty";
    }
    return phone;
}

void Contact::setPhone(const std::string& newPhone) {
    try {
        if (newPhone.empty()) {
            throw std::invalid_argument("Phone cannot be empty");
        }
        phone = newPhone;
        std::cout << "Phone successfully changed" << std::endl;
    } catch (const std::invalid_argument& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}
