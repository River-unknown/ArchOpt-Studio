import sys
import os
import glob
from run_sim import run_simulation
from parser import parse_output
from visualizer import generate_comparison_charts 

def main():
    # 1. Validate Arguments
    if len(sys.argv) < 2:
        print("Usage: python src/profile.py <path_to_project_dir>")
        sys.exit(1)
        
    project_dir = sys.argv[1]
    
    if not os.path.isdir(project_dir):
        print(f"Error: Directory not found: {project_dir}")
        sys.exit(1)
        
    project_name = os.path.basename(os.path.normpath(project_dir))
    print(f"=== Profiling Project: {project_name} ===")
    print("Running simulations, please wait...")
    
    # 2. Find all .c files
    c_files = glob.glob(os.path.join(project_dir, "*.c"))
    
    if not c_files:
        print(f"Error: No .c files found in {project_dir}")
        sys.exit(1)
        
    comparison_data = {}

    # 3. Run Loop
    for c_file_path in c_files:
        file_name = os.path.basename(c_file_path)
        impl_name = file_name.replace('.c', '')
        
        # Print a simple progress indicator
        print(f"  > Processing {impl_name}...", end=" ", flush=True)
        
        raw_output = run_simulation(c_file_path)
        if raw_output is None:
            print("[Failed: Sim Error]")
            continue
            
        metrics = parse_output(raw_output)
        if not any(metrics.values()):
            print("[Failed: Parse Error]")
            continue
            
        comparison_data[impl_name] = metrics
        print("[Done]")
        
    if not comparison_data:
        print("Error: No data collected.")
        sys.exit(1)

    # 4. Print Summary Table
    print("\n" + "="*65)
    print(f"{'Implementation':<15} | {'CPU Cycles (M)':<15} | {'Misses':<10} | {'Energy':<10}")
    print("-" * 65)
    
    for impl, m in comparison_data.items():
        cpu = m.get('cpu_cycles_m', 'N/A')
        miss = m.get('cache_misses', 'N/A')
        nrg = m.get('energy_1e13_nj', 'N/A')
        # Format the table row
        print(f"{impl:<15} | {cpu:<15} | {miss:<10} | {nrg:<10}")
    
    print("="*65 + "\n")

    # 5. Generate Charts
    print(f"Generating charts in {project_dir}...")
    generate_comparison_charts(comparison_data, project_name, project_dir)
    print("Done! Charts generated successfully.")

if __name__ == "__main__":
    # Ensure we run from the project root so imports work
    src_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(src_dir)
    os.chdir(project_root)
    
    main()