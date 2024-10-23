import os
import glob
from itertools import product
import subprocess
import time

def get_job_size(num_procs):
    if num_procs <= 48:
        return ""
    elif num_procs <= 96:
        return "2"
    elif num_procs <= 144:
        return "3"
    elif num_procs <= 288:
        return "6"
    elif num_procs <= 528:
        return "11"
    else:
        return "22"

def run_missing_experiments(missing_files):
    print("\nRerunning missing experiments...")
    
    for filename in sorted(missing_files):
        # Parse filename to get parameters
        parts = filename.replace('.cali', '').split('-')
        num_procs = int(parts[0][1:])  # Remove 'p'
        input_size = int(parts[1][1:])  # Remove 'a'
        list_type = parts[2][1:]        # Remove 'l'
        
        # Convert "1perturbed" back to "1%perturbed" for the job submission
        if list_type == "1perturbed":
            list_type = "1%perturbed"
            
        # Get appropriate job size suffix
        size = get_job_size(num_procs)
        
        print(f"Submitting job for: {filename}")
        print(f"Parameters: procs={num_procs}, size={input_size}, type={list_type}")
        
        # Submit the job
        process = subprocess.Popen(['sbatch', f'mpi.grace_job{size}', 
                                  str(input_size), str(num_procs), list_type])
        
        # Wait for job to complete
        time.sleep(21)
        output = subprocess.check_output(['sacct']).decode('utf-8')
        while "PENDING" in output:
            time.sleep(2)
            output = subprocess.check_output(['sacct']).decode('utf-8')
        time.sleep(10)
        
        # Clean up core files
        directory = '.'
        word = 'core'
        command = "find {} -type f -name '*{}*' -exec rm -f {{}} \\;".format(directory, word)
        subprocess.call(command, shell=True)
        
        print(f"Completed job for: {filename}\n")

def process_caliper_files(rerun_missing=False):
    # Fix files ending with "-l1"
    for filename in glob.glob("*-l1"):
        new_filename = filename + "perturbed.cali"
        os.rename(filename, new_filename)
        print(f"Renamed: {filename} -> {new_filename}")

    # Generate expected filenames
    input_sizes = [2**16, 2**18, 2**20, 2**22, 2**24, 2**26, 2**28]
    list_types = ["sorted", "random", "reverse_sorted", "1%perturbed"]
    num_proc_list = [2, 4, 8, 16, 32, 64, 128, 256, 512, 1024]

    expected_files = set()
    for procs, size, list_type in product(num_proc_list, input_sizes, list_types):
        # Handle the special case for "1%perturbed"
        if list_type == "1%perturbed":
            list_type_str = "1perturbed"
        else:
            list_type_str = list_type
            
        filename = f"p{procs}-a{size}-l{list_type_str}.cali"
        expected_files.add(filename)

    # Get actual files
    actual_files = set(f for f in os.listdir('.') if f.endswith('.cali'))

    # Find missing files
    missing_files = expected_files - actual_files

    # Print results
    print(f"\nTotal expected files: {len(expected_files)}")
    print(f"Actual files found: {len(actual_files)}")
    print(f"Missing files: {len(missing_files)}")

    if missing_files:
        print("\nMissing files:")
        # Sort missing files for better readability
        for filename in sorted(missing_files):
            # Parse filename to get parameters
            parts = filename.replace('.cali', '').split('-')
            procs = parts[0][1:]  # Remove 'p'
            size = parts[1][1:]   # Remove 'a'
            list_type = parts[2][1:]  # Remove 'l'
            print(f"  - {filename} (Processes: {procs}, Size: {size}, Type: {list_type})")
        
        if rerun_missing:
            run_missing_experiments(missing_files)
    
    return missing_files

if __name__ == "__main__":
    # Set rerun_missing=True to automatically rerun missing experiments
    missing_files = process_caliper_files(rerun_missing=True)