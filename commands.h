// this file is made for the commands typed on the app's command line (:add, :find, :remove ...)

#pragma once

#include <string>
#include "Encryption.h"
#include "tui.h"

// everything the commands need
struct App {
    Tui ui;
    std::string contactsPath;
    uc key[KEY_BYTES];
    size_t count = 0;
};

// command names, used for Tab completion
extern const std::vector<std::string> COMMAND_NAMES;

// loads the key and the contacts (encrypting plain rows first), all output goes to the log
bool loadData(App& app);
// shows the file name and the amount of contacts in the status bar
void updateInfo(App& app);
// runs one command line, returns false when the program should quit
bool runCommand(App& app, std::string line);
// removes spaces at both ends
std::string trim(const std::string& s);
