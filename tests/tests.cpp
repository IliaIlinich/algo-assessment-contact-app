// this file holds the automated tests for the contact app
// run all normal tests:      ./contact_tests
// run the million row test:  ./contact_tests --large
// a small test runner is written here instead of a framework, so the tests have no extra dependencies

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "../Contact.h"
#include "../Encryption.h"
#include "../database.h"

using namespace std;

// the hash table is normally defined in main.cpp, the tests need their own copy
vector<vector<string>> table(TABLE_SIZE);

// ---------- tiny test runner ----------

static int checksFailed = 0;
static int testsFailed = 0;
static int testsRun = 0;

// records a failed check with the line it happened on, but keeps running the test
#define CHECK(cond)                                                                 \
    do {                                                                            \
        if (!(cond)) {                                                              \
            checksFailed++;                                                         \
            cerr << "    FAILED: " #cond " (line " << __LINE__ << ")" << endl;     \
        }                                                                           \
    } while (0)

// the app prints errors to cout, this hides them while a test runs so the output stays readable
struct SilenceCout {
    stringstream sink;
    streambuf* old;
    SilenceCout() : old(cout.rdbuf(sink.rdbuf())) {}
    ~SilenceCout() { cout.rdbuf(old); }
};

static void runTest(const string& name, const function<void()>& test) {
    int before = checksFailed;
    testsRun++;
    {
        SilenceCout silence;
        test();
    }
    if (checksFailed == before) {
        cout << "[PASS] " << name << endl;
    } else {
        cout << "[FAIL] " << name << endl;
        testsFailed++;
    }
}

// ---------- helpers ----------

static void clearTable() {
    for (auto& bucket : table) bucket.clear();
}

static size_t bucketOf(const string& encPhone) {
    return generateHash(encPhone) % (TABLE_SIZE - 1);
}

static size_t tableSize() {
    size_t total = 0;
    for (const auto& bucket : table) total += bucket.size();
    return total;
}

// every test gets a fresh temporary contacts file with only the header in it
static string freshFile(const string& name) {
    filesystem::path dir = filesystem::temp_directory_path() / "contact_app_tests";
    filesystem::create_directories(dir);
    string path = (dir / name).string();
    ofstream(path) << "firstName,surname,email,phone,address" << endl;
    return path;
}

static size_t countRows(const string& path) {
    ifstream in(path);
    string line;
    getline(in, line); // header
    size_t rows = 0;
    while (getline(in, line)) {
        if (!line.empty()) rows++;
    }
    return rows;
}

static Contact sampleContact(const string& phone = "+44 7786 579303") {
    Contact c;
    c.firstName = "Hannah";
    c.surname = "Ward";
    c.email = "hannah.ward@example.org";
    c.phone = phone;
    c.address = "Leeds, 7 Queen Street, HH18 1ZT";
    return c;
}

static void newKey(uc key[KEY_BYTES]) {
    crypto_aead_xchacha20poly1305_ietf_keygen(key);
}

// ---------- hash function ----------

static void testFnvKnownValues() {
    // official 64 bit FNV-1a test vectors
    CHECK(generateHash("") == 0xcbf29ce484222325ULL);
    CHECK(generateHash("a") == 0xaf63dc4c8601ec8cULL);
    CHECK(generateHash("foobar") == 0x85944171f73967e8ULL);
}

static void testFnvDeterministic() {
    CHECK(generateHash("447786579303") == generateHash("447786579303"));
    CHECK(generateHash("447786579303") != generateHash("447786579304"));
}

static void testBucketIndexInRange() {
    // the index must always fit in the table, even for the biggest possible hash
    CHECK(0xFFFFFFFFFFFFFFFFULL % (TABLE_SIZE - 1) < TABLE_SIZE);
    CHECK(bucketOf("anything") < TABLE_SIZE);
}

// ---------- phone normalising ----------

static void testNormalisePhone() {
    CHECK(normalisePhone("+44 7786-579303") == "447786579303");
    CHECK(normalisePhone("447786579303") == "447786579303");
    CHECK(normalisePhone("") == "");
    CHECK(normalisePhone("+ - ") == "");
}

// ---------- encryption ----------

static void testRowRoundTrip() {
    uc key[KEY_BYTES];
    newKey(key);
    string out;
    CHECK(decryptRow(encryptRow("Hannah,Ward,a@b.c,Leeds", key), key, out));
    CHECK(out == "Hannah,Ward,a@b.c,Leeds");
}

static void testRowEmptyString() {
    uc key[KEY_BYTES];
    newKey(key);
    string out = "not empty";
    CHECK(decryptRow(encryptRow("", key), key, out));
    CHECK(out.empty());
}

static void testRowRandomNonce() {
    // the same text must not give the same ciphertext twice
    uc key[KEY_BYTES];
    newKey(key);
    CHECK(encryptRow("same text", key) != encryptRow("same text", key));
}

static void testRowWrongKey() {
    uc key[KEY_BYTES], otherKey[KEY_BYTES];
    newKey(key);
    newKey(otherKey);
    string out;
    CHECK(!decryptRow(encryptRow("secret", key), otherKey, out));
}

static void testRowTampered() {
    uc key[KEY_BYTES];
    newKey(key);
    string row = encryptRow("secret data", key);
    // changing one character of the ciphertext (after the nonce) must make authentication fail
    char& c = row[row.find(',') + 5];
    c = (c == 'A') ? 'B' : 'A';
    string out;
    CHECK(!decryptRow(row, key, out));
}

static void testRowBrokenInput() {
    uc key[KEY_BYTES];
    newKey(key);
    string out;
    CHECK(!decryptRow("", key, out));
    CHECK(!decryptRow("no comma here", key, out));
    CHECK(!decryptRow("!!!,???", key, out)); // not base64
    CHECK(!decryptRow("AAAA,AAAA", key, out)); // valid base64 but too short
}

static void testPhoneDeterministic() {
    // the phone must always encrypt the same way, otherwise it couldn't be found in the table
    uc key[KEY_BYTES];
    newKey(key);
    CHECK(encryptPhone("447786579303", key) == encryptPhone("447786579303", key));
    CHECK(encryptPhone("447786579303", key) != encryptPhone("447786579304", key));
}

static void testPhoneRoundTripAndKey() {
    uc key[KEY_BYTES], otherKey[KEY_BYTES];
    newKey(key);
    newKey(otherKey);
    string phone;
    CHECK(decryptPhone(encryptPhone("447786579303", key), key, phone));
    CHECK(phone == "447786579303");
    // a different key gives a different ciphertext and can't decrypt the first one
    CHECK(encryptPhone("447786579303", key) != encryptPhone("447786579303", otherKey));
    CHECK(!decryptPhone(encryptPhone("447786579303", key), otherKey, phone));
}

static void testContactRoundTripWithCommas() {
    uc key[KEY_BYTES];
    newKey(key);
    Contact in = sampleContact(), out;
    CHECK(decryptContact(encryptContact(in, key), key, out));
    CHECK(out.firstName == in.firstName);
    CHECK(out.surname == in.surname);
    CHECK(out.email == in.email);
    CHECK(out.phone == "447786579303"); // stored normalised
    CHECK(out.address == in.address);   // address has commas in it
}

static void testLargeValues() {
    // values much bigger than 64 bits must survive encryption unchanged
    uc key[KEY_BYTES];
    newKey(key);
    Contact in = sampleContact(string(500, '9')); // 500 digit phone number
    in.address = string(100000, 'x');            // 100 KB address
    Contact out;
    CHECK(decryptContact(encryptContact(in, key), key, out));
    CHECK(out.phone == in.phone);
    CHECK(out.address == in.address);
}

static void testKeyCreateAndReload() {
    string path = freshFile("key_test.csv") + ".key";
    filesystem::remove(path);
    uc first[KEY_BYTES], second[KEY_BYTES];
    CHECK(loadOrCreateKey(path, first));   // creates the key
    CHECK(filesystem::file_size(path) == KEY_BYTES);
    CHECK(loadOrCreateKey(path, second));  // loads the same key back
    CHECK(sodium_memcmp(first, second, KEY_BYTES) == 0);
}

static void testKeyCorrupted() {
    string path = freshFile("bad_key_test.csv") + ".key";
    ofstream(path, ios::binary) << "too short";
    uc key[KEY_BYTES];
    CHECK(!loadOrCreateKey(path, key));
}

// ---------- add / find / remove ----------

static void testFindInEmptyDatabase() {
    clearTable();
    uc key[KEY_BYTES];
    newKey(key);
    Contact out;
    CHECK(!findContact("447786579303", key, out));
    CHECK(!findContact("", key, out));
}

static void testAddThenFind() {
    clearTable();
    string path = freshFile("add_find.csv");
    uc key[KEY_BYTES];
    newKey(key);
    CHECK(addContact(sampleContact(), path, key));
    CHECK(countRows(path) == 1);
    CHECK(tableSize() == 1);
    Contact out;
    CHECK(findContact("+44 7786 579303", key, out));
    CHECK(out.firstName == "Hannah");
    // a different way of typing the same number still finds it
    CHECK(findContact("447786579303", key, out));
    CHECK(findContact("+44-7786-579303", key, out));
}

static void testFindMissing() {
    clearTable();
    string path = freshFile("find_missing.csv");
    uc key[KEY_BYTES];
    newKey(key);
    CHECK(addContact(sampleContact(), path, key));
    Contact out;
    CHECK(!findContact("447700000000", key, out));
}

static void testAddDuplicateRejected() {
    clearTable();
    string path = freshFile("duplicate.csv");
    uc key[KEY_BYTES];
    newKey(key);
    CHECK(addContact(sampleContact("+44 7786 579303"), path, key));
    // same number, written differently
    CHECK(!addContact(sampleContact("447786579303"), path, key));
    CHECK(countRows(path) == 1);
    CHECK(tableSize() == 1);
}

static void testAddEmptyFieldsRejected() {
    clearTable();
    string path = freshFile("empty_fields.csv");
    uc key[KEY_BYTES];
    newKey(key);
    Contact empty;
    CHECK(!addContact(empty, path, key));
    Contact noAddress = sampleContact();
    noAddress.address = "";
    CHECK(!addContact(noAddress, path, key));
    CHECK(countRows(path) == 0);
    CHECK(tableSize() == 0);
}

static void testRemove() {
    clearTable();
    string path = freshFile("remove.csv");
    uc key[KEY_BYTES];
    newKey(key);
    CHECK(addContact(sampleContact("447700000001"), path, key));
    CHECK(addContact(sampleContact("447700000002"), path, key));
    CHECK(addContact(sampleContact("447700000003"), path, key));
    CHECK(removeContact("447700000002", path, key));
    Contact out;
    CHECK(!findContact("447700000002", key, out));
    CHECK(findContact("447700000001", key, out));
    CHECK(findContact("447700000003", key, out));
    CHECK(countRows(path) == 2);
    CHECK(tableSize() == 2);
    CHECK(!filesystem::exists(path + ".tmp")); // temporary file cleaned up
}

static void testRemoveMissing() {
    clearTable();
    string path = freshFile("remove_missing.csv");
    uc key[KEY_BYTES];
    newKey(key);
    CHECK(addContact(sampleContact(), path, key));
    CHECK(!removeContact("447700000000", path, key));
    CHECK(countRows(path) == 1);
}

static void testRemoveLastThenEmpty() {
    clearTable();
    string path = freshFile("remove_last.csv");
    uc key[KEY_BYTES];
    newKey(key);
    CHECK(addContact(sampleContact(), path, key));
    CHECK(removeContact("447786579303", path, key));
    CHECK(countRows(path) == 0);
    CHECK(tableSize() == 0);
    Contact out;
    CHECK(!findContact("447786579303", key, out));
}

static void testCollisionsInOneBucket() {
    // searching phone numbers until two of them land in the same bucket (a real collision),
    // with about a million buckets this takes around a thousand tries
    clearTable();
    string path = freshFile("collision.csv");
    uc key[KEY_BYTES];
    newKey(key);
    vector<size_t> bucketFor(TABLE_SIZE, SIZE_MAX);
    string phoneA, phoneB;
    for (int i = 0; i < 100000 && phoneB.empty(); i++) {
        string phone = "447" + to_string(200000000 + i);
        size_t bucket = bucketOf(encryptPhone(phone, key));
        if (bucketFor[bucket] != SIZE_MAX) {
            phoneA = "447" + to_string(200000000 + bucketFor[bucket]);
            phoneB = phone;
        }
        bucketFor[bucket] = i;
    }
    CHECK(!phoneB.empty());
    Contact a = sampleContact(phoneA), b = sampleContact(phoneB);
    a.firstName = "Alice";
    b.firstName = "Bob";
    CHECK(addContact(a, path, key));
    CHECK(addContact(b, path, key));
    // both are in one bucket, so the chain has to be searched to tell them apart
    CHECK(table[bucketOf(encryptPhone(phoneA, key))].size() == 2);
    Contact out;
    CHECK(findContact(phoneA, key, out) && out.firstName == "Alice");
    CHECK(findContact(phoneB, key, out) && out.firstName == "Bob");
    // removing one must leave the other one in the bucket
    CHECK(removeContact(phoneA, path, key));
    CHECK(!findContact(phoneA, key, out));
    CHECK(findContact(phoneB, key, out) && out.firstName == "Bob");
}

// ---------- loading from file ----------

static void testLoadEmptyFile() {
    clearTable();
    string path = freshFile("load_empty.csv");
    CHECK(loadEncryptedContactsFromFile(path) == 0);
    CHECK(tableSize() == 0);
}

static void testLoadMissingFile() {
    clearTable();
    CHECK(loadEncryptedContactsFromFile("/this/file/does/not/exist.csv") == 0);
}

static void testReloadAfterRestart() {
    // contacts added in one run must be found again after loading the file in the next run
    clearTable();
    string path = freshFile("reload.csv");
    uc key[KEY_BYTES];
    newKey(key);
    for (int i = 0; i < 100; i++) {
        CHECK(addContact(sampleContact("4477000" + to_string(10000 + i)), path, key));
    }
    clearTable(); // pretend the program was closed
    CHECK(loadEncryptedContactsFromFile(path) == 100);
    Contact out;
    CHECK(findContact("447700010000", key, out));
    CHECK(findContact("447700010099", key, out));
    CHECK(!findContact("447700010100", key, out));
}

static void testEncryptPlainRows() {
    clearTable();
    string path = freshFile("plain.csv");
    {
        ofstream out(path, ios::app);
        out << "Isla,Kim,isla@example.com,+44 7219 540831,Birmingham, 152 Albert Road, BA12 3HS" << endl;
        out << "Ian,Day,ian@example.com,+44 7000 000001,London" << endl;
    }
    uc key[KEY_BYTES];
    newKey(key);
    CHECK(encryptPlainRows(path, key) == 2);
    // running it again must not encrypt anything twice
    CHECK(encryptPlainRows(path, key) == 0);
    // the plain text must not be in the file any more
    ifstream in(path);
    string all((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
    CHECK(all.find("Isla") == string::npos);
    CHECK(all.find("7219") == string::npos);
    CHECK(loadEncryptedContactsFromFile(path) == 2);
    Contact out;
    CHECK(findContact("447219540831", key, out));
    CHECK(out.address == "Birmingham, 152 Albert Road, BA12 3HS");
}

static void testWrongKeyCannotRead() {
    clearTable();
    string path = freshFile("wrong_key.csv");
    uc key[KEY_BYTES], otherKey[KEY_BYTES];
    newKey(key);
    newKey(otherKey);
    CHECK(addContact(sampleContact(), path, key));
    Contact out;
    CHECK(!findContact("447786579303", otherKey, out));
}

// ---------- 1,000,000 records ----------

static void testMillionContacts() {
    clearTable();
    uc key[KEY_BYTES];
    newKey(key);
    const size_t n = 1000000;
    // rows are put straight into the table, writing them to a file isn't what is being tested here
    auto start = chrono::steady_clock::now();
    for (size_t i = 0; i < n; i++) {
        Contact c = sampleContact("447" + to_string(100000000 + i));
        string row = encryptContact(c, key);
        table[bucketOf(getEncPhone(row))].push_back(row);
    }
    double insertSec = chrono::duration<double>(chrono::steady_clock::now() - start).count();
    CHECK(tableSize() == n);

    // bucket statistics
    size_t used = 0, longest = 0;
    for (const auto& bucket : table) {
        if (!bucket.empty()) used++;
        longest = max(longest, bucket.size());
    }
    double load = static_cast<double>(n) / TABLE_SIZE;

    // searching for 10,000 contacts that exist and 10,000 that don't
    start = chrono::steady_clock::now();
    size_t found = 0, notFound = 0;
    Contact out;
    for (size_t i = 0; i < n; i += 100) {
        if (findContact("447" + to_string(100000000 + i), key, out)) found++;
        if (!findContact("448" + to_string(100000000 + i), key, out)) notFound++;
    }
    double findSec = chrono::duration<double>(chrono::steady_clock::now() - start).count();
    CHECK(found == n / 100);
    CHECK(notFound == n / 100);
    // with a load of 0.95 the longest chain should stay very short
    CHECK(longest <= 15);

    clog << "    contacts: " << n << ", load factor: " << load << endl;
    clog << "    buckets used: " << used << " of " << TABLE_SIZE << ", longest chain: " << longest << endl;
    clog << "    insert time: " << insertSec << " s (" << insertSec / n * 1e6 << " us per contact)" << endl;
    clog << "    find time: " << findSec / (2.0 * n / 100) * 1e6 << " us per search" << endl;
    clearTable();
}

int main(int argc, char* argv[]) {
    if (sodium_init() < 0) {
        cerr << "libsodium failed to initialise" << endl;
        return 1;
    }
    bool large = argc > 1 && string(argv[1]) == "--large";

    if (large) {
        runTest("1,000,000 contacts: all found, chains stay short", testMillionContacts);
    } else {
        runTest("FNV-1a matches official test values", testFnvKnownValues);
        runTest("FNV-1a is deterministic", testFnvDeterministic);
        runTest("bucket index always inside the table", testBucketIndexInRange);
        runTest("phone normalising", testNormalisePhone);
        runTest("row encrypt/decrypt round trip", testRowRoundTrip);
        runTest("row with empty text", testRowEmptyString);
        runTest("row uses a random nonce", testRowRandomNonce);
        runTest("row cannot be read with the wrong key", testRowWrongKey);
        runTest("tampered row is rejected", testRowTampered);
        runTest("broken rows are rejected", testRowBrokenInput);
        runTest("phone encryption is deterministic", testPhoneDeterministic);
        runTest("phone round trip and wrong key", testPhoneRoundTripAndKey);
        runTest("contact round trip, address with commas", testContactRoundTripWithCommas);
        runTest("values larger than 64 bits", testLargeValues);
        runTest("key is created and loaded back", testKeyCreateAndReload);
        runTest("corrupted key file is rejected", testKeyCorrupted);
        runTest("find in an empty database", testFindInEmptyDatabase);
        runTest("add then find", testAddThenFind);
        runTest("find a contact that doesn't exist", testFindMissing);
        runTest("duplicate phone is rejected", testAddDuplicateRejected);
        runTest("contact with empty fields is rejected", testAddEmptyFieldsRejected);
        runTest("remove a contact", testRemove);
        runTest("remove a contact that doesn't exist", testRemoveMissing);
        runTest("remove the last contact", testRemoveLastThenEmpty);
        runTest("contacts in the same bucket", testCollisionsInOneBucket);
        runTest("load an empty file", testLoadEmptyFile);
        runTest("load a missing file", testLoadMissingFile);
        runTest("contacts are found again after restart", testReloadAfterRestart);
        runTest("plain rows get encrypted", testEncryptPlainRows);
        runTest("wrong key finds nothing", testWrongKeyCannotRead);
    }

    cout << endl << testsRun - testsFailed << "/" << testsRun << " tests passed" << endl;
    return testsFailed == 0 ? 0 : 1;
}
