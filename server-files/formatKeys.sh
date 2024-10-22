#!/bin/bash

# Check if the input file is provided
if [ $# -ne 1 ]; then
  echo "Usage: $0 <input.pem>"
  exit 1
fi

input_file=$1
output_file="modified_$(basename "$input_file" .pem).txt"  # Change output to .txt

# Check if the input file exists
if [ ! -f "$input_file" ]; then
  echo "Error: File '$input_file' not found!"
  exit 1
fi

# Step 1: Add a literal "\n" at the end of each line
# Step 2: Remove actual newline characters to concatenate the file into one line
sed 's/$/\\n/' "$input_file" | tr -d '\n' > "$output_file"

# Output the result
echo "New file concatenated into one line with literal '\\n' added after each line saved as: $output_file"
