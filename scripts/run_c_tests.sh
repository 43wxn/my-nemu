#!/usr/bin/env bash
set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RESULT_DIR="$ROOT_DIR/result"
BUILD_DIR="$ROOT_DIR/build/c_tests"
SUPPORT_DIR="$ROOT_DIR/test_support"

mkdir -p "$RESULT_DIR" "$BUILD_DIR"

mapfile -t TEST_FILES < <(find "$ROOT_DIR" -maxdepth 1 -type f -name 'test_*.c' | sort)

if [ ${#TEST_FILES[@]} -eq 0 ]; then
  echo "未找到 test_*.c 测试文件。"
  exit 1
fi

RUN_ID="$(date -u +%Y%m%dT%H%M%SZ)"
SUMMARY_FILE="$RESULT_DIR/summary_${RUN_ID}.csv"

echo "run_id,test_name,source_file,build_status,pass_fail,elapsed_ms,exit_code,computed_result,detail_file" > "$SUMMARY_FILE"

echo "Run ID: $RUN_ID"
echo "Summary: $SUMMARY_FILE"

for SRC in "${TEST_FILES[@]}"; do
  TEST_NAME="$(basename "$SRC" .c)"
  BIN="$BUILD_DIR/${TEST_NAME}_${RUN_ID}.out"
  DETAIL_FILE="$RESULT_DIR/${TEST_NAME}_${RUN_ID}.txt"

  {
    echo "run_id=$RUN_ID"
    echo "test_name=$TEST_NAME"
    echo "source_file=$SRC"
    echo "build_cmd=gcc -std=c11 -O2 -I\"$SUPPORT_DIR\" \"$SRC\" -o \"$BIN\""
  } > "$DETAIL_FILE"

  BUILD_MSG=$(gcc -std=c11 -O2 -I"$SUPPORT_DIR" "$SRC" -o "$BIN" 2>&1)
  BUILD_RC=$?

  if [ $BUILD_RC -ne 0 ]; then
    {
      echo "build_status=FAIL"
      echo "pass_fail=FAIL"
      echo "elapsed_ms=NA"
      echo "exit_code=NA"
      echo "computed_result=NA"
      echo "--- build_output ---"
      echo "$BUILD_MSG"
    } >> "$DETAIL_FILE"

    echo "$RUN_ID,$TEST_NAME,$SRC,FAIL,FAIL,NA,NA,NA,$DETAIL_FILE" >> "$SUMMARY_FILE"
    continue
  fi

  START_NS=$(date +%s%N)
  PROGRAM_OUTPUT="$($BIN 2>&1)"
  EXIT_CODE=$?
  END_NS=$(date +%s%N)
  ELAPSED_MS=$(( (END_NS - START_NS) / 1000000 ))

  if [[ "$PROGRAM_OUTPUT" == *"CHECK_FAIL:"* ]] || [ $EXIT_CODE -ge 128 ]; then
    PASS_FAIL="FAIL"
  else
    PASS_FAIL="PASS"
  fi

  COMPUTED_RESULT="$EXIT_CODE"

  {
    echo "build_status=PASS"
    echo "pass_fail=$PASS_FAIL"
    echo "elapsed_ms=$ELAPSED_MS"
    echo "exit_code=$EXIT_CODE"
    echo "computed_result=$COMPUTED_RESULT"
    echo "--- program_output ---"
    if [ -n "$PROGRAM_OUTPUT" ]; then
      echo "$PROGRAM_OUTPUT"
    else
      echo "(no stdout/stderr output)"
    fi
  } >> "$DETAIL_FILE"

  echo "$RUN_ID,$TEST_NAME,$SRC,PASS,$PASS_FAIL,$ELAPSED_MS,$EXIT_CODE,$COMPUTED_RESULT,$DETAIL_FILE" >> "$SUMMARY_FILE"
done

echo "测试完成。详细结果位于: $RESULT_DIR"
