#!/bin/bash

read -p "Enter server host" HOST
read -p "Enter server port" PORT
read -p "Enter duration (seconds): " DURATION
read -p "Enter mode (0 = cache-heavy, 1 = db-heavy, 2 = mixed): " MODE
read -p "Enter start thread count: " START_THREADS
read -p "Enter end thread count: " END_THREADS
read -p "Enter thread increment step: " STEP
read -p "Enter CSV output file name (e.g., results.csv): " OUTFILE


echo "threads,throughput,avg_latency_ms,total_requests" > "$OUTFILE"

echo "Running tests..."

for (( t=$START_THREADS; t<=$END_THREADS; t+=$STEP ))
do
    echo "Running test with $t threads..."

    OUTPUT=$(./loadgen "$HOST" "$PORT" "$t" "$DURATION" "$MODE" "$THINK")

    THROUGHPUT=$(echo "$OUTPUT" | grep "Throughput" | awk '{print $2}')
    LATENCY=$(echo "$OUTPUT" | grep "Avg Latency" | awk '{print $3}')
    TOTAL=$(echo "$OUTPUT" | grep "Total Requests" | awk '{print $3}')

    echo "$t,$THROUGHPUT,$LATENCY,$TOTAL" >> "$OUTFILE"

done

echo "Results saved to $OUTFILE"