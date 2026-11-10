// this file is made for the vim-like terminal interface (ncurses): logo and help at the top,
// output log in the middle, status bar and command line at the bottom

#pragma once

#include <sstream>
#include <string>
#include <vector>

// decides the colour of a line in the output log
enum class LineKind { Normal, Command, Error, Warning, Success, Info };

// what happened when the user finished typing a line
enum class InputResult { Ok, Cancelled, Quit };

class Tui {
public:
    // starts ncurses and redirects cout into the log, returns false if the terminal can't be used
    bool start();
    // closes ncurses, restores cout and prints any output that never made it to the screen
    void stop();
    // adds text (may have several lines) to the log with the given colour
    void print(const std::string& text, LineKind kind = LineKind::Normal);
    // adds the help lines to the log
    void printHelp();
    void clearLog();
    // left side of the status bar shows the mode (like vim's -- INSERT --), right side shows info
    void setMode(const std::string& mode);
    void setInfo(const std::string& info);
    // command names used for Tab completion
    void setCommands(const std::vector<std::string>& names);
    // shows the label on the command line and lets the user type a line;
    // Up/Down go through history only when useHistory is true (so typed contact details aren't saved)
    InputResult readLine(const std::string& label, std::string& out, bool useHistory);
    // draws the whole screen, also moves anything written to cout into the log
    void redraw();

private:
    struct LogLine {
        std::string text;
        LineKind kind;
    };
    bool started = false;
    std::ostringstream captured;      // everything written to cout while the interface is running
    std::streambuf* oldCout = nullptr;
    std::vector<LogLine> log;
    std::vector<std::string> history;
    std::vector<std::string> commands;
    size_t scroll = 0;                // how many lines the log is scrolled up from the bottom
    size_t maxScroll = 0;
    std::string mode = "COMMAND";
    std::string info;
    std::string inputLabel;
    std::string input;
    size_t cursor = 0;
    size_t inputOffset = 0;           // first visible character when the input is wider than the screen

    void flushCaptured();
    // colours each line of program output by how it starts (Error, Warning, ...)
    void printOutput(const std::string& text);
    void completeCommand();
    int headerHeight(bool& full) const;
    void drawHeader(bool full, int height);
    void drawLog(int top, int bottom);
    void drawStatus(int row);
    void drawInput(int row);
};
