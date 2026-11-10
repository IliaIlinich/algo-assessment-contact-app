// Created by Ilia 21/10/2026
// Algorithms And Data Structures-26AUTHW assessment

#include <iostream>
#include <string>
#include <vector>
#include "Contact.h"
#include "Encryption.h"
#include "commands.h"
#include "tui.h"

using namespace std;

// the hash table with all encrypted rows, declared in Contact.h
vector<vector<string>> table(TABLE_SIZE);

int main(int argc, char* argv[]) {
    // initialise sodium
    if (sodium_init() < 0) {
        cerr << "Error: libsodium failed to initialise" << endl;
        return 1;
    }
    App app;
    // contacts.csv holds the encrypted rows, contacts_test.csv is plain test data to copy rows from
    app.contactsPath = argc > 1 ? argv[1] : "../contacts.csv";
    if (!app.ui.start()) return 1;
    app.ui.setCommands(COMMAND_NAMES);
    app.ui.setInfo("loading...");
    app.ui.print("Loading contacts from " + app.contactsPath + " ...", LineKind::Info);
    app.ui.redraw();

    if (!loadData(app)) {
        // stop() prints the error that was written to the log
        app.ui.stop();
        sodium_memzero(app.key, sizeof app.key);
        return 1;
    }
    updateInfo(app);

    // main loop: read a command from the bottom line and run it
    bool running = true;
    while (running) {
        string line;
        InputResult r = app.ui.readLine(":", line, true);
        if (r == InputResult::Quit) break;
        if (r == InputResult::Cancelled || trim(line).empty()) continue;
        app.ui.print(":" + trim(line), LineKind::Command);
        running = runCommand(app, line);
    }

    app.ui.stop();
    // removing the key from memory
    sodium_memzero(app.key, sizeof app.key);
    return 0;
}
