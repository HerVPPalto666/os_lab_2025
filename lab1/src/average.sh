#!/bin/bash

if [ $# -eq 0 ]; then
  echo "Usage: $0 number1 number2 ..."
  exit 1
fi

count=$#
sum=0

for arg in "$@"; do
  sum=$((sum + arg))
done

average=$(echo "scale=2; $sum / $count" | bc)

echo "Count: $count"
echo "Average: $average"
