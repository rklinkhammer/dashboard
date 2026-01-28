// MIT License
//
// Copyright (c) 2025 Robert Klinkhammer
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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
