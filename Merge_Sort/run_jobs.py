import subprocess
import time

input_sizes = [2**16, 2**18, 2**20, 2**22, 2**24, 2**26, 2**28]
list_types = ["sorted", "random", "reverse_sorted", "1%perturbed"]
num_proc_list = [2, 4, 8, 16, 32, 64, 128, 256, 512, 1024] # [2, 4, 8, 16, 32, 64, 128, 256, 512, 1024]

for num_procs in num_proc_list:

    
    for input_size in input_sizes:

        size = ""
        if num_procs <= 48:
            size = ""
        elif num_procs <= 96:
            size = "2"
        elif num_procs <= 144:
            size = "3"
        elif num_procs <= 288:
            size = "6"
        elif num_procs <= 528:
            size = "11"
        else:  
            size = "22"
            
        processes = [subprocess.Popen(['sbatch', 'mpi.grace_job{}'.format(size), '%s' % input_size, '%s' % num_procs, list_type]) for list_type in list_types]
        # Wait for all processes to finish
        # for process in processes:
        #     process.wait()
        
        time.sleep(21)
        output = subprocess.check_output(['sacct'])
        while "PENDING" in output:
            time.sleep(2)
            output = subprocess.check_output(['sacct'])
        time.sleep(10)
        directory = '.'
        word = 'core'

        command = "find {} -type f -name '*{}*' -exec rm -f {{}} \\;".format(directory, word)

        subprocess.call(command, shell=True)