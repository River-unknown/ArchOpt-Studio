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
    
    # Determine executable name
    if platform.system() == "Windows":
        exe_name = base_name.replace('.c', '.exe')
    else:
        exe_name = base_name.replace('.c', '.out')
    
    # Get absolute path for the executable to avoid ./ or .\ issues
    # We output the executable in the CURRENT working directory (root)
    executable_path = os.path.abspath(exe_name)
    
    # 1. Compile
    # '-lm' is needed for math library in Linux
    # We use absolute path for output -o to be safe
    compile_command = ['gcc', c_file_path, '-I.', '-o', executable_path, '-lm']
    
    try:
        # Capture both stdout and stderr to debug compilation errors
        compile_result = subprocess.run(compile_command, capture_output=True, text=True)
        
        if compile_result.returncode != 0:
            print(f"Compilation Failed for {c_file_path}:", file=sys.stderr)
            print(compile_result.stderr, file=sys.stderr) 
            return None

        # Check if executable actually exists before running
        if not os.path.exists(executable_path):
            print(f"Error: Executable {executable_path} was not created.", file=sys.stderr)
            return None

        # 2. Run
        # Use absolute path for the command
        run_command = [executable_path]
        
        run_result = subprocess.run(run_command, capture_output=True, text=True)
        
        if run_result.returncode != 0:
            print(f"Execution Failed for {exe_name}:", file=sys.stderr)
            print(run_result.stderr, file=sys.stderr)
            return None
            
        return run_result.stdout

    except Exception as e:
        print(f"An unexpected error occurred: {e}", file=sys.stderr)
        return None

    finally:
        # 3. Clean up
        if os.path.exists(executable_path):
            os.remove(executable_path)