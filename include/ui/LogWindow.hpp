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

#ifndef GDASHBOARD_LOG_WINDOW_HPP
#define GDASHBOARD_LOG_WINDOW_HPP

#include <ncurses.h>
#include <string>
#include <deque>
#include <algorithm>

// ============================================================================
// Logging Window - Scrollable log buffer
// ============================================================================
class LogWindow {
private:
    WINDOW* win;
    int width, height;
    std::deque<std::string> logs;
    int scroll_y = 0;
    const int MAX_LOGS = 1000;

public:
    LogWindow(WINDOW* w, int wth, int hgt) 
        : win(w), width(wth), height(hgt) {}

    void AddLog(const std::string& line) {
        logs.push_back(line);
        if ((int)logs.size() > MAX_LOGS) {
            logs.pop_front();
        }
        // Auto-scroll to bottom when adding logs
        scroll_y = std::max(0, (int)logs.size() - (height - 2));
    }

    void Render() {
        if (!win) return;
        
        werase(win);
        box(win, 0, 0);
        mvwprintw(win, 0, 2, "Logs");

        int y = 1;
        for (int i = scroll_y; i < (int)logs.size() && y < height - 2; ++i, ++y) {
            // Truncate long lines to fit window
            std::string line = logs[i];
            if ((int)line.length() > width - 4) {
                line = line.substr(0, width - 4);
            }
            mvwprintw(win, y, 2, "%s", line.c_str());
        }

        wrefresh(win);
    }

    void ScrollDown() { 
        int max_scroll = std::max(0, (int)logs.size() - (height - 2));
        if (scroll_y < max_scroll) {
            scroll_y++;
        }
    }

    void ScrollUp() { 
        if (scroll_y > 0) {
            scroll_y--;
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

#endif // GDASHBOARD_LOG_WINDOW_HPP
