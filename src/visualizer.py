import matplotlib.pyplot as plt
import os
import numpy as np

# --- This function is from Sprint 1 (Single File) ---
def generate_chart(metrics_dict, title, output_filename="chart.png"):
    """
    Generates and saves a simple bar chart for a single run.
    """
    valid_metrics = {k: v for k, v in metrics_dict.items() if v is not None}
    
    if not valid_metrics:
        print("No valid metrics to plot.")
        return

    labels = list(valid_metrics.keys())
    values = list(valid_metrics.values())
    
    plt.figure(figsize=(10, 6))
    bars = plt.bar(labels, values, color=['blue', 'orange', 'green'])
    
    plt.title(title, fontsize=16)
    plt.ylabel("Value")
    plt.xticks(rotation=0)
    
    for bar in bars:
        yval = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2.0, yval, 
                 f'{yval:,.2f}', va='bottom', ha='center')

    plt.savefig(output_filename)
    print(f"--- Chart saved to {output_filename} ---")
    plt.close()

# --- NEW FUNCTION FOR SPRINT 2 (Comparison) ---
def generate_comparison_charts(comparison_data, project_name, output_dir):
    """
    Generates grouped bar charts comparing all implementations (AoS, SoA, etc.)
    for each metric found.
    
    Args:
        comparison_data (dict): {"AoS": {"metric": val}, "SoA": {"metric": val}}
        project_name (str): "RandomForest"
        output_dir (str): Path to save charts
    """
    
    # Get list of implementations (e.g., ['AoS', 'SoA', 'Reordered'])
    impl_names = list(comparison_data.keys())
    
    # Collect all unique metric keys present in the data
    all_metrics = set()
    for data in comparison_data.values():
        all_metrics.update(data.keys())
    
    # Generate one chart per metric
    for metric in all_metrics:
        if metric is None:
            continue
            
        # Extract values for this specific metric across all implementations
        # Default to 0 if an implementation is missing the metric
        values = [comparison_data[impl].get(metric, 0) for impl in impl_names] 
        
        plt.figure(figsize=(10, 6))
        
        # Color code bars for consistency
        colors = plt.cm.viridis(np.linspace(0, 0.8, len(impl_names)))
        bars = plt.bar(impl_names, values, color=colors)
        
        # Formatting
        chart_title = f"{project_name}: {metric} Comparison"
        plt.title(chart_title, fontsize=16)
        plt.ylabel(metric)
        plt.xlabel("Implementation")
        
        # Add labels on top of bars
        for bar in bars:
            yval = bar.get_height()
            plt.text(bar.get_x() + bar.get_width()/2.0, yval, 
                     f'{yval:,.2f}', va='bottom', ha='center', fontsize=10, fontweight='bold')

        # Save file
        output_filename = os.path.join(output_dir, f"{project_name}_{metric}_comparison.png")
        plt.savefig(output_filename)
        print(f"--- Comparison chart saved to {output_filename} ---")
        plt.close()