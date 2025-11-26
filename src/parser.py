import re

def parse_output(raw_text):
    metrics = {
        "cpu_cycles_m": None,
        "cache_misses": None,
        "energy_1e13_nj": None
    }

    # Regex patterns
    cpu_pattern = r"CPU clock cycles required:\s*([\d\.]+)"
    misses_pattern = r"L1-DCACHE misses=([\d\.]+)"
    energy_pattern = r"Total energy \(in nJ\):\s+([\d\.]+)e\+013"

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
        metrics["energy_1e13_nj"] = float(energy_match.group(1))

    # print(f"--- Parsed Metrics --- \n{metrics}") <-- Silenced
    return metrics