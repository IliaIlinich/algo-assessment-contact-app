#pragma once

#include <sodium.h>
#include <string>

#define keyBytes crypto_aead_xchacha20poly1305_ietf_KEYBYTES
#define uc unsigned char

constexpr size_t KEY_BYTES = keyBytes;

using namespace std;

string encryptRow(const string& plain, const uc key[KEY_BYTES]);
bool decryptRow(const string& row, const uc key[KEY_BYTES], string& plainOut);
string encryptPhone(const string& phone, const uc key[KEY_BYTES]);
bool decryptPhone(const string& encPhone, const uc key[KEY_BYTES], string& phoneOut);
