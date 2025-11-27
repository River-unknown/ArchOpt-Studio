import subprocess
import os
import sys
import platform

def run_simulation(c_file_path):
    """
    Compiles and runs a single C simulation file, capturing its output.
    Cross-platform compatible (Windows/Linux).
    """
    base_name = os.path.basename(c_file_path)
    
    # Determine executable extension based on OS
    if platform.system() == "Windows":
        executable_name = base_name.replace('.c', '.exe')
        run_command = [f'.\\{executable_name}']
    else:
        executable_name = base_name.replace('.c', '.out')
        run_command = [f'./{executable_name}']
    
    # 1. Compile
    # '-lm' is needed for math library in Linux
    compile_command = ['gcc', c_file_path, '-I.', '-o', executable_name, '-lm']
    
    # print(f"--- Compiling {c_file_path} ---")  <-- Silenced
    try:
        compile_result = subprocess.run(compile_command, capture_output=True, text=True)
        
        if compile_result.returncode != 0:
            print(f"Compilation Failed for {c_file_path}:", file=sys.stderr)
            print(compile_result.stderr, file=sys.stderr) # Print error for debugging
            return None

        # 2. Run
        # print(f"--- Running {executable_name} ---") <-- Silenced
        run_result = subprocess.run(run_command, capture_output=True, text=True)
        
        if run_result.returncode != 0:
            print(f"Execution Failed for {executable_name}:", file=sys.stderr)
            print(run_result.stderr, file=sys.stderr)
            return None

    finally:
        # 3. Clean up (Ensure cleanup happens even if run fails)
        if os.path.exists(executable_name):
            os.remove(executable_name)
    
    return run_result.stdout