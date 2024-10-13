#!/bin/bash
set -e

# Start the server in the background, redirecting output to a log file
./server > tests/server.log 2>&1 &
./server2 > tests/server2.log 2>&1 &
SERVER_PID=$!
SERVER2_PID=$!
# Wait a moment to ensure the server starts properly
sleep 2

# Check if the server is running (using the PID)
if ps -p $SERVER_PID > /dev/null; then
    echo "Server is running."
else
    echo "Server failed to start."
    cat tests/server.log
    exit 1
fi

# Check if the server log contains any errors (e.g., "ERROR" keyword)
if grep -i "error" tests/server.log; then
    echo "Error found in server log."
    cat tests/server.log
    kill $SERVER_PID
    kill $SERVER2_PID
    exit 1
fi

# Continue with the client and comparison as before
./testClient > tests/client.log 2>&1 &
CLIENT_PID=$!
wait $CLIENT_PID

# Check if the client log contains any errors (optional)
# if grep -i "error" tests/client.log; then
#     echo "Error found in client log."
#     cat tests/client.log
#     kill $SERVER_PID
#     exit 1
# fi


#Removes client ID, server ID and dates from output to ensure the outputs remain the same
sed -E 's/\[[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}\]//g' tests/client.log | \
sed 's/"client-id":"[^"]*"/"client-id":"<client-id>"/g' | \
sed 's/"server-id":"[^"]*"/"server-id":"<server-id>"/g' | \
sed 's/"time-to-die":"[^"]*"/"time-to-die":"<time-to-die>"/g' > tests/processed_output_client.log

sed -E 's/\[[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}\]//g' tests/server.log | \
sed 's/"client-id":"[^"]*"/"client-id":"<client-id>"/g' | \
sed 's/"server-id":"[^"]*"/"server-id":"<server-id>"/g' | \
sed 's/"time-to-die":"[^"]*"/"time-to-die":"<time-to-die>"/g' > tests/processed_output_server.log


# Define the expected output file
EXPECTED_OUTPUT_CLIENT="tests/expected_output_client.txt"
EXPECTED_OUTPUT_SERVER="tests/expected_output_server.txt"
CLIENT_OUTPUT="tests/processed_output_client.log"
SERVER_OUTPUT="tests/processed_output_server.log"

# Compare the client's output to the expected output
if diff -q "$CLIENT_OUTPUT" "$EXPECTED_OUTPUT_CLIENT" > /dev/null; then
    echo "Client output matches expected output."
else
    echo "Client output does not match expected output."
    diff "$CLIENT_OUTPUT" "$EXPECTED_OUTPUT_CLIENT"
    kill $SERVER_PID
    kill $SERVER2_PID
    exit 1
fi

#This needed to be removed later due to the nature of connectivity between two clients in a server to server connection and timing.

# if diff -q "$SERVER_OUTPUT" "$EXPECTED_OUTPUT_SERVER" > /dev/null; then
#     echo "Server output matches expected output."
# else
#     echo "Server output does not match expected output."
#     diff "$SERVER_OUTPUT" "$EXPECTED_OUTPUT_SERVER"
#     kill $SERVER_PID
#     exit 1
# fi

# Terminate the server process
kill $SERVER_PID
kill $SERVER2_PID
exit 0
