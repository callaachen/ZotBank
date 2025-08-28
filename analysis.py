#!/usr/bin/env python3
# analysis.py – Analyze ZotBank log files for resource usage and session metrics

import os
import sys
import pandas as pd
import matplotlib
matplotlib.use('Agg')  # Headless backend for EECS server
import matplotlib.pyplot as plt

def load_csv(file_path):
    if not os.path.exists(file_path):
        print(f"[WARN] File not found: {file_path}")
        return None
    try:
        return pd.read_csv(file_path)
    except Exception as e:
        print(f"[ERROR] Failed to load {file_path}: {e}")
        return None

def plot_resource_utilization(df, output_path="logs/resource_utilization.png"):
    try:
        if 'ArrivalTime' not in df.columns or 'TurnaroundTime' not in df.columns:
            print("[ERROR] Missing required columns in logs/per_customer_log.csv.")
            return

        # Sort by arrival time
        df = df.sort_values(by='ArrivalTime')

        plt.figure(figsize=(10, 6))
        x = df['ArrivalTime'].values
        y = df['TurnaroundTime'].values
        plt.plot(list(x), list(y), marker='o', linestyle='-')
        plt.title("Turnaround Time per Customer")
        plt.xlabel("Arrival Time (timestamp)")
        plt.ylabel("Turnaround Time (seconds)")
        plt.grid(True)
        plt.tight_layout()
        plt.savefig(output_path)
        print(f"[INFO] Turnaround time graph saved to: {output_path}")
    except Exception as e:
        print(f"[ERROR] Plotting failed: {e}")

def summarize_command_counts(log_summary_path):
    df = load_csv(log_summary_path)
    if df is None or df.empty:
        print("[WARN] No data in log_summary.csv.")
        return

    print("\n===== COMMAND SUMMARY =====")
    row = df.iloc[0]
    print(f"Timestamp            : {row['Timestamp']}")
    print(f"Total Requests       : {row['Total Requests']}")
    print(f"Total Releases       : {row['Total Releases']}")
    print(f"Denied Requests      : {row['Total Denied']}")
    print(f"Safe Requests        : {row['Safe Requests']}")
    print(f"Unsafe Requests      : {row['Unsafe Requests']}")
    print(f"Denied (Need)        : {row['Denied Need']}")
    print(f"Denied (Availability): {row['Denied Availability']}")
    print(f"Denied (Unsafe)      : {row['Denied Unsafe']}\n")

    print("Command Counts:")
    columns = ['RQ', 'RL', '*', 'safety', 'reset', 'report', 'explain', 'undo',
               'history', 'help', 'verbose', 'color', 'snapshot', 'load',
               'save', 'exit', 'unknown']
    for col in columns:
        if col in df.columns:
            print(f"  {col:<10}: {row[col]}")

def plot_per_customer_metrics(per_customer_path):
    df = load_csv(per_customer_path)
    if df is None or df.empty:
        print("[WARN] No data in per_customer_log.csv.")
        return

    df.columns = ['CustomerID', 'WaitTime', 'RetryCount', 'ArrivalTime', 'TurnaroundTime']

    fig1, ax1 = plt.subplots()
    ax1.bar(df['CustomerID'], df['WaitTime'], color='skyblue')
    ax1.set_title('Per-Customer Wait Time')
    ax1.set_xlabel('Customer ID')
    ax1.set_ylabel('Wait Time (s)')
    fig1.tight_layout()
    fig1.savefig('logs/per_customer_wait.png')
    plt.close(fig1)

    fig2, ax2 = plt.subplots()
    ax2.bar(df['CustomerID'], df['RetryCount'], color='orange')
    ax2.set_title('Per-Customer Retry Count')
    ax2.set_xlabel('Customer ID')
    ax2.set_ylabel('Retries')
    fig2.tight_layout()
    fig2.savefig('logs/per_customer_retries.png')
    plt.close(fig2)

    fig3, ax3 = plt.subplots()
    ax3.bar(df['CustomerID'], df['TurnaroundTime'], color='green')
    ax3.set_title('Per-Customer Turnaround Time')
    ax3.set_xlabel('Customer ID')
    ax3.set_ylabel('Turnaround Time (s)')
    fig3.tight_layout()
    fig3.savefig('logs/per_customer_turnaround.png')
    plt.close(fig3)

    print("[INFO] Per-customer bar charts saved to:")
    print("       logs/per_customer_wait.png")
    print("       logs/per_customer_retries.png")
    print("       logs/per_customer_turnaround.png")

def main():
    summary_file = "logs/log_summary.csv"
    customer_file = "logs/per_customer_log.csv"

    print("[INFO] Starting analysis...")

    summary_df = load_csv(summary_file)
    customer_df = load_csv(customer_file)

    if summary_df is not None:
        summarize_command_counts(summary_file)

    if customer_df is not None:
        plot_resource_utilization(customer_df)
        plot_per_customer_metrics(customer_file)

    print("[INFO] Analysis complete.")

if __name__ == "__main__":
    main()
