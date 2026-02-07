#!/bin/bash
# Comprehensive C++ vs Java FreeRouting Benchmark
# Tests all DSN files in freerouting.app/tests/ directory

CPP_CLI="./build/freerouting-cli"
JAVA_JAR="freerouting.app/integrations/KiCad/kicad-freerouting/plugins/jar/freerouting-2.1.0.jar"
OUTPUT_DIR="/tmp/full_benchmark_results"
TIMEOUT_SECONDS=300

mkdir -p "$OUTPUT_DIR/cpp" "$OUTPUT_DIR/java"

# Find all DSN files
DSN_FILES=($(find freerouting.app/tests -name "*.dsn" -type f | sort))

echo "==================================================================="
echo "FreeRouting C++ vs Java - Comprehensive Benchmark"
echo "$(date)"
echo "==================================================================="
echo ""
echo "Found ${#DSN_FILES[@]} DSN files to test"
echo ""

# Results arrays
declare -a NAMES
declare -a CPP_TIMES
declare -a CPP_SUCCESS
declare -a CPP_SEGMENTS
declare -a CPP_VIAS
declare -a CPP_DRC
declare -a JAVA_TIMES
declare -a JAVA_SUCCESS
declare -a JAVA_SEGMENTS
declare -a JAVA_VIAS

test_num=0
for dsn_file in "${DSN_FILES[@]}"; do
  ((test_num++))

  # Extract test name from path
  NAME=$(basename "$dsn_file" .dsn)
  NAMES+=("$NAME")

  echo "-------------------------------------------------------------------"
  echo "Test $test_num/${#DSN_FILES[@]}: $NAME"
  echo "Input: $dsn_file"
  echo "-------------------------------------------------------------------"

  # C++ Test
  echo ""
  echo "[C++] Running freerouting-cpp..."
  CPP_OUTPUT="$OUTPUT_DIR/cpp/${NAME}_routed.kicad_pcb"
  START=$(date +%s.%N)
  timeout $TIMEOUT_SECONDS $CPP_CLI --passes 10 -o "$CPP_OUTPUT" "$dsn_file" > "$OUTPUT_DIR/cpp/${NAME}_log.txt" 2>&1
  CPP_STATUS=$?
  END=$(date +%s.%N)
  CPP_TIME=$(echo "$END - $START" | bc)

  if [ $CPP_STATUS -eq 124 ]; then
    echo "[C++] TIMEOUT after ${TIMEOUT_SECONDS}s"
    CPP_TIMES+=("TIMEOUT")
    CPP_SUCCESS+=("0")
    CPP_SEGMENTS+=("0")
    CPP_VIAS+=("0")
    CPP_DRC+=("?")
  elif [ $CPP_STATUS -ne 0 ]; then
    echo "[C++] FAILED with exit code $CPP_STATUS"
    CPP_TIMES+=("FAILED")
    CPP_SUCCESS+=("0")
    CPP_SEGMENTS+=("0")
    CPP_VIAS+=("0")
    CPP_DRC+=("?")
  else
    echo "[C++] Completed in ${CPP_TIME}s"
    CPP_TIMES+=("$CPP_TIME")

    # Extract metrics
    CPP_SUCCESS_RATE=$(grep "Success rate" "$OUTPUT_DIR/cpp/${NAME}_log.txt" | grep -oP '\d+\.\d+' | head -1 || echo "0")
    CPP_SUCCESS+=("$CPP_SUCCESS_RATE")

    CPP_SEG=$(grep -c "^  (segment" "$CPP_OUTPUT" 2>/dev/null || echo "0")
    CPP_SEGMENTS+=("$CPP_SEG")

    CPP_VIA=$(grep -c "^  (via" "$CPP_OUTPUT" 2>/dev/null || echo "0")
    CPP_VIAS+=("$CPP_VIA")

    CPP_DRC_COUNT=$(grep "Violations:" "$OUTPUT_DIR/cpp/${NAME}_log.txt" | grep -oP '\d+' | head -1 || echo "?")
    CPP_DRC+=("$CPP_DRC_COUNT")
  fi

  # Java Test
  echo ""
  echo "[Java] Running freerouting-2.1.0.jar..."
  JAVA_OUTPUT="$OUTPUT_DIR/java/${NAME}_routed.kicad_pcb"
  START=$(date +%s.%N)
  timeout $TIMEOUT_SECONDS java -Djava.awt.headless=true -jar "$JAVA_JAR" -de "$dsn_file" -do "$JAVA_OUTPUT" -mp 10 > "$OUTPUT_DIR/java/${NAME}_log.txt" 2>&1
  JAVA_STATUS=$?
  END=$(date +%s.%N)
  JAVA_TIME=$(echo "$END - $START" | bc)

  if [ $JAVA_STATUS -eq 124 ]; then
    echo "[Java] TIMEOUT after ${TIMEOUT_SECONDS}s"
    JAVA_TIMES+=("TIMEOUT")
    JAVA_SUCCESS+=("0")
    JAVA_SEGMENTS+=("0")
    JAVA_VIAS+=("0")
  elif [ $JAVA_STATUS -ne 0 ]; then
    echo "[Java] FAILED with exit code $JAVA_STATUS"
    JAVA_TIMES+=("FAILED")
    JAVA_SUCCESS+=("0")
    JAVA_SEGMENTS+=("0")
    JAVA_VIAS+=("0")
  else
    echo "[Java] Completed in ${JAVA_TIME}s"
    JAVA_TIMES+=("$JAVA_TIME")

    # Extract metrics (Java doesn't provide easy success rate)
    JAVA_SUCCESS+=("?")

    JAVA_SEG=$(grep -c "^  (segment" "$JAVA_OUTPUT" 2>/dev/null || echo "0")
    JAVA_SEGMENTS+=("$JAVA_SEG")

    JAVA_VIA=$(grep -c "^  (via" "$JAVA_OUTPUT" 2>/dev/null || echo "0")
    JAVA_VIAS+=("$JAVA_VIA")
  fi

  echo ""
done

# ===================================================================
# NexRx Test (KiCad 9 PCB format) - COMMENTED OUT
# ===================================================================
# echo "-------------------------------------------------------------------"
# echo "Test: NexRx (KiCad 9 PCB)"
# echo "Input: tests/NexRx/NexRx.kicad_pcb"
# echo "-------------------------------------------------------------------"
#
# # C++ Test - NexRx
# echo ""
# echo "[C++] Running freerouting-cpp on NexRx..."
# NEXRX_CPP_OUTPUT="$OUTPUT_DIR/cpp/NexRx_routed.kicad_pcb"
# START=$(date +%s.%N)
# timeout $TIMEOUT_SECONDS $CPP_CLI --passes 10 -o "$NEXRX_CPP_OUTPUT" "tests/NexRx/NexRx.kicad_pcb" > "$OUTPUT_DIR/cpp/NexRx_log.txt" 2>&1
# NEXRX_CPP_STATUS=$?
# END=$(date +%s.%N)
# NEXRX_CPP_TIME=$(echo "$END - $START" | bc)
#
# if [ $NEXRX_CPP_STATUS -eq 124 ]; then
#   echo "[C++] TIMEOUT after ${TIMEOUT_SECONDS}s"
# elif [ $NEXRX_CPP_STATUS -ne 0 ]; then
#   echo "[C++] FAILED with exit code $NEXRX_CPP_STATUS"
# else
#   echo "[C++] Completed in ${NEXRX_CPP_TIME}s"
#   NEXRX_CPP_SEG=$(grep -c "^  (segment" "$NEXRX_CPP_OUTPUT" 2>/dev/null || echo "0")
#   NEXRX_CPP_VIA=$(grep -c "^  (via" "$NEXRX_CPP_OUTPUT" 2>/dev/null || echo "0")
#   echo "  Segments: $NEXRX_CPP_SEG, Vias: $NEXRX_CPP_VIA"
# fi
#
# # Note: Java version cannot directly route KiCad PCB files - would need DSN export first
# echo ""
# echo "[Java] Skipped - requires DSN format (KiCad PCB not supported directly)"
# echo ""

# Generate Summary Report
echo ""
echo "==================================================================="
echo "BENCHMARK SUMMARY"
echo "==================================================================="
echo ""

printf "%-40s %12s %12s %12s %12s %12s\n" "Test Name" "C++ Time" "Java Time" "C++ Seg" "Java Seg" "Speedup"
printf "%-40s %12s %12s %12s %12s %12s\n" "----------------------------------------" "------------" "------------" "------------" "------------" "------------"

for i in "${!NAMES[@]}"; do
  NAME="${NAMES[$i]}"
  CPP_T="${CPP_TIMES[$i]}"
  JAVA_T="${JAVA_TIMES[$i]}"
  CPP_S="${CPP_SEGMENTS[$i]}"
  JAVA_S="${JAVA_SEGMENTS[$i]}"

  # Calculate speedup if both completed
  if [[ "$CPP_T" =~ ^[0-9]+\.?[0-9]*$ ]] && [[ "$JAVA_T" =~ ^[0-9]+\.?[0-9]*$ ]]; then
    SPEEDUP=$(echo "scale=1; $JAVA_T / $CPP_T" | bc)
    SPEEDUP="${SPEEDUP}x"
  else
    SPEEDUP="N/A"
  fi

  printf "%-40s %12s %12s %12s %12s %12s\n" \
    "${NAME:0:40}" "${CPP_T:0:12}" "${JAVA_T:0:12}" "$CPP_S" "$JAVA_S" "$SPEEDUP"
done

echo ""
echo "==================================================================="
echo "DETAILED RESULTS"
echo "==================================================================="
echo ""

# Count successes and failures
CPP_SUCCESS_COUNT=0
CPP_FAIL_COUNT=0
JAVA_SUCCESS_COUNT=0
JAVA_FAIL_COUNT=0

for i in "${!NAMES[@]}"; do
  if [[ "${CPP_TIMES[$i]}" =~ ^[0-9]+\.?[0-9]*$ ]]; then
    ((CPP_SUCCESS_COUNT++))
  else
    ((CPP_FAIL_COUNT++))
  fi

  if [[ "${JAVA_TIMES[$i]}" =~ ^[0-9]+\.?[0-9]*$ ]]; then
    ((JAVA_SUCCESS_COUNT++))
  else
    ((JAVA_FAIL_COUNT++))
  fi
done

echo "C++ Results:"
echo "  Successful: $CPP_SUCCESS_COUNT / ${#DSN_FILES[@]}"
echo "  Failed:     $CPP_FAIL_COUNT / ${#DSN_FILES[@]}"
echo ""
echo "Java Results:"
echo "  Successful: $JAVA_SUCCESS_COUNT / ${#DSN_FILES[@]}"
echo "  Failed:     $JAVA_FAIL_COUNT / ${#DSN_FILES[@]}"
echo ""

# Calculate average times for successful runs
CPP_TOTAL_TIME=0
CPP_COUNT=0
JAVA_TOTAL_TIME=0
JAVA_COUNT=0

for i in "${!NAMES[@]}"; do
  if [[ "${CPP_TIMES[$i]}" =~ ^[0-9]+\.?[0-9]*$ ]]; then
    CPP_TOTAL_TIME=$(echo "$CPP_TOTAL_TIME + ${CPP_TIMES[$i]}" | bc)
    ((CPP_COUNT++))
  fi

  if [[ "${JAVA_TIMES[$i]}" =~ ^[0-9]+\.?[0-9]*$ ]]; then
    JAVA_TOTAL_TIME=$(echo "$JAVA_TOTAL_TIME + ${JAVA_TIMES[$i]}" | bc)
    ((JAVA_COUNT++))
  fi
done

if [ $CPP_COUNT -gt 0 ]; then
  CPP_AVG=$(echo "scale=3; $CPP_TOTAL_TIME / $CPP_COUNT" | bc)
  echo "C++ Average Time (successful runs): ${CPP_AVG}s"
fi

if [ $JAVA_COUNT -gt 0 ]; then
  JAVA_AVG=$(echo "scale=3; $JAVA_TOTAL_TIME / $JAVA_COUNT" | bc)
  echo "Java Average Time (successful runs): ${JAVA_AVG}s"
fi

if [ $CPP_COUNT -gt 0 ] && [ $JAVA_COUNT -gt 0 ]; then
  AVG_SPEEDUP=$(echo "scale=1; $JAVA_AVG / $CPP_AVG" | bc)
  echo ""
  echo "Average Speedup: ${AVG_SPEEDUP}x faster (C++ vs Java)"
fi

echo ""
echo "==================================================================="
echo "Benchmark Complete"
echo "Results saved to: $OUTPUT_DIR"
echo "==================================================================="
