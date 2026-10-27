#pragma once

#include <string>

struct contact {
    std::string firstName = "";
    std::string surname = "";
    std::string email = "";
    std::string phone = "";

    std::string getFirstName() const;
    void setFirstName(const std::string& newFirstName);
    std::string getSurname() const;
    void setSurname(const std::string& newSurname);
    std::string getEmail() const;
    void setEmail(const std::string& newEmail);
    std::string getPhone() const;
    void setPhone(const std::string& newPhone);
};
