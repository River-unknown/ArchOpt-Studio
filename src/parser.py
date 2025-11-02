import re

def parse_output(raw_text):
    """
    Parses the raw text output from the simulator to find key metrics.
    
    Args:
        raw_text (str): The stdout string from run_simulation.

    Returns:
        dict: A dictionary containing the parsed metrics.
    """
    metrics = {
        "cpu_cycles_m": None,
        "cache_misses": None,
        "energy_1e13_nj": None  # Key changed for clarity
    }

    # Regex patterns updated to match your program's output:
    
    # Pattern 1: Finds "CPU clock cycles required: 191339864"
    cpu_pattern = r"CPU clock cycles required:\s*([\d\.]+)"
    
    # Pattern 2: Finds "L1-DCACHE misses=866"
    misses_pattern = r"L1-DCACHE misses=([\d\.]+)"
    
    # Pattern 3: Finds "Total energy (in nJ): 1.7e+013"
    # This specifically captures the '1.7' part
    energy_pattern = r"Total energy \(in nJ\):\s+([\d\.]+)e\+013"

    # Find matches
    cpu_match = re.search(cpu_pattern, raw_text)
    misses_match = re.search(misses_pattern, raw_text)
    energy_match = re.search(energy_pattern, raw_text)

    # Extract values
    if cpu_match:
        # Convert the raw cycle count (e.g., 191339864) to millions (191.34)
        raw_cycles = float(cpu_match.group(1))
        metrics["cpu_cycles_m"] = round(raw_cycles / 1_000_000.0, 2)
        
    if misses_match:
        # Cache misses should be an integer
        metrics["cache_misses"] = int(misses_match.group(1))
        
    if energy_match:
        # This captures the base value (e.g., 1.7)
        metrics["energy_1e13_nj"] = float(energy_match.group(1))

    print(f"--- Parsed Metrics --- \n{metrics}")
    return metrics