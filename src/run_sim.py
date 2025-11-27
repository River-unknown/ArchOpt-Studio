import subprocess
import os
import sys
import platform

def run_simulation(c_file_path):
    """
    Compiles and runs a single C simulation file, capturing its output.
    Cross-platform compatible (Windows/Linux).
    """
    # 1. Setup Paths & Extensions
    base_name = os.path.basename(c_file_path)
    abs_c_path = os.path.abspath(c_file_path)
    
    # Use a unique name for the executable to avoid collisions
    # We put it in the SAME directory as the C file to avoid include path issues
    output_dir = os.path.dirname(abs_c_path)
    
    if platform.system() == "Windows":
        exe_name = base_name.replace('.c', '.exe')
    else:
        exe_name = base_name.replace('.c', '.out')
        
    executable_path = os.path.join(output_dir, exe_name)
    
    # 2. Compile
    # We need -lm for math library in Linux
    # We use -I. to look for headers in the CURRENT working directory (root where we run python)
    # We assume the script is run from project root
    compile_command = ['gcc', abs_c_path, '-I.', '-o', executable_path, '-lm']
    
    try:
        # print(f"Compiling: {' '.join(compile_command)}") # Debug
        compile_result = subprocess.run(
            compile_command, 
            capture_output=True, 
            text=True
        )
        
        if compile_result.returncode != 0:
            print(f"Compilation Failed for {c_file_path}", file=sys.stderr)
            print(f"Command: {' '.join(compile_command)}", file=sys.stderr)
            print(f"Error: {compile_result.stderr}", file=sys.stderr)
            return None

        # 3. Check if executable exists
        if not os.path.exists(executable_path):
            print(f"Error: Executable {executable_path} was not created.", file=sys.stderr)
            return None

        # 4. Run
        # Use absolute path to execute
        run_command = [executable_path]
        
        # print(f"Running: {executable_path}") # Debug
        run_result = subprocess.run(
            run_command, 
            capture_output=True, 
            text=True,
            timeout=10 # Safety timeout
        )
        
        if run_result.returncode != 0:
            print(f"Execution Failed for {exe_name}", file=sys.stderr)
            print(f"Error: {run_result.stderr}", file=sys.stderr)
            return None
            
        return run_result.stdout

    except subprocess.TimeoutExpired:
        print(f"Simulation timed out for {c_file_path}", file=sys.stderr)
        return None
    except Exception as e:
        print(f"An unexpected error occurred: {e}", file=sys.stderr)
        return None

    finally:
        # 5. Clean up
        if os.path.exists(executable_path):
            os.remove(executable_path)