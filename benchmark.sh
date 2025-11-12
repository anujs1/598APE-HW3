#!/usr/bin/env bash

set -euo pipefail

L_vals=(64 1024 1024)
steps=(100000 100000000 10000000000)
temps=(1.75 2.269 2.75)

out="results.csv"
printf 'L,temperature,steps,total_time_seconds\n' > "$out"

for idx in "${!L_vals[@]}"; do
  L="${L_vals[$idx]}"
  S="${steps[$idx]}"
  for T in "${temps[@]}"; do
    echo "Running: ./main.exe $L $T $S"

    if prog_out=$(timeout 300 ./main.exe "$L" "$T" "$S" 2>&1); then
      if echo "$prog_out" | grep -q '\[ERROR\]'; then
        echo "ERROR: Sanity check failed!"
        echo "$prog_out"
        exit 1
      fi

      total=$(printf '%s\n' "$prog_out" | sed -n 's/.*Total time: \([0-9]*\(\.[0-9]*\)\?\) seconds.*/\1/p' | head -n1)

      if [ -z "$total" ]; then
        total="N/A"
      fi
    else
      total="TIMEOUT"
    fi

    printf '%s,%s,%s,%s\n' "$L" "$T" "$S" "$total" >> "$out"
  done
done

echo "Results saved to $out"
