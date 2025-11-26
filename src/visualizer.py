import matplotlib.pyplot as plt
import os
import numpy as np

def generate_comparison_charts(comparison_data, project_name, output_dir):
    """
    Generates grouped bar charts comparing all implementations.
    Safely handles missing (None) values.
    """
    
    impl_names = list(comparison_data.keys())
    
    # Collect all unique metric keys
    all_metrics = set()
    for data in comparison_data.values():
        all_metrics.update(data.keys())
    
    for metric in all_metrics:
        if not metric: continue
            
        # --- Safe Value Extraction ---
        values = []
        for impl in impl_names:
            val = comparison_data[impl].get(metric)
            if val is None:
                values.append(0.0) # Use 0.0 instead of None to prevent crash
            else:
                values.append(val)
        
        # Skip empty charts
        if all(v == 0 for v in values):
            continue

        plt.figure(figsize=(10, 6))
        colors = plt.cm.viridis(np.linspace(0, 0.8, len(impl_names)))
        bars = plt.bar(impl_names, values, color=colors)
        
        plt.title(f"{project_name}: {metric} Comparison", fontsize=16)
        plt.ylabel(metric)
        plt.xlabel("Implementation")
        
        for bar in bars:
            yval = bar.get_height()
            if yval > 0:
                plt.text(bar.get_x() + bar.get_width()/2.0, yval, 
                         f'{yval:,.2f}', va='bottom', ha='center', fontsize=10, fontweight='bold')

        output_filename = os.path.join(output_dir, f"{project_name}_{metric}_comparison.png")
        plt.savefig(output_filename)
        plt.close()