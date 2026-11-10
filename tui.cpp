// this file shows the vim-like terminal interface, built with ncurses

// curses.h is ncurses on macOS/Linux and PDCurses on Windows, both have the same functions
#include <curses.h>
#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#else
#include <unistd.h>
#endif
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "tui.h"

using namespace std;

// colour pair numbers
enum { C_LOGO = 1, C_COMMAND, C_ERROR, C_WARNING, C_SUCCESS, C_INFO, C_TILDE, C_HELPKEY };

static const vector<string> LOGO = {
    "  ____ ___  _   _ _____  _    ____ _____ ____  ",
    " / ___/ _ \\| \\ | |_   _|/ \\  / ___|_   _/ ___| ",
    "| |  | | | |  \\| | | | / _ \\| |     | | \\___ \\ ",
    "| |__| |_| | |\\  | | |/ ___ \\ |___  | |  ___) |",
    " \\____\\___/|_| \\_| |_/_/   \\_\\____| |_| |____/ ",
};
static const string SUBTITLE = "encrypted contact book - libsodium + FNV-1a hash table";

// command and its description, shown under the logo and by :help
static const vector<pair<string, string>> HELP = {
    {":add", "add a new contact (asks for each field)"},
    {":find <phone>", "look up a contact by phone number"},
    {":remove <phone>", "delete a contact by phone number"},
    {":count", "show how many contacts are loaded"},
    {":clear", "clear the output log"},
    {":help", "show this help in the log"},
    {":q", "quit"},
};
static const string KEYS = "Up/Down history | PgUp/PgDn scroll | Tab complete | Esc cancel";

static const size_t MAX_LOG_LINES = 5000;
static const size_t MAX_HISTORY = 500;

// splits text into lines that fit the width, breaking on spaces where possible
static vector<string> wrap(const string& text, size_t width) {
    vector<string> out;
    if (width == 0) return out;
    string rest = text;
    while (rest.size() > width) {
        size_t cut = rest.rfind(' ', width);
        if (cut == string::npos || cut == 0) cut = width;
        out.push_back(rest.substr(0, cut));
        rest = rest.substr(cut);
        if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
    }
    out.push_back(rest);
    return out;
}

static int colourOf(LineKind kind) {
    switch (kind) {
        case LineKind::Command: return COLOR_PAIR(C_COMMAND) | A_BOLD;
        case LineKind::Error: return COLOR_PAIR(C_ERROR) | A_BOLD;
        case LineKind::Warning: return COLOR_PAIR(C_WARNING);
        case LineKind::Success: return COLOR_PAIR(C_SUCCESS);
        case LineKind::Info: return COLOR_PAIR(C_INFO);
        default: return A_NORMAL;
    }
}

static bool startsWith(const string& s, const string& prefix) {
    return s.compare(0, prefix.size(), prefix) == 0;
}

// starts ncurses and redirects cout into the log, returns false if the terminal can't be used
bool Tui::start() {
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        cerr << "Error: the interface needs an interactive terminal" << endl;
        return false;
    }
    if (initscr() == nullptr) return false;
    raw();                 // Ctrl-C comes to us as a key, so the key in memory still gets wiped on exit
    noecho();
    keypad(stdscr, TRUE);  // arrow keys, PgUp, PgDn etc. come as single key codes
#ifdef NCURSES_VERSION
    set_escdelay(25);      // Esc reacts straight away instead of waiting a second (only ncurses needs this)
#endif
    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(C_LOGO, COLOR_CYAN, -1);
        init_pair(C_COMMAND, COLOR_GREEN, -1);
        init_pair(C_ERROR, COLOR_RED, -1);
        init_pair(C_WARNING, COLOR_YELLOW, -1);
        init_pair(C_SUCCESS, COLOR_GREEN, -1);
        init_pair(C_INFO, COLOR_CYAN, -1);
        init_pair(C_TILDE, COLOR_BLUE, -1);
        init_pair(C_HELPKEY, COLOR_YELLOW, -1);
    }
    // anything the rest of the program writes to cout goes into the log instead of breaking the screen
    oldCout = cout.rdbuf(captured.rdbuf());
    started = true;
    return true;
}

// closes ncurses, restores cout and prints any output that never made it to the screen
void Tui::stop() {
    if (!started) return;
    endwin();
    cout.rdbuf(oldCout);
    started = false;
    string pending = captured.str();
    if (!pending.empty()) cout << pending << flush;
    captured.str("");
}

void Tui::print(const string& text, LineKind kind) {
    flushCaptured(); // keeps the order of cout output and printed lines
    stringstream ss(text);
    string line;
    while (getline(ss, line)) log.push_back({line, kind});
    if (text.empty()) log.push_back({"", kind});
    if (log.size() > MAX_LOG_LINES) log.erase(log.begin(), log.begin() + (log.size() - MAX_LOG_LINES));
    scroll = 0; // new output always jumps back to the bottom
}

void Tui::printHelp() {
    for (const auto& h : HELP) {
        string padded = h.first;
        padded.resize(18, ' ');
        print("  " + padded + h.second, LineKind::Info);
    }
    print("  " + KEYS, LineKind::Info);
}

void Tui::clearLog() {
    flushCaptured();
    log.clear();
    scroll = 0;
}

void Tui::setMode(const string& newMode) { mode = newMode; }
void Tui::setInfo(const string& newInfo) { info = newInfo; }
void Tui::setCommands(const vector<string>& names) { commands = names; }

void Tui::flushCaptured() {
    string text = captured.str();
    if (text.empty()) return;
    captured.str("");
    captured.clear();
    printOutput(text);
}

// colours each line of program output by how it starts (Error, Warning, ...)
void Tui::printOutput(const string& text) {
    stringstream ss(text);
    string line;
    while (getline(ss, line)) {
        LineKind kind = LineKind::Normal;
        if (startsWith(line, "Error")) kind = LineKind::Error;
        else if (startsWith(line, "Warning") || startsWith(line, "No contact")) kind = LineKind::Warning;
        else if (startsWith(line, "Hash:")) kind = LineKind::Info;
        log.push_back({line, kind});
    }
    if (log.size() > MAX_LOG_LINES) log.erase(log.begin(), log.begin() + (log.size() - MAX_LOG_LINES));
    scroll = 0;
}

// Tab: completes the command name, or lists the options if there are several
void Tui::completeCommand() {
    if (input.find(' ') != string::npos || cursor != input.size()) return;
    string typed = input;
    bool colon = !typed.empty() && typed[0] == ':';
    if (colon) typed.erase(0, 1);
    vector<string> matches;
    for (const string& c : commands) {
        if (startsWith(c, typed)) matches.push_back(c);
    }
    if (matches.empty()) return;
    // longest common start of all matches
    string common = matches[0];
    for (const string& m : matches) {
        size_t i = 0;
        while (i < common.size() && i < m.size() && common[i] == m[i]) i++;
        common.resize(i);
    }
    if (matches.size() == 1) common += " ";
    if (common.size() > typed.size()) {
        input = (colon ? ":" : "") + common;
        cursor = input.size();
    } else {
        string list;
        for (const string& m : matches) list += m + "  ";
        print(list, LineKind::Info);
    }
}

// full header needs enough room for the logo, help and at least a few log lines
int Tui::headerHeight(bool& full) const {
    int fullHeight = 1 + (int)LOGO.size() + 1 + 1 + (int)HELP.size() + 1 + 1;
    full = LINES - fullHeight >= 2 + 6 && COLS >= (int)LOGO[0].size() + 2;
    if (full) return fullHeight;
    return LINES >= 6 ? 2 : 0; // one title line + separator, or nothing on a tiny terminal
}

static void centred(int row, const string& text, int attr) {
    int col = max(0, (COLS - (int)text.size()) / 2);
    attron(attr);
    mvaddnstr(row, col, text.c_str(), COLS - col);
    attroff(attr);
}

void Tui::drawHeader(bool full, int height) {
    if (height == 0) return;
    if (!full) {
        centred(0, "CONTACTS  -  type :help for commands", COLOR_PAIR(C_LOGO) | A_BOLD);
        mvhline(1, 0, ACS_HLINE, COLS);
        return;
    }
    int row = 1;
    for (const string& line : LOGO) centred(row++, line, COLOR_PAIR(C_LOGO) | A_BOLD);
    centred(row++, SUBTITLE, A_DIM);
    row++;
    // help block is left aligned inside itself and centred as a whole
    size_t width = 0;
    for (const auto& h : HELP) width = max(width, 18 + h.second.size());
    width = max(width, KEYS.size());
    int col = max(0, (COLS - (int)width) / 2);
    for (const auto& h : HELP) {
        attron(COLOR_PAIR(C_HELPKEY) | A_BOLD);
        mvaddnstr(row, col, h.first.c_str(), COLS - col);
        attroff(COLOR_PAIR(C_HELPKEY) | A_BOLD);
        if (col + 18 < COLS) mvaddnstr(row, col + 18, h.second.c_str(), COLS - col - 18);
        row++;
    }
    attron(A_DIM);
    mvaddnstr(row++, col, KEYS.c_str(), COLS - col);
    attroff(A_DIM);
    mvhline(row, 0, ACS_HLINE, COLS);
}

// log fills rows top..bottom-1, newest at the bottom, empty rows get a '~' like in vim
void Tui::drawLog(int top, int bottom) {
    int height = bottom - top;
    if (height <= 0) return;
    vector<LogLine> visual;
    for (const LogLine& l : log) {
        for (const string& part : wrap(l.text, max(1, COLS - 1))) visual.push_back({part, l.kind});
    }
    size_t h = (size_t)height;
    maxScroll = visual.size() > h ? visual.size() - h : 0;
    scroll = min(scroll, maxScroll);
    size_t first = visual.size() > h ? visual.size() - h - scroll : 0;
    int row = top;
    for (size_t i = first; i < visual.size() && row < bottom; i++, row++) {
        int attr = colourOf(visual[i].kind);
        attron(attr);
        mvaddnstr(row, 0, visual[i].text.c_str(), COLS);
        attroff(attr);
    }
    attron(COLOR_PAIR(C_TILDE) | A_BOLD);
    for (; row < bottom; row++) mvaddch(row, 0, '~');
    attroff(COLOR_PAIR(C_TILDE) | A_BOLD);
}

void Tui::drawStatus(int row) {
    attron(A_REVERSE);
    mvhline(row, 0, ' ', COLS);
    string left = " -- " + mode + " -- ";
    string right = info;
    if (scroll > 0) right = "[scrolled " + to_string(scroll) + "/" + to_string(maxScroll) + "]  " + right;
    right += " ";
    attron(A_BOLD);
    mvaddnstr(row, 0, left.c_str(), COLS);
    attroff(A_BOLD);
    int col = COLS - (int)right.size();
    if (col > (int)left.size()) mvaddstr(row, col, right.c_str());
    attroff(A_REVERSE);
}

void Tui::drawInput(int row) {
    move(row, 0);
    clrtoeol();
    attron(A_BOLD);
    mvaddnstr(row, 0, inputLabel.c_str(), COLS);
    attroff(A_BOLD);
    int labelWidth = min((int)inputLabel.size(), COLS - 1);
    size_t avail = (size_t)max(1, COLS - labelWidth - 1);
    // scrolling the input sideways so the cursor is always visible
    if (cursor < inputOffset) inputOffset = cursor;
    if (cursor > inputOffset + avail) inputOffset = cursor - avail;
    mvaddnstr(row, labelWidth, input.c_str() + min(inputOffset, input.size()), (int)avail);
    move(row, labelWidth + (int)(cursor - inputOffset));
}

// draws the whole screen, also moves anything written to cout into the log
void Tui::redraw() {
    if (!started) return;
    flushCaptured();
    erase();
    bool full;
    int header = headerHeight(full);
    drawHeader(full, header);
    drawLog(header, LINES - 2);
    drawStatus(LINES - 2);
    curs_set(inputLabel.empty() ? 0 : 1);
    drawInput(LINES - 1);
    refresh();
}

// shows the label on the command line and lets the user type a line
InputResult Tui::readLine(const string& label, string& out, bool useHistory) {
    inputLabel = label;
    input.clear();
    cursor = 0;
    inputOffset = 0;
    size_t histPos = history.size();
    string draft; // what was typed before going through history, also used as the search prefix like in vim
    InputResult result = InputResult::Ok;
    bool done = false;
    while (!done) {
        redraw();
        int ch = getch();
        bool edited = false;
        switch (ch) {
            case KEY_RESIZE:
#ifdef PDCURSES
                resize_term(0, 0); // PDCurses has to be told to pick up the new console size
#endif
                break;
#ifdef PADENTER
            case PADENTER: // Enter on the number pad (PDCurses)
#endif
            case '\n': case '\r': case KEY_ENTER:
                done = true;
                break;
            case 27:  // Esc
            case 3:   // Ctrl-C
                result = InputResult::Cancelled;
                done = true;
                break;
            case 4:   // Ctrl-D quits on an empty line, otherwise deletes like Delete
                if (input.empty()) {
                    result = InputResult::Quit;
                    done = true;
                } else if (cursor < input.size()) {
                    input.erase(cursor, 1);
                    edited = true;
                }
                break;
            case KEY_BACKSPACE: case 127: case 8:
                if (cursor > 0) {
                    input.erase(--cursor, 1);
                    edited = true;
                }
                break;
            case KEY_DC:
                if (cursor < input.size()) {
                    input.erase(cursor, 1);
                    edited = true;
                }
                break;
            case KEY_LEFT: if (cursor > 0) cursor--; break;
            case KEY_RIGHT: if (cursor < input.size()) cursor++; break;
            case KEY_HOME: case 1: cursor = 0; break;            // Ctrl-A
            case KEY_END: case 5: cursor = input.size(); break;  // Ctrl-E
            case 21:  // Ctrl-U deletes everything before the cursor
                input.erase(0, cursor);
                cursor = 0;
                edited = true;
                break;
            case 11:  // Ctrl-K deletes everything after the cursor
                input.erase(cursor);
                edited = true;
                break;
            case 23: {  // Ctrl-W deletes the word before the cursor
                size_t start = cursor;
                while (start > 0 && input[start - 1] == ' ') start--;
                while (start > 0 && input[start - 1] != ' ') start--;
                input.erase(start, cursor - start);
                cursor = start;
                edited = true;
                break;
            }
            case 12:  // Ctrl-L repaints the screen
                clearok(stdscr, TRUE);
                break;
            case KEY_PPAGE: scroll += max(1, (LINES - 2) / 2); break;
            case KEY_NPAGE: scroll -= min(scroll, (size_t)max(1, (LINES - 2) / 2)); break;
            case '\t':
                if (useHistory) {
                    completeCommand();
                    edited = true;
                }
                break;
            case KEY_UP:
                if (!useHistory) break;
                if (histPos == history.size()) draft = input;
                // previous entry that starts with what was typed
                for (size_t i = histPos; i-- > 0;) {
                    if (startsWith(history[i], draft)) {
                        histPos = i;
                        input = history[i];
                        cursor = input.size();
                        break;
                    }
                }
                break;
            case KEY_DOWN: {
                if (!useHistory || histPos == history.size()) break;
                size_t i = histPos + 1;
                while (i < history.size() && !startsWith(history[i], draft)) i++;
                histPos = i;
                input = i < history.size() ? history[i] : draft;
                cursor = input.size();
                break;
            }
            default:
                if ((ch >= 32 && ch < 127) || (ch >= 128 && ch < 256)) {
                    input.insert(cursor++, 1, (char)ch);
                    edited = true;
                }
                break;
        }
        // after editing, Up starts searching from the newest entry again with the new text
        if (edited) histPos = history.size();
    }
    if (result == InputResult::Ok && useHistory && !input.empty() && (history.empty() || history.back() != input)) {
        history.push_back(input);
        if (history.size() > MAX_HISTORY) history.erase(history.begin());
    }
    out = input;
    inputLabel.clear();
    input.clear();
    cursor = 0;
    redraw(); // hides the prompt while the command runs
    return result;
}
