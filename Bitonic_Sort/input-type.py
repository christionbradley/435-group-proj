import os
import re
import json
import subprocess

# Define the path where .cali files are located (same directory as Python script)
caliper_files_path = "."  # Use "." to refer to the current directory

# List all .cali files in the directory
cali_files = [f for f in os.listdir(caliper_files_path) if f.endswith(".cali")]

# Iterate over each .cali file
for cali_file in cali_files:
    input_file_path = os.path.join(caliper_files_path, cali_file)
    
    # Step 1: Use `cali-query` to export .cali data to JSON
    # Extract the base filename without extension to use for output
    base_filename = os.path.splitext(cali_file)[0]
    output_json_path = os.path.join(caliper_files_path, f"{base_filename}_output.json")

    # Run cali-query command to convert .cali to JSON
    cali_query_command = [
        "cali-query",
        "-q", "*",  # Select all data
        "-o", output_json_path,  # Output JSON path
        input_file_path  # Input .cali file
    ]

    try:
        # Execute the command
        subprocess.run(cali_query_command, check=True)
        print(f"[INFO] Successfully exported {cali_file} to {output_json_path}")
    except subprocess.CalledProcessError as e:
        print(f"[ERROR] Error processing {cali_file}: {e}")
        continue

    # Step 2: Load the exported JSON and modify metadata
    with open(output_json_path, 'r') as json_file:
        caliper_data = json.load(json_file)

    # Extract metadata from the filename (e.g., p2-a65536-random.cali)
    match = re.match(r"p(\d+)-a(\d+)-(.+)\.cali", cali_file)
    if match:
        input_type = match.group(3)  # Extracting input_type from filename
        print(f"[DEBUG] Extracted input_type from filename '{cali_file}': {input_type}")

        # Update input_type if it's "1" to "1%perturbed"
        if input_type == "1":
            input_type = "1%perturbed"
            print(f"[DEBUG] Updated input_type from '1' to '1%perturbed' for file '{cali_file}'")
    else:
        input_type = "unknown"  # Default to "unknown" if the pattern does not match
        print(f"[WARNING] Filename pattern did not match. Set input_type to 'unknown' for file '{cali_file}'")

    # Step 3: Modify the JSON data to update the `input_type`
    for entry in caliper_data:
        if 'attributes' in entry:
            original_input_type = entry['attributes'].get('input_type', 'not set')
            entry['attributes']['input_type'] = input_type  # Set the input type

            # Debug statement to confirm change
            print(f"[INFO] Changed input_type from '{original_input_type}' to '{input_type}' for entry in {cali_file}")

    # Step 4: Save the modified JSON back to a new file
    modified_json_path = os.path.join(caliper_files_path, f"{base_filename}_modified_output.json")
    with open(modified_json_path, 'w') as modified_file:
        json.dump(caliper_data, modified_file, indent=4)

    print(f"[INFO] Modified JSON saved to {modified_json_path}")

print("All files processed successfully.")
