// this file is made for inclusion and easier view on all functions related to encryption using libSodium

#pragma once

#include <sodium.h>
#include <string>

#define keyBytes crypto_aead_xchacha20poly1305_ietf_KEYBYTES
#define uc unsigned char

constexpr size_t KEY_BYTES = keyBytes;

using namespace std;

// random nonce, so the same text gives a different result every time; returns nonce,ciphertext in base64
string encryptRow(const string& plain, const uc key[KEY_BYTES]);
// reverse of encryptRow, returns false if the row is broken, was changed or the key is wrong
bool decryptRow(const string& row, const uc key[KEY_BYTES], string& plainOut);
// deterministic encryption: nonce is a keyed hash of the phone, so the same phone always
// gives the same ciphertext and it can be hashed for the table lookup
string encryptPhone(const string& phone, const uc key[KEY_BYTES]);
// reverse of encryptPhone, the nonce is taken from the start of the decoded field
bool decryptPhone(const string& encPhone, const uc key[KEY_BYTES], string& phoneOut);
// reads the key from the file, or generates and saves a new one if there is no file yet
bool loadOrCreateKey(const string& keyPath, uc key[KEY_BYTES]);
// plain rows are firstName,surname,email,phone,address (address may contain commas)
bool looksLikePlainRow(const string& line);
// encrypted rows are encPhone,nonce,encRest (base64 has no commas)
bool looksLikeEncryptedRow(const string& line);
// encrypts plain rows found in the file and keeps the encrypted ones, returns the amount of newly encrypted rows
size_t encryptPlainRows(const string& path, const uc key[KEY_BYTES]);
