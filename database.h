// this file is made for inclusion and easier view on all functions related to database operation

#pragma once

#include <string>
#include "Encryption.h"

#define ull unsigned long long
// constants for the 64 bit FNV-1a hash
#define FNVoffset 14695981039346656037ULL
#define FNVprime 1099511628211ULL

using namespace std;

// generating hash using FNV-1a from the encrypted phone
ull generateHash(const string& encPhone);
// strips '+', ' ' and '-' so the same phone always encrypts to the same encPhone
string normalisePhone(const string& phone);
// prints the bucket info and the contact with this phone upon searching for a user in the database
void printBucket(const string& phone, const uc key[KEY_BYTES]);
