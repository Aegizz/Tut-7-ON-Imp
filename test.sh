#!/bin/bash
set -e

# Start the server in the background, redirecting output to a log file
./server > server.log 2>&1 &
SERVER_PID=$!

# Wait a moment to ensure the server starts properly
sleep 2

# Check if the server is running (using the PID)
if ps -p $SERVER_PID > /dev/null; then
    echo "Server is running."
else
    echo "Server failed to start."
    cat server.log
    exit 1
fi

# Check if the server log contains any errors (e.g., "ERROR" keyword)
if grep -i "error" server.log; then
    echo "Error found in server log."
    cat server.log
    kill $SERVER_PID
    exit 1
fi

# Continue with the client and comparison as before
./debugClient > client.log 2>&1 &
CLIENT_PID=$!
wait $CLIENT_PID

# Check if the client log contains any errors (optional)
if grep -i "error" client.log; then
    echo "Error found in client log."
    cat client.log
    kill $SERVER_PID
    exit 1
fi

# Remove repetitive "Enter Command:" lines from the log before comparison
sed -i '/Enter Command:/d' client.log

# Ensure there's a newline at the end of the log file
echo >> client.log

# Define the expected output file
EXPECTED_OUTPUT="tests/expected_output.txt"

# Compare the client's output to the expected output
if diff -q client.log "$EXPECTED_OUTPUT" > /dev/null; then
    echo "Client output matches expected output."
else
    echo "Client output does not match expected output."
    diff client.log "$EXPECTED_OUTPUT"
    kill $SERVER_PID
    exit 1
fi

# Terminate the server process
kill $SERVER_PID
exit 0
