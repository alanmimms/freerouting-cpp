#ifndef FREEROUTING_CLI_PROGRESSDISPLAY_H
#define FREEROUTING_CLI_PROGRESSDISPLAY_H

#include <string>
#include <chrono>
#include <iostream>
#include <iomanip>

namespace freerouting {

// Real-time progress display for routing operations
// Provides visual feedback during long-running autoroute tasks
class ProgressDisplay {
public:
  explicit ProgressDisplay(bool enabled = true, bool useColor = true)
    : enabled_(enabled),
      useColor_(useColor),
      totalItems_(0),
      completedItems_(0),
      failedItems_(0),
      currentPass_(0),
      maxPasses_(0),
      lastUpdateTime_(std::chrono::steady_clock::now()) {}

  // Initialize progress tracking
  void init(int totalItems, int maxPasses) {
    totalItems_ = totalItems;
    maxPasses_ = maxPasses;
    completedItems_ = 0;
    failedItems_ = 0;
    currentPass_ = 0;
    startTime_ = std::chrono::steady_clock::now();
  }

  // Update progress for current pass
  void startPass(int passNumber) {
    currentPass_ = passNumber;
    if (enabled_) {
      std::cout << "\n" << colorize("=== Pass " + std::to_string(passNumber) +
                                   "/" + std::to_string(maxPasses_) + " ===", Color::Cyan)
                << std::endl;
    }
  }

  // Report item routed
  void itemRouted(const std::string& message = "") {
    ++completedItems_;
    updateProgress(message);
  }

  // Report item failed
  void itemFailed(const std::string& message = "") {
    ++failedItems_;
    updateProgress(colorize("FAILED: " + message, Color::Red));
  }

  // Report general progress message
  void message(const std::string& msg, bool important = false) {
    if (enabled_) {
      if (important) {
        std::cout << colorize(">>> " + msg, Color::Yellow) << std::endl;
      } else {
        std::cout << "  " << msg << std::endl;
      }
    }
  }

  // Display final summary
  void showSummary() {
    if (!enabled_) return;

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      endTime - startTime_).count();

    std::cout << "\n" << colorize("=== Routing Summary ===", Color::Cyan) << std::endl;
    std::cout << "  Total items:    " << totalItems_ << std::endl;
    std::cout << "  " << colorize("Routed:         " + std::to_string(completedItems_), Color::Green) << std::endl;

    if (failedItems_ > 0) {
      std::cout << "  " << colorize("Failed:         " + std::to_string(failedItems_), Color::Red) << std::endl;
    }

    double successRate = totalItems_ > 0 ? (100.0 * completedItems_ / totalItems_) : 0.0;
    std::cout << "  Success rate:   " << std::fixed << std::setprecision(1)
              << successRate << "%" << std::endl;
    std::cout << "  Time elapsed:   " << formatDuration(duration) << std::endl;

    if (completedItems_ > 0) {
      double avgTime = static_cast<double>(duration) / completedItems_;
      std::cout << "  Avg per item:   " << std::fixed << std::setprecision(1)
                << avgTime << " ms" << std::endl;
    }
  }

  // Progress bar (optional, for long operations)
  void showProgressBar() {
    if (!enabled_ || totalItems_ == 0) return;

    int processed = completedItems_ + failedItems_;
    double percent = 100.0 * processed / totalItems_;
    int barWidth = 50;
    int progress = static_cast<int>(percent * barWidth / 100.0);

    std::cout << "\r  [";
    for (int i = 0; i < barWidth; ++i) {
      if (i < progress) std::cout << colorize("=", Color::Green);
      else if (i == progress) std::cout << colorize(">", Color::Green);
      else std::cout << " ";
    }
    std::cout << "] " << std::fixed << std::setprecision(1) << percent << "% "
              << "(" << processed << "/" << totalItems_ << ")";
    std::cout << std::flush;
  }

private:
  enum class Color {
    Reset,
    Red,
    Green,
    Yellow,
    Blue,
    Magenta,
    Cyan,
    White
  };

  bool enabled_;
  bool useColor_;
  int totalItems_;
  int completedItems_;
  int failedItems_;
  int currentPass_;
  int maxPasses_;
  std::chrono::steady_clock::time_point startTime_;
  std::chrono::steady_clock::time_point lastUpdateTime_;

  // Colorize text (ANSI codes)
  std::string colorize(const std::string& text, Color color) const {
    if (!useColor_) return text;

    const char* code = "";
    switch (color) {
      case Color::Red:     code = "\033[31m"; break;
      case Color::Green:   code = "\033[32m"; break;
      case Color::Yellow:  code = "\033[33m"; break;
      case Color::Blue:    code = "\033[34m"; break;
      case Color::Magenta: code = "\033[35m"; break;
      case Color::Cyan:    code = "\033[36m"; break;
      case Color::White:   code = "\033[37m"; break;
      default:             code = "\033[0m";  break;
    }
    return std::string(code) + text + "\033[0m";
  }

  // Update progress display
  void updateProgress(const std::string& message = "") {
    if (!enabled_) return;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      now - lastUpdateTime_).count();

    // Throttle updates to avoid spam (max 10 updates/second)
    if (elapsed < 100 && message.empty()) return;

    lastUpdateTime_ = now;

    if (!message.empty()) {
      std::cout << "    " << message << std::endl;
    }
  }

  // Format duration as human-readable string
  std::string formatDuration(int64_t milliseconds) const {
    if (milliseconds < 1000) {
      return std::to_string(milliseconds) + " ms";
    } else if (milliseconds < 60000) {
      double seconds = milliseconds / 1000.0;
      return std::to_string(static_cast<int>(seconds)) + "." +
             std::to_string(static_cast<int>((seconds - static_cast<int>(seconds)) * 10)) + " s";
    } else {
      int seconds = static_cast<int>(milliseconds / 1000);
      int minutes = seconds / 60;
      seconds %= 60;
      return std::to_string(minutes) + "m " + std::to_string(seconds) + "s";
    }
  }
};

} // namespace freerouting

#endif // FREEROUTING_CLI_PROGRESSDISPLAY_H
