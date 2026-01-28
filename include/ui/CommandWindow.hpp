#ifndef GDASHBOARD_COMMAND_WINDOW_HPP
#define GDASHBOARD_COMMAND_WINDOW_HPP

#include <ncurses.h>
#include <string>
#include <deque>

// ============================================================================
// Command Window - User input with history
// ============================================================================
class CommandWindow {
private:
    WINDOW* win;
    int width, height;
    std::string buffer;
    std::deque<std::string> history;
    int history_index = -1;

public:
    CommandWindow(WINDOW* w, int wth, int hgt) 
        : win(w), width(wth), height(hgt) {}

    void Render()
    {
        if (!win)
            return;

        werase(win);
        box(win, 0, 0);
        mvwprintw(win, 0, 2, "Command");

        // Truncate buffer if too long
        std::string display = buffer;
        if ((int)display.length() > width - 6)
        {
            display = "..." + display.substr(display.length() - (width - 9));
        }

        mvwprintw(win, 1, 2, "> %s", display.c_str());

        // --- force cursor to the command input position ---
        // Cursor should be after "> " + displayed text.
        int prompt_col = 2;                                    // where '>' begins
        int cursor_col = prompt_col + 2 + (int)display.size(); // 2 for "> "
        int max_inner_col = width - 2;                         // keep inside the box border
        if (cursor_col > max_inner_col)
            cursor_col = max_inner_col;
        if (cursor_col < 1)
            cursor_col = 1;

        wmove(win, 1, cursor_col);
        // ---------------------------------------------------

        wrefresh(win);
    }

    std::string GetCommand() { 
        return buffer; 
    }

    void ClearBuffer() { 
        buffer.clear(); 
        history_index = -1;
    }

    void AddChar(char ch) {
        if ((int)buffer.length() < width - 6) {
            buffer.push_back(ch);
        }
    }

    void DeleteChar() {
        if (!buffer.empty()) {
            buffer.pop_back();
        }
    }

    void AddHistory(const std::string& cmd) {
        if (!cmd.empty()) {
            history.push_back(cmd);
            history_index = -1;
        }
    }

    void HistoryUp() {
        if (!history.empty()) {
            if (history_index < 0) {
                history_index = (int)history.size() - 1;
            } else if (history_index > 0) {
                history_index--;
            }
            buffer = history[history_index];
        }
    }

    void HistoryDown() {
        if (!history.empty() && history_index >= 0) {
            history_index++;
            if (history_index < (int)history.size()) {
                buffer = history[history_index];
            } else {
                buffer.clear();
                history_index = -1;
            }
        }
    }

    void Resize(int h, int w, int starty) {
        height = h;
        width = w;
        if (win) {
            wresize(win, h, w);
            mvwin(win, starty, 0);
        }
    }
};

#endif // GDASHBOARD_COMMAND_WINDOW_HPP
