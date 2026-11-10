// this file shows the commands typed on the app's command line (:add, :find, :remove ...)

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "Contact.h"
#include "Encryption.h"
#include "commands.h"
#include "database.h"
#include "tui.h"

using namespace std;

const vector<string> COMMAND_NAMES = {"add", "find", "remove", "count", "clear", "help", "quit"};

// removes spaces at both ends
string trim(const string& s) {
    size_t start = s.find_first_not_of(' ');
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(' ');
    return s.substr(start, end - start + 1);
}

// shows the file name and the amount of contacts in the status bar
void updateInfo(App& app) {
    app.ui.setInfo(app.contactsPath + "  " + to_string(app.count) + " contacts");
}

// uses the argument if there is one, otherwise asks for it on the command line
static bool argOrAsk(App& app, const string& arg, const string& label, string& out) {
    out = arg;
    if (!out.empty()) return true;
    if (app.ui.readLine(label, out, false) != InputResult::Ok || trim(out).empty()) {
        app.ui.print("Cancelled", LineKind::Warning);
        return false;
    }
    out = trim(out);
    return true;
}

// asks for every field one by one, Esc cancels
static void commandAdd(App& app) {
    Contact c;
    vector<pair<string, string*>> fields = {
        {"First name: ", &c.firstName},
        {"Surname: ", &c.surname},
        {"Email: ", &c.email},
        {"Phone: ", &c.phone},
        {"Address: ", &c.address},
    };
    app.ui.setMode("ADD CONTACT");
    for (auto& field : fields) {
        if (app.ui.readLine(field.first, *field.second, false) != InputResult::Ok) {
            app.ui.setMode("COMMAND");
            app.ui.print("Add cancelled", LineKind::Warning);
            return;
        }
        *field.second = trim(*field.second);
        app.ui.print("  " + field.first + *field.second);
    }
    app.ui.setMode("COMMAND");
    if (addContact(c, app.contactsPath, app.key)) {
        app.count++;
        app.ui.print("Contact added", LineKind::Success);
        printBucket(c.phone, app.key);
    }
}

static void commandFind(App& app, const string& arg) {
    string phone;
    if (!argOrAsk(app, arg, "Phone to find: ", phone)) return;
    printBucket(phone, app.key);
}

// shows the contact first and asks to confirm before removing it
static void commandRemove(App& app, const string& arg) {
    string phone;
    if (!argOrAsk(app, arg, "Phone to remove: ", phone)) return;
    Contact c;
    if (!findContact(phone, app.key, c)) {
        app.ui.print("Error: no contact with phone " + phone, LineKind::Error);
        return;
    }
    string answer;
    string question = "Remove " + c.firstName + " " + c.surname + " (" + c.phone + ")? [y/N] ";
    if (app.ui.readLine(question, answer, false) != InputResult::Ok || (trim(answer) != "y" && trim(answer) != "Y")) {
        app.ui.print("Remove cancelled", LineKind::Warning);
        return;
    }
    // rewriting a big file takes a moment, so the status bar shows it
    app.ui.setInfo("removing...");
    app.ui.redraw();
    if (removeContact(phone, app.contactsPath, app.key)) {
        app.count--;
        app.ui.print("Contact " + phone + " has been successfully deleted", LineKind::Success);
    }
}

// runs one command line, returns false when the program should quit
bool runCommand(App& app, string line) {
    line = trim(line);
    while (!line.empty() && line[0] == ':') line.erase(0, 1);
    if (line.empty()) return true;
    size_t space = line.find(' ');
    string name = line.substr(0, space);
    string arg = space == string::npos ? "" : trim(line.substr(space + 1));

    if (name == "q" || name == "q!" || name == "quit" || name == "exit" || name == "wq" || name == "x") return false;
    if (name == "add" || name == "a") commandAdd(app);
    else if (name == "find" || name == "f") commandFind(app, arg);
    else if (name == "remove" || name == "rm" || name == "delete" || name == "d") commandRemove(app, arg);
    else if (name == "count") app.ui.print(to_string(app.count) + " contacts loaded", LineKind::Info);
    else if (name == "clear" || name == "cls") app.ui.clearLog();
    else if (name == "help" || name == "h") app.ui.printHelp();
    else app.ui.print("E492: Not a command: " + name + " (type :help)", LineKind::Error);
    updateInfo(app);
    return true;
}

// loads the key and the contacts (encrypting plain rows first), all output goes to the log
bool loadData(App& app) {
    string keyPath = app.contactsPath + ".key";
    // loading the key (created on first run)
    if (!loadOrCreateKey(keyPath, app.key)) return false;
    // creating an empty contacts file (header only) if it doesn't exist yet
    if (!ifstream(app.contactsPath)) {
        ofstream(app.contactsPath) << "firstName,surname,email,phone,address" << endl;
        cout << "Created empty " << app.contactsPath << endl;
    }
    // checking for unencrypted records and encrypting them
    size_t encrypted = encryptPlainRows(app.contactsPath, app.key);
    if (encrypted > 0) {
        cout << "Found and encrypted " << encrypted << " unencrypted contacts in " << app.contactsPath << endl;
    }
    // loading data from the encrypted file
    app.count = loadEncryptedContactsFromFile(app.contactsPath);
    app.ui.print("Loaded " + to_string(app.count) + " contacts from " + app.contactsPath, LineKind::Success);
    return true;
}
