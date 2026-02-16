#!/bin/bash

LOG_FILE="/home/ayman/ITI/Project_cpp_iti/Phases/scripts/shell_logs.txt"

DURATION=250
INTERVAL=0.1

# create file if not exists and clear it
: > "$LOG_FILE"

START_TIME=$(date +%s)

while (( $(date +%s) - START_TIME < DURATION )); do
    VALUE=$(awk -v r=$RANDOM 'BEGIN { printf "%.2f", r/32767*100 }')

    echo "$VALUE"
    echo "$VALUE" >> "$LOG_FILE"

    sleep "$INTERVAL"
done
