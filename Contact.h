// Contact class its variables and methods for inclusion

#pragma once

#include <string>

struct Contact {
    std::string firstName = "";
    std::string surname = "";
    std::string email = "";
    std::string phone = "";
    std::string address = "";

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
