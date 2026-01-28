#ifndef GDASHBOARD_STATUS_BAR_HPP
#define GDASHBOARD_STATUS_BAR_HPP

#include <ncurses.h>
#include <string>
#include <algorithm>

// ============================================================================
// Status Bar - Runtime info display
// ============================================================================
class StatusBar {
private:
    WINDOW* win;
    int width, height;
    std::string text;

public:
    StatusBar(WINDOW* w, int wth, int hgt) 
        : win(w), width(wth), height(hgt) {}

    void SetText(const std::string& t) {
        text = t;
        if ((int)text.length() > width - 4) {
            text = text.substr(0, width - 4);
        }
    }

    void Render() {
        if (!win) return;
        
        werase(win);
        wattron(win, A_REVERSE);
        mvwprintw(win, 0, 1, "%s", text.c_str());
        wattroff(win, A_REVERSE);
        wrefresh(win);
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

#endif // GDASHBOARD_STATUS_BAR_HPP
