import matplotlib.pyplot as plt
import os

def generate_chart(metrics_dict, title, output_filename="chart.png"):
    """
    Generates and saves a bar chart for the given metrics.
    
    Args:
        metrics_dict (dict): The dictionary from parse_output.
        title (str): The title for the chart (e.g., "RF_AoS Performance").
        output_filename (str): The path to save the .png file.
    """
    
    # Filter out any metrics that weren't found
    valid_metrics = {k: v for k, v in metrics_dict.items() if v is not None}
    
    if not valid_metrics:
        print("No valid metrics to plot.")
        return

    labels = list(valid_metrics.keys())
    values = list(valid_metrics.values())
    
    plt.figure(figsize=(10, 6))
    
    # Create bar chart
    bars = plt.bar(labels, values, color=['blue', 'orange', 'green'])
    
    # Add title and labels
    plt.title(title, fontsize=16)
    plt.ylabel("Value")
    plt.xticks(rotation=0)
    
    # Add data labels on top of bars
    for bar in bars:
        yval = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2.0, yval, 
                 f'{yval:,.2f}', va='bottom', ha='center') # 'va' is vertical alignment

    # Save the plot to a file
    plt.savefig(output_filename)
    print(f"--- Chart saved to {output_filename} ---")
    plt.close() # Close the plot to free up memory