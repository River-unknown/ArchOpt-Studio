import subprocess
import os
import sys

def run_simulation(c_file_path):
    """
    Compiles and runs a single C simulation file, capturing its output.
    We assume this script is run from the project root directory.
    
    Args:
        c_file_path (str): The path to the .c file (e.g., "projects/RandomForest/AoS.c")

    Returns:
        str: The captured stdout text from the simulation, or None if failed.
    """
    base_name = os.path.basename(c_file_path)
    # Use .exe for Windows executable
    executable_name = base_name.replace('.c', '.exe') 
    
    # 1. Compile the C file
    compile_command = ['gcc', c_file_path, '-I.', '-o', executable_name, '-lm']
    
    print(f"--- Compiling {c_file_path} ---")
    compile_result = subprocess.run(compile_command, capture_output=True, text=True)
    
    if compile_result.returncode != 0:
        print(f"Compilation Failed for {c_file_path}:", file=sys.stderr)
        print(compile_result.stderr, file=sys.stderr)
        return None

    # 2. Run the compiled executable
    # Use the executable name directly
    print(f"--- Running {executable_name} ---")
    run_command = [f'.\\{executable_name}'] # Use Windows path separator
    
    run_result = subprocess.run(run_command, capture_output=True, text=True)
    
    if run_result.returncode != 0:
        print(f"Execution Failed for {executable_name}:", file=sys.stderr)
        print(run_result.stderr, file=sys.stderr)
        return None

    # 3. Clean up the executable file
    os.remove(executable_name)
    
    print(f"--- Simulation Complete ---")
    return run_result.stdout