#ifndef FREEROUTING_CLI_COMMANDLINEARGS_H
#define FREEROUTING_CLI_COMMANDLINEARGS_H

#include <string>
#include <vector>

namespace freerouting {

// Command-line argument configuration
struct CommandLineArgs {
  // Input/Output
  std::string inputFile;
  std::string outputFile;

  // Routing options
  int maxPasses = 10;
  int maxThreads = 0;  // 0 = auto-detect
  int timeLimit = 0;   // 0 = no limit (seconds)
  bool optimize = true;

  // DRC options
  bool runDrc = true;
  bool stopOnDrcError = false;

  // Verbosity
  int verbosity = 1;  // 0=quiet, 1=normal, 2=verbose, 3=debug
  bool showProgress = true;

  // Visualization
  bool generateHeatmap = false;
  std::string heatmapFile;  // If empty, use <output>.heatmap.svg

  // Mode
  bool versionOnly = false;
  bool helpOnly = false;
  bool dryRun = false;

  // Parse command line arguments
  static bool parse(int argc, const char* argv[], CommandLineArgs& args, std::string& errorMsg);

  // Print usage information
  static void printUsage(const char* programName);

  // Print version information
  static void printVersion();

  // Validate arguments
  bool validate(std::string& errorMsg) const;
};

} // namespace freerouting

#endif // FREEROUTING_CLI_COMMANDLINEARGS_H
