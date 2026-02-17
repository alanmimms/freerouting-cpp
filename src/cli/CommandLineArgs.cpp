#include "cli/CommandLineArgs.h"
#include <iostream>
#include <cstring>
#include <thread>

namespace freerouting {

bool CommandLineArgs::parse(int argc, const char* argv[],
                             CommandLineArgs& args, std::string& errorMsg) {
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      args.helpOnly = true;
      return true;
    } else if (arg == "-v" || arg == "--version") {
      args.versionOnly = true;
      return true;
    } else if (arg == "-i" || arg == "--input") {
      if (i + 1 >= argc) {
        errorMsg = "Missing argument for " + arg;
        return false;
      }
      args.inputFile = argv[++i];
    } else if (arg == "-o" || arg == "--output") {
      if (i + 1 >= argc) {
        errorMsg = "Missing argument for " + arg;
        return false;
      }
      args.outputFile = argv[++i];
    } else if (arg == "-p" || arg == "--passes") {
      if (i + 1 >= argc) {
        errorMsg = "Missing argument for " + arg;
        return false;
      }
      try {
        args.maxPasses = std::stoi(argv[++i]);
        if (args.maxPasses < 1) {
          errorMsg = "Max passes must be at least 1";
          return false;
        }
      } catch (...) {
        errorMsg = "Invalid number for max passes";
        return false;
      }
    } else if (arg == "-t" || arg == "--threads") {
      if (i + 1 >= argc) {
        errorMsg = "Missing argument for " + arg;
        return false;
      }
      try {
        args.maxThreads = std::stoi(argv[++i]);
        if (args.maxThreads < 0) {
          errorMsg = "Max threads cannot be negative";
          return false;
        }
      } catch (...) {
        errorMsg = "Invalid number for max threads";
        return false;
      }
    } else if (arg == "--time-limit") {
      if (i + 1 >= argc) {
        errorMsg = "Missing argument for " + arg;
        return false;
      }
      try {
        args.timeLimit = std::stoi(argv[++i]);
        if (args.timeLimit < 0) {
          errorMsg = "Time limit cannot be negative";
          return false;
        }
      } catch (...) {
        errorMsg = "Invalid number for time limit";
        return false;
      }
    } else if (arg == "--no-optimize") {
      args.optimize = false;
    } else if (arg == "--no-drc") {
      args.runDrc = false;
    } else if (arg == "--stop-on-drc-error") {
      args.stopOnDrcError = true;
    } else if (arg == "-q" || arg == "--quiet") {
      args.verbosity = 0;
      args.showProgress = false;
    } else if (arg == "--verbose") {
      args.verbosity = 2;
    } else if (arg == "--debug") {
      args.verbosity = 3;
    } else if (arg == "--no-progress") {
      args.showProgress = false;
    } else if (arg == "--heatmap") {
      args.generateHeatmap = true;
      // Optional: next arg could be heatmap filename (must end with .svg or similar)
      if (i + 1 < argc && argv[i + 1][0] != '-') {
        std::string nextArg = argv[i + 1];
        // Only consume as heatmap filename if it looks like an output file
        // (ends with .svg, .png, etc.) rather than an input file (.kicad_pcb, .dsn)
        if (nextArg.find(".svg") != std::string::npos ||
            nextArg.find(".png") != std::string::npos) {
          args.heatmapFile = argv[++i];
        }
      }
    } else if (arg == "--visualize" || arg == "-V") {
      args.visualize = true;
    } else if (arg == "--visualize-only") {
      args.visualize = true;
      args.visualizeOnly = true;
    } else if (arg == "--dry-run") {
      args.dryRun = true;
    } else if (arg[0] == '-') {
      errorMsg = "Unknown option: " + arg;
      return false;
    } else {
      // Positional argument - treat as input file if not set
      if (args.inputFile.empty()) {
        args.inputFile = arg;
      } else if (args.outputFile.empty()) {
        args.outputFile = arg;
      } else {
        errorMsg = "Too many positional arguments";
        return false;
      }
    }
  }

  return true;
}

void CommandLineArgs::printUsage(const char* programName) {
  std::cout << "Usage: " << programName << " [OPTIONS] [INPUT_FILE] [OUTPUT_FILE]\n\n";
  std::cout << "FreeRouting C++ - PCB Autorouter\n\n";
  std::cout << "Options:\n";
  std::cout << "  -h, --help              Show this help message\n";
  std::cout << "  -v, --version           Show version information\n";
  std::cout << "  -i, --input FILE        Input file (KiCad .kicad_pcb)\n";
  std::cout << "  -o, --output FILE       Output file (default: input file with _routed suffix)\n";
  std::cout << "  -p, --passes N          Maximum routing passes (default: 10)\n";
  std::cout << "  -t, --threads N         Number of threads (default: auto-detect)\n";
  std::cout << "  --time-limit N          Time limit in seconds (default: no limit)\n";
  std::cout << "  --no-optimize           Skip route optimization\n";
  std::cout << "  --no-drc                Skip design rule checking\n";
  std::cout << "  --stop-on-drc-error     Stop if DRC errors are found\n";
  std::cout << "  -q, --quiet             Quiet mode (minimal output)\n";
  std::cout << "  --verbose               Verbose output\n";
  std::cout << "  --debug                 Debug output\n";
  std::cout << "  --no-progress           Don't show progress indicator\n";
  std::cout << "  --heatmap [FILE]        Generate congestion heatmap (SVG)\n";
  std::cout << "  -V, --visualize         Enable real-time visualization (SDL2)\n";
  std::cout << "  --dry-run               Parse input but don't route or write output\n";
  std::cout << "\n";
  std::cout << "Examples:\n";
  std::cout << "  " << programName << " board.kicad_pcb\n";
  std::cout << "  " << programName << " -i board.kicad_pcb -o routed.kicad_pcb\n";
  std::cout << "  " << programName << " --passes 20 --threads 4 board.kicad_pcb\n";
  std::cout << "  " << programName << " --time-limit 300 --verbose board.kicad_pcb\n";
}

void CommandLineArgs::printVersion() {
  std::cout << "FreeRouting C++ v0.1.0\n";
  std::cout << "C++23 port of the FreeRouting PCB autorouter\n";
  std::cout << "Built with GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__ << "\n";
}

bool CommandLineArgs::validate(std::string& errorMsg) const {
  // If help or version only, validation always succeeds
  if (helpOnly || versionOnly) {
    return true;
  }

  // Input file is required
  if (inputFile.empty()) {
    errorMsg = "Input file is required";
    return false;
  }

  // Check max passes
  if (maxPasses < 1 || maxPasses > 1000) {
    errorMsg = "Max passes must be between 1 and 1000";
    return false;
  }

  // Check max threads
  if (maxThreads < 0 || maxThreads > 256) {
    errorMsg = "Max threads must be between 0 and 256";
    return false;
  }

  // Auto-detect threads if set to 0
  if (maxThreads == 0) {
    unsigned int hwThreads = std::thread::hardware_concurrency();
    if (hwThreads == 0) {
      hwThreads = 1;  // Fallback
    }
    // Note: This doesn't modify the const object, just validates
  }

  return true;
}

} // namespace freerouting
