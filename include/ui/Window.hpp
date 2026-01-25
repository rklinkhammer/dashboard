#pragma once

#include <string>
#include <memory>
#include <ftxui/dom/elements.hpp>

// Base class for all window components
class Window {
public:
    explicit Window(const std::string& title = "");
    virtual ~Window() = default;

    // Height management
    void SetHeight(int percent) { height_percent_ = percent; }
    int GetHeight() const { return height_percent_; }

    // Title management
    void SetTitle(const std::string& title) { title_ = title; }
    const std::string& GetTitle() const { return title_; }

    // Rendering - pure virtual, must be implemented by subclasses
    virtual ftxui::Element Render() const = 0;

protected:
    std::string title_;
    int height_percent_ = 100;  // Default to full height
};
