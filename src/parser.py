import re

def parse_output(raw_text):
    # Initialize empty; let app.py handle defaults
    metrics = {}

    # Regex patterns
    cpu_pattern = r"CPU clock cycles required:\s*([\d\.]+)"
    misses_pattern = r"L1-DCACHE misses=([\d\.]+)"
    
    # Updated: Captures "2.1e+011" OR "1.7e+013" generically
    energy_pattern = r"Total energy \(in nJ\):\s+([\d\.]+e\+\d+)"

    # Find matches
    cpu_match = re.search(cpu_pattern, raw_text)
    misses_match = re.search(misses_pattern, raw_text)
    energy_match = re.search(energy_pattern, raw_text)

    # Extract values
    if cpu_match:
        raw_cycles = float(cpu_match.group(1))
        metrics["cpu_cycles_m"] = round(raw_cycles / 1_000_000.0, 2)
        
    if misses_match:
        metrics["cache_misses"] = int(misses_match.group(1))
        
    if energy_match:
        raw_energy = float(energy_match.group(1))
        # Normalize to 1e13 scale for readability in charts
        metrics["energy_1e13_nj"] = round(raw_energy / 1e13, 3)

    return metrics