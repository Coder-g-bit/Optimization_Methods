import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

def read_schedule(filename):
    with open(filename, 'r') as f:
        first_line = f.readline().strip().split()
        num_jobs = int(first_line[0])
        num_machines = int(first_line[1])
        makespan = int(first_line[2])
        
        operations = []
        for line in f:
            parts = line.strip().split()
            if len(parts) == 5:
                job_id = int(parts[0])
                op_id = int(parts[1])
                machine = int(parts[2])
                start = int(parts[3])
                end = int(parts[4])
                operations.append((job_id, op_id, machine, start, end))
        
        return num_jobs, num_machines, makespan, operations

def draw_gantt_chart(filename, output_name):
    num_jobs, num_machines, makespan, operations = read_schedule(filename)
    
    colors = plt.cm.tab20(np.linspace(0, 1, num_jobs))
    
    fig, ax = plt.subplots(figsize=(max(12, makespan/30), max(4, num_machines * 0.6)))
    
    for job_id, op_id, machine, start, end in operations:
        width = end - start
        color = colors[job_id]
        rect = mpatches.Rectangle((start, machine - 0.4), width, 0.8, 
                                   facecolor=color, edgecolor='black', linewidth=0.5)
        ax.add_patch(rect)
        if width > 20:
            ax.text(start + width/2, machine, f'J{job_id+1}-{op_id+1}', 
                   ha='center', va='center', fontsize=7, color='white', fontweight='bold')
    
    ax.set_xlim(0, makespan)
    ax.set_ylim(-0.5, num_machines)
    ax.set_xlabel('Time', fontsize=12)
    ax.set_ylabel('Machine', fontsize=12)
    ax.set_title(f'FJSP Gantt Chart (Makespan: {makespan})', fontsize=14, fontweight='bold')
    ax.set_yticks(range(num_machines))
    ax.set_yticklabels([f'M{i}' for i in range(num_machines)])
    ax.grid(axis='x', alpha=0.3, linestyle='--')
    
    legend_patches = [mpatches.Patch(color=colors[j], label=f'Job {j+1}') for j in range(num_jobs)]
    ax.legend(handles=legend_patches, loc='upper right', ncol=min(num_jobs, 10), fontsize=8)
    
    plt.tight_layout()
    plt.savefig(output_name, dpi=150, bbox_inches='tight')
    print(f"Gantt chart saved to {output_name}")

if __name__ == "__main__":
    for i in range(6):
        input_file = f"gantt_instance{i}.txt"
        output_file = f"gantt_instance{i}.png"
        try:
            draw_gantt_chart(input_file, output_file)
        except Exception as e:
            print(f"Error processing instance {i}: {e}")
