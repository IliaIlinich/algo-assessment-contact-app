# Tests

All tests are in `tests.cpp`. They use a small test runner written in the file itself, so they need nothing apart from libsodium.

## Running

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

`ctest` runs two tests: `unit_tests` (`./contact_tests`) and `million_contacts` (`./contact_tests --large`).
Every test works on its own temporary file in the system temp folder, so `contacts.csv` and its key are never touched.

## Test cases

| Area | Test | Type | Expected result |
|---|---|---|---|
| Hash | FNV-1a matches official test values (`""`, `"a"`, `"foobar"`) | normal | hashes equal the published 64-bit FNV-1a values |
| Hash | FNV-1a is deterministic | normal | same input gives same hash, different input gives different hash |
| Hash | bucket index always inside the table | boundary | biggest possible 64-bit hash still gives an index < 2^20 |
| Input | phone normalising | boundary | `+`, spaces and `-` removed, empty input stays empty |
| Encryption | row round trip | normal | decrypted text equals the original |
| Encryption | row with empty text | boundary | empty string encrypts and decrypts correctly |
| Encryption | row uses a random nonce | normal | same text gives a different ciphertext each time |
| Encryption | row with the wrong key | failure | decryption fails |
| Encryption | tampered row | failure | changing one character makes authentication fail |
| Encryption | broken rows (empty, no comma, not base64, too short) | failure | all rejected without crashing |
| Encryption | phone encryption is deterministic | normal | same phone always gives the same ciphertext, so it can be hashed |
| Encryption | phone round trip and wrong key | normal / failure | decrypts with the right key only |
| Encryption | contact round trip, address with commas | boundary | every field comes back unchanged |
| Encryption | values larger than 64 bits (500-digit phone, 100 KB address) | boundary | stored and read back unchanged |
| Key | key is created and loaded back | normal | new 32-byte key file, same key on second load |
| Key | corrupted key file | failure | rejected |
| Table | find in an empty database | boundary | nothing found, no crash |
| Table | add then find (phone typed in different formats) | normal | contact found every time |
| Table | find a contact that doesn't exist | failure | not found |
| Table | duplicate phone (typed differently) | failure | rejected, file and table unchanged |
| Table | contact with empty fields | failure | rejected, file and table unchanged |
| Table | remove a contact | normal | gone from table and file, others still found, temp file cleaned up |
| Table | remove a contact that doesn't exist | failure | returns false, file unchanged |
| Table | remove the last contact | boundary | table and file empty afterwards |
| Table | two contacts in the same bucket (a real collision is searched for) | boundary | both found, removing one leaves the other |
| File | load an empty file | boundary | 0 contacts |
| File | load a missing file | failure | 0 contacts, no crash |
| File | contacts are found again after restart (100 contacts) | normal | all 100 loaded and found |
| File | plain rows get encrypted | normal | rows encrypted once, no plain text left, contacts findable |
| File | wrong key finds nothing | failure | contact not found |
| Scale | 1,000,000 contacts | scale | every searched contact found, missing ones not found, longest chain ≤ 15 |

## Results

All 30 unit tests pass. The 1,000,000 contact test (Release build, Apple Silicon Mac) gave:

| Measure | Value |
|---|---|
| Load factor | 0.954 |
| Buckets used | 644,270 of 1,048,576 (61.4%) |
| Expected for a perfectly uniform hash, 1 - e^(-α) | 61.5% |
| Longest chain | 8 |
| Insert (encrypt + hash + store) | ~4.3 µs per contact |
| Search (encrypt phone + hash + scan bucket + decrypt) | ~3.5 µs per search |

The number of used buckets is almost exactly what a perfectly uniform hash would give. That shows FNV-1a spreads the encrypted phones evenly, and the longest chain staying at 8 shows the O(n) worst case doesn't happen in practice.
