// this file shows all functions related to encryption in database using libSodium

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "Contact.h"
#include "Encryption.h"

using namespace std;

// nonce and tag sizes from libSodium and the base64 type used in the file
constexpr size_t NONCE_BYTES = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
constexpr size_t TAG_BYTES = crypto_aead_xchacha20poly1305_ietf_ABYTES;
constexpr int B64_VARIANT = sodium_base64_VARIANT_ORIGINAL;

// converts binary data to base64 text, so it can be stored in the csv file
static string toB64(const unsigned char* data, size_t len) {
    string out(sodium_base64_ENCODED_LEN(len, B64_VARIANT), '\0');
    sodium_bin2base64(out.data(), out.size(), data, len, B64_VARIANT);
    out.resize(out.size() - 1); // removing the '\0' sodium adds at the end
    return out;
}

// converts base64 text back to binary, returns false if the text is not valid base64
static bool fromB64(const string& text, vector<unsigned char>& out) {
    out.resize(text.size()); // binary is always shorter than its base64, so this is enough
    size_t binLen = 0;
    const char* end = nullptr;
    if (sodium_base642bin(out.data(), out.size(), text.data(), text.size(), nullptr, &binLen, &end, B64_VARIANT) != 0) {
        return false;
    }
    // sodium stops at the first invalid character, so checking that all of the text was read
    if (end != text.data() + text.size()) return false;
    out.resize(binLen);
    return true;
}

// random nonce, so the same text gives a different result every time; returns nonce,ciphertext in base64
string encryptRow(const string& plain, const unsigned char key[KEY_BYTES]) {
    unsigned char nonce[NONCE_BYTES];
    randombytes_buf(nonce, sizeof nonce);

    vector<unsigned char> ct(plain.size() + TAG_BYTES);
    unsigned long long ctLen = 0;
    crypto_aead_xchacha20poly1305_ietf_encrypt(ct.data(), &ctLen, reinterpret_cast<const unsigned char*>(plain.data()), plain.size(), nullptr, 0, nullptr, nonce, key);

    return toB64(nonce, sizeof nonce) + "," + toB64(ct.data(), ctLen);
}

// reverse of encryptRow, returns false if the row is broken, was changed or the key is wrong
bool decryptRow(const string& row, const unsigned char key[KEY_BYTES], string& plainOut) {
    size_t comma = row.find(',');
    if (comma == string::npos) return false;

    vector<unsigned char> nonce, ct;
    if (!fromB64(row.substr(0, comma), nonce) || nonce.size() != NONCE_BYTES) return false;
    if (!fromB64(row.substr(comma + 1), ct) || ct.size() < TAG_BYTES) return false;

    vector<unsigned char> plain(ct.size() - TAG_BYTES);
    unsigned long long plainLen = 0;
    if (crypto_aead_xchacha20poly1305_ietf_decrypt(
            plain.data(), &plainLen, nullptr,
            ct.data(), ct.size(), nullptr, 0, nonce.data(), key) != 0) {
        return false;
    }

    plainOut.assign(reinterpret_cast<const char*>(plain.data()), plainLen);
    return true;
}


// deterministic encryption: nonce is a keyed hash of the phone, so the same phone always
// gives the same ciphertext and it can be hashed for the table lookup
string encryptPhone(const string& phone, const unsigned char key[KEY_BYTES]) {
    unsigned char nonce[NONCE_BYTES];
    crypto_generichash(nonce, sizeof nonce, reinterpret_cast<const unsigned char*>(phone.data()), phone.size(), key, KEY_BYTES);

    vector<unsigned char> out(NONCE_BYTES + phone.size() + TAG_BYTES);
    memcpy(out.data(), nonce, NONCE_BYTES);
    unsigned long long ctLen = 0;
    crypto_aead_xchacha20poly1305_ietf_encrypt(out.data() + NONCE_BYTES, &ctLen, reinterpret_cast<const unsigned char*>(phone.data()), phone.size(), nullptr, 0, nullptr, nonce, key);

    return toB64(out.data(), NONCE_BYTES + ctLen); // nonce||ct in one field
}

// reverse of encryptPhone, the nonce is taken from the start of the decoded field
bool decryptPhone(const string& encPhone, const unsigned char key[KEY_BYTES], string& phoneOut) {
    vector<unsigned char> data;
    if (!fromB64(encPhone, data) || data.size() < NONCE_BYTES + TAG_BYTES) return false;

    vector<unsigned char> plain(data.size() - NONCE_BYTES - TAG_BYTES);
    unsigned long long plainLen = 0;
    if (crypto_aead_xchacha20poly1305_ietf_decrypt(
            plain.data(), &plainLen, nullptr,
            data.data() + NONCE_BYTES, data.size() - NONCE_BYTES, nullptr, 0, data.data(), key) != 0) {
        return false;
    }

    phoneOut.assign(reinterpret_cast<const char*>(plain.data()), plainLen);
    return true;
}

// reads the key from the file, or generates and saves a new one if there is no file yet
bool loadOrCreateKey(const string& keyPath, uc key[KEY_BYTES]) {
    ifstream in(keyPath, ios::binary);
    if (in) {
        in.read(reinterpret_cast<char*>(key), KEY_BYTES);
        if (in.gcount() != static_cast<streamsize>(KEY_BYTES)) {
            cout << "Error: key file " << keyPath << " is corrupted" << '\n';
            return false;
        }
        return true;
    }
    crypto_aead_xchacha20poly1305_ietf_keygen(key);
    ofstream out(keyPath, ios::binary);
    if (!out) {
        cout << "Error: could not create key file " << keyPath << '\n';
        return false;
    }
    out.write(reinterpret_cast<const char*>(key), KEY_BYTES);
    cout << "New key generated and saved to " << keyPath << '\n';
    return true;
}

// plain rows are firstName,surname,email,phone,address (address may contain commas)
bool looksLikePlainRow(const string& line) {
    size_t commas = 0;
    for (char c : line) {
        if (c == ',') commas++;
    }
    return commas >= 4;
}

// encrypted rows are encPhone,nonce,encRest (base64 has no commas)
bool looksLikeEncryptedRow(const string& line) {
    size_t commas = 0;
    for (char c : line) {
        if (c == ',') commas++;
    }
    return commas == 2;
}

// encrypts plain rows found in the file and keeps the encrypted ones, returns the amount of newly encrypted rows
size_t encryptPlainRows(const string& path, const uc key[KEY_BYTES]) {
    ifstream in(path);
    if (!in) {
        cout << "Error: could not open " << path << '\n';
        return 0;
    }
    // writing to a tmp file, which replaces the original only if something got encrypted
    string tmpPath = path + ".tmp";
    ofstream out(tmpPath);
    if (!out) {
        cout << "Error: could not create " << tmpPath << '\n';
        return 0;
    }
    string line;
    getline(in, line); // copy header as it is
    out << line << '\n';
    size_t encrypted = 0, broken = 0;
    while (getline(in, line)) {
        if (line.empty()) continue;
        if (looksLikePlainRow(line)) {
            out << encryptContact(contactFromLine(line), key) << '\n';
            encrypted++;
        } else if (looksLikeEncryptedRow(line)) {
            out << line << '\n';
        } else {
            // broken rows are kept in the file, so no data gets lost
            out << line << '\n';
            broken++;
        }
    }
    in.close();
    out.close();
    if (broken > 0) cout << "Warning: " << broken << " rows are not valid encrypted or plain rows" << '\n';
    if (encrypted == 0) {
        remove(tmpPath.c_str());
        return 0;
    }
    if (rename(tmpPath.c_str(), path.c_str()) != 0) {
        cout << "Error: could not replace " << path << '\n';
        return 0;
    }
    return encrypted;
}
