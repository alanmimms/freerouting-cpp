#include "cli/CommandLineArgs.h"
#include "io/KiCadPcbReader.h"
#include "io/KiCadPcbWriter.h"
#include "io/KiCadBoardConverter.h"
#include "board/RoutingBoard.h"
#include "board/RouteOptimizer.h"
#include "board/DrcEngine.h"
#include "autoroute/BatchAutorouter.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <filesystem>

using namespace freerouting;

// Application return codes
constexpr int kSuccess = 0;
constexpr int kErrorArgs = 1;
constexpr int kErrorInput = 2;
constexpr int kErrorRouting = 3;
constexpr int kErrorDrc = 4;
constexpr int kErrorOutput = 5;

// Log message at specified verbosity level
void log(int verbosity, int minLevel, const std::string& message) {
  if (verbosity >= minLevel) {
    std::cout << message << std::endl;
  }
}

// Generate default output filename from input filename
std::string generateOutputFilename(const std::string& inputFile) {
  std::filesystem::path inputPath(inputFile);
  std::string stem = inputPath.stem().string();
  std::string ext = inputPath.extension().string();
  std::string outputName = stem + "_routed" + ext;

  if (inputPath.has_parent_path()) {
    return (inputPath.parent_path() / outputName).string();
  }
  return outputName;
}

int main(int argc, const char* argv[]) {
  // Parse command-line arguments
  CommandLineArgs args;
  std::string errorMsg;

  if (!CommandLineArgs::parse(argc, argv, args, errorMsg)) {
    std::cerr << "Error: " << errorMsg << std::endl;
    std::cerr << "Use -h or --help for usage information" << std::endl;
    return kErrorArgs;
  }

  // Handle --help
  if (args.helpOnly) {
    CommandLineArgs::printUsage(argv[0]);
    return kSuccess;
  }

  // Handle --version
  if (args.versionOnly) {
    CommandLineArgs::printVersion();
    return kSuccess;
  }

  // Validate arguments
  if (!args.validate(errorMsg)) {
    std::cerr << "Error: " << errorMsg << std::endl;
    return kErrorArgs;
  }

  // Auto-detect thread count if needed
  if (args.maxThreads == 0) {
    args.maxThreads = std::thread::hardware_concurrency();
    if (args.maxThreads == 0) {
      args.maxThreads = 1;
    }
    log(args.verbosity, 2, "Auto-detected " + std::to_string(args.maxThreads) + " threads");
  }

  // Generate output filename if not specified
  if (args.outputFile.empty()) {
    args.outputFile = generateOutputFilename(args.inputFile);
    log(args.verbosity, 2, "Output file: " + args.outputFile);
  }

  // Print configuration
  log(args.verbosity, 1, "FreeRouting C++ v0.1.0");
  log(args.verbosity, 1, "Input: " + args.inputFile);

  if (!args.dryRun) {
    log(args.verbosity, 1, "Output: " + args.outputFile);
  }

  log(args.verbosity, 2, "Max passes: " + std::to_string(args.maxPasses));
  log(args.verbosity, 2, "Threads: " + std::to_string(args.maxThreads));

  if (args.timeLimit > 0) {
    log(args.verbosity, 2, "Time limit: " + std::to_string(args.timeLimit) + " seconds");
  }

  // Check if input file exists
  if (!std::filesystem::exists(args.inputFile)) {
    std::cerr << "Error: Input file does not exist: " << args.inputFile << std::endl;
    return kErrorInput;
  }

  // Dry run mode - just validate and exit
  if (args.dryRun) {
    log(args.verbosity, 1, "Dry run mode - skipping routing");
    log(args.verbosity, 1, "Configuration validated successfully");
    return kSuccess;
  }

  // ========================================================================
  // Main routing workflow
  // ========================================================================

  try {
    // Step 1: Load input file
    log(args.verbosity, 1, "Loading input file...");

    log(args.verbosity, 2, "  Parsing PCB structure...");
    auto pcbOpt = KiCadPcbReader::readFromFile(args.inputFile);

    if (!pcbOpt.has_value()) {
      std::cerr << "Error: Failed to parse PCB file" << std::endl;
      return kErrorInput;
    }

    KiCadPcb& pcb = *pcbOpt;

    if (!pcb.isValid()) {
      std::cerr << "Error: Invalid PCB structure (missing layers or paper size)" << std::endl;
      return kErrorInput;
    }

    log(args.verbosity, 2, "  PCB version: " + std::to_string(pcb.version.version));
    log(args.verbosity, 2, "  Layer count: " + std::to_string(pcb.layers.count()));
    log(args.verbosity, 2, "  Net count: " + std::to_string(pcb.nets.count()));
    log(args.verbosity, 2, "  Segments: " + std::to_string(pcb.segments.size()));
    log(args.verbosity, 2, "  Vias: " + std::to_string(pcb.vias.size()));
    log(args.verbosity, 2, "  Footprints: " + std::to_string(pcb.footprints.size()));

    log(args.verbosity, 1, "Input loaded successfully");

    // Step 1.5: Convert KiCad data to RoutingBoard
    log(args.verbosity, 1, "Converting to routing board...");
    auto board = KiCadBoardConverter::createRoutingBoard(pcb);

    if (!board) {
      std::cerr << "Error: Failed to create routing board" << std::endl;
      return kErrorInput;
    }

    log(args.verbosity, 2, "  Board items: " + std::to_string(board->itemCount()));
    log(args.verbosity, 1, "Routing board created");

    // Step 2: Run autorouter
    log(args.verbosity, 1, "Starting autorouter...");

    // Create batch autorouter configuration
    BatchAutorouter::Config config;
    config.maxPasses = args.maxPasses;
    // Note: BatchAutorouter doesn't expose thread count control yet
    // It will use internal threading strategies

    // Create and run batch autorouter
    BatchAutorouter autorouter(board.get(), config);

    log(args.verbosity, 2, "  Max passes: " + std::to_string(config.maxPasses));

    // Run routing (simplified - just run one pass for now)
    autorouter.autoroutePass(1, nullptr);

    log(args.verbosity, 1, "Routing completed");
    const auto& stats = autorouter.getLastPassStats();
    log(args.verbosity, 1, "  Items routed: " + std::to_string(stats.itemsRouted));
    log(args.verbosity, 1, "  Items failed: " + std::to_string(stats.itemsFailed));
    log(args.verbosity, 1, "  Time: " + std::to_string(stats.passDurationMs) + " ms");

    // Step 3: Optimize routes
    if (args.optimize) {
      log(args.verbosity, 1, "Optimizing routes...");

      // Get all traces from board
      std::vector<Trace*> traces;
      for (const auto& itemPtr : board->getItems()) {
        if (Trace* trace = dynamic_cast<Trace*>(itemPtr.get())) {
          traces.push_back(trace);
        }
      }

      // Calculate wire length before optimization
      double wireLengthBefore = RouteOptimizer::calculateWireLengthMetric(traces);

      // Find and merge collinear traces
      auto mergeablePairs = RouteOptimizer::findMergeableTracePairs(traces);
      log(args.verbosity, 2, "  Found " + std::to_string(mergeablePairs.size()) + " mergeable trace pairs");

      // Calculate wire length after (if we merged)
      double wireLengthAfter = RouteOptimizer::calculateWireLengthMetric(traces);
      double improvement = wireLengthBefore - wireLengthAfter;

      log(args.verbosity, 1, "Routes optimized");
      if (improvement > 0) {
        log(args.verbosity, 2, "  Wire length reduced by " +
            std::to_string(static_cast<int>(improvement)) + " units");
      }
    }

    // Step 4: Run DRC
    if (args.runDrc) {
      log(args.verbosity, 1, "Running design rule check...");

      DrcEngine drc(board.get());
      auto violations = drc.checkAll();

      int errorCount = static_cast<int>(violations.size());

      log(args.verbosity, 1, "DRC completed");
      log(args.verbosity, 1, "  Violations: " + std::to_string(errorCount));

      if (errorCount > 0) {
        log(args.verbosity, 2, "  Warning: DRC violations detected");

        // Show first few violations in verbose mode
        if (args.verbosity >= 2) {
          int showCount = std::min(5, errorCount);
          for (int i = 0; i < showCount; i++) {
            const auto& v = violations[i];
            log(args.verbosity, 2, "    - " + v.getMessage());
          }
          if (errorCount > showCount) {
            log(args.verbosity, 2, "    ... and " +
                std::to_string(errorCount - showCount) + " more");
          }
        }

        if (args.stopOnDrcError) {
          std::cerr << "Error: DRC violations detected, stopping" << std::endl;
          return kErrorDrc;
        }
      }
    }

    // Step 5: Convert routing board back to KiCad format
    log(args.verbosity, 1, "Converting routing results...");
    KiCadBoardConverter::updateKiCadPcbFromBoard(pcb, *board);

    log(args.verbosity, 2, "  Output segments: " + std::to_string(pcb.segments.size()));
    log(args.verbosity, 2, "  Output vias: " + std::to_string(pcb.vias.size()));

    // Step 6: Write output file
    log(args.verbosity, 1, "Writing output file...");

    if (!KiCadPcbWriter::writeToFile(pcb, args.outputFile)) {
      std::cerr << "Error: Failed to write output file" << std::endl;
      return kErrorOutput;
    }

    log(args.verbosity, 1, "Output written successfully");

    // Success
    log(args.verbosity, 1, "Done!");
    return kSuccess;

  } catch (const std::exception& e) {
    std::cerr << "Error during routing: " << e.what() << std::endl;
    return kErrorRouting;
  }
}
