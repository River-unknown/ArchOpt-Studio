import sys
import os
from run_sim import run_simulation
from parser import parse_output
from visualizer import generate_chart

def main():
    # Check if a C file path is provided as an argument
    if len(sys.argv) < 2:
        print("Error: Please provide the path to the C file.")
        print("Usage: python src/main.py <path_to_c_file>")
        sys.exit(1)
        
    c_file = sys.argv[1] # e.g., "projects/RandomForest/AoS.c"
    
    # 1. Run Simulation
    raw_output = run_simulation(c_file)
    
    if raw_output is None:
        print("Failed to get simulation output. Exiting.")
        sys.exit(1)
    
    # 2. Parse Output
    metrics = parse_output(raw_output)
    
    if not any(metrics.values()):
         print("Failed to parse any metrics. Exiting.")
         sys.exit(1)

    # 3. Generate Visual
    chart_title = f"Performance for {c_file}"
    
    # Create a path for the output file in the *same directory* as the C file
    output_file_name = f"{os.path.basename(c_file).replace('.c', '')}_performance.png"
    output_path = os.path.join(os.path.dirname(c_file), output_file_name)

    generate_chart(metrics, chart_title, output_path)
    
    print("\nSprint 1 Pipeline Complete!")
    print(f"Chart saved to: {output_path}")

if __name__ == "__main__":
    # Get the directory of this script (src)
    src_dir = os.path.dirname(os.path.abspath(__file__))
    # Get the parent directory (project root)
    project_root = os.path.dirname(src_dir)
    
    # Change the current working directory to the project root
    # This makes all file paths (memory.h, projects/...) consistent
    os.chdir(project_root)
    
    main()