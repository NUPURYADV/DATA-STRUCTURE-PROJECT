"""
Plots the CSV files written by disk_scheduler (needs: pip install matplotlib).
  1. ./disk_scheduler demo        -> path_*.csv  -> head_paths.png
  2. ./disk_scheduler experiment  -> results.csv -> bar_<workload>.png, line_<workload>.png
Run:  python plot.py
"""
import csv
import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ALGOS = ["FCFS", "SSTF", "SCAN", "C-SCAN", "LOOK"]


if all(os.path.exists(f"path_{a}.csv") for a in ALGOS):
    fig, axes = plt.subplots(len(ALGOS), 1, figsize=(7, 3 * len(ALGOS)), sharex=False)
    for ax, a in zip(axes, ALGOS):
        steps, tracks = [], []
        with open(f"path_{a}.csv") as f:
            for row in csv.DictReader(f):
                steps.append(int(row["step"]))
                tracks.append(int(row["track"]))
        ax.plot(steps, tracks, marker="o")
        ax.set_title(a)
        ax.set_xlabel("step")
        ax.set_ylabel("track")
    fig.tight_layout()
    fig.savefig("head_paths.png", dpi=120)
    print("saved head_paths.png")


if os.path.exists("results.csv"):
    data = {}   
    with open("results.csv") as f:
        for row in csv.DictReader(f):
            w, n, a = row["workload"], int(row["size"]), row["algorithm"]
            data.setdefault(w, {}).setdefault(n, {})[a] = float(row["avg_total_movement"])

    for w, by_size in data.items():
        sizes = sorted(by_size)

       
        fig, ax = plt.subplots(figsize=(8, 4))
        width = 0.15
        for i, a in enumerate(ALGOS):
            xs = [k + i * width for k in range(len(sizes))]
            ax.bar(xs, [by_size[n][a] for n in sizes], width, label=a)
        ax.set_xticks([k + 2 * width for k in range(len(sizes))])
        ax.set_xticklabels([str(n) for n in sizes])
        ax.set_yscale("log")
        ax.set_xlabel("number of requests")
        ax.set_ylabel("total head movement (log scale)")
        ax.set_title(f"{w} workload")
        ax.legend()
        fig.tight_layout()
        fig.savefig(f"bar_{w}.png", dpi=120)
        plt.close(fig)

     
        fig, ax = plt.subplots(figsize=(8, 4))
        for a in ALGOS:
            ax.plot(sizes, [by_size[n][a] for n in sizes], marker="o", label=a)
        ax.set_xscale("log")
        ax.set_yscale("log")
        ax.set_xlabel("number of requests")
        ax.set_ylabel("total head movement")
        ax.set_title(f"{w} workload")
        ax.legend()
        fig.tight_layout()
        fig.savefig(f"line_{w}.png", dpi=120)
        plt.close(fig)
    print("saved bar_*.png and line_*.png")