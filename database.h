#pragma once
#include <string>
#include "contact.h"

struct Item {
    std::string hash = "";
    Contact contact;
    
    std::string getHash() const;
    void setHash(const std::string& newHash);
};

struct Database {
    //TODO
};