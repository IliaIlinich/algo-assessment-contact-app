#include "Encryption.h"
#include <cstring>
#include <vector>

using namespace std;

constexpr size_t NONCE_BYTES = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
constexpr size_t TAG_BYTES = crypto_aead_xchacha20poly1305_ietf_ABYTES;
constexpr int B64_VARIANT = sodium_base64_VARIANT_ORIGINAL;

static string toB64(const unsigned char* data, size_t len) {
    string out(sodium_base64_ENCODED_LEN(len, B64_VARIANT), '\0');
    sodium_bin2base64(out.data(), out.size(), data, len, B64_VARIANT);
    out.resize(out.size() - 1);
    return out;
}

static bool fromB64(const string& text, vector<unsigned char>& out) {
    out.resize(text.size());
    size_t binLen = 0;
    const char* end = nullptr;
    if (sodium_base642bin(out.data(), out.size(), text.data(), text.size(), nullptr, &binLen, &end, B64_VARIANT) != 0) {
        return false;
    }
    if (end != text.data() + text.size()) return false;
    out.resize(binLen);
    return true;
}

string encryptRow(const string& plain, const unsigned char key[KEY_BYTES]) {
    unsigned char nonce[NONCE_BYTES];
    randombytes_buf(nonce, sizeof nonce);

    vector<unsigned char> ct(plain.size() + TAG_BYTES);
    unsigned long long ctLen = 0;
    crypto_aead_xchacha20poly1305_ietf_encrypt(ct.data(), &ctLen, reinterpret_cast<const unsigned char*>(plain.data()), plain.size(), nullptr, 0, nullptr, nonce, key);

    return toB64(nonce, sizeof nonce) + "," + toB64(ct.data(), ctLen);
}

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
