#pragma once
#include <string>
using namespace std;

class Contact {
public:
    Contact(string name, string phone, string email);

    const string& name() const {
        return name_;
    }
    const string& phone() const {
        return phone_;
    }
    const string& email() const {
        return email_;
    }
    void setName(string name);
    void setPhone(string phone);
    void setEmail(string email);
private:
    string name_;
    string phone_;
    string email_;
};
