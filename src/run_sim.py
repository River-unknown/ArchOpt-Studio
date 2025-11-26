import subprocess
import os
import sys

def run_simulation(c_file_path):
    base_name = os.path.basename(c_file_path)
    executable_name = base_name.replace('.c', '.exe') 
    
    # 1. Compile
    compile_command = ['gcc', c_file_path, '-I.', '-o', executable_name, '-lm']
    
    # print(f"--- Compiling {c_file_path} ---")  <-- Silenced
    compile_result = subprocess.run(compile_command, capture_output=True, text=True)
    
    if compile_result.returncode != 0:
        print(f"Compilation Failed for {c_file_path}:", file=sys.stderr)
        return None

    # 2. Run
    # print(f"--- Running {executable_name} ---") <-- Silenced
    run_command = [f'.\\{executable_name}']
    
    run_result = subprocess.run(run_command, capture_output=True, text=True)
    
    if run_result.returncode != 0:
        print(f"Execution Failed for {executable_name}:", file=sys.stderr)
        return None

    # 3. Clean up
    if os.path.exists(executable_name):
        os.remove(executable_name)
    
    return run_result.stdout