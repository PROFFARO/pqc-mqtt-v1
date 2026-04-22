#!/usr/bin/env python3
"""
compute_statistics.py — Compute descriptive statistics from benchmark CSV files.

Reads the raw CSV files produced by handshake_bench, throughput_bench, and
stress_test, then computes mean, median, P95, P99, std deviation, and
exports summary tables.

Usage:
    python3 eval/analysis/compute_statistics.py
"""

import os
import sys
import pandas as pd
import numpy as np
from pathlib import Path

# --- Project paths ---
PROJECT_ROOT = Path(__file__).resolve().parents[2]
RAW_DIR      = PROJECT_ROOT / "eval" / "results" / "raw"
TABLES_DIR   = PROJECT_ROOT / "eval" / "results" / "tables"


def compute_latency_stats(csv_path: Path) -> dict:
    """Compute statistics for a handshake latency CSV."""
    df = pd.read_csv(csv_path)

    if "latency_ms" not in df.columns:
        print(f"  [SKIP] No 'latency_ms' column in {csv_path.name}")
        return None

    latencies = df["latency_ms"].dropna()

    return {
        "file":     csv_path.name,
        "mode":     df["mode"].iloc[0] if "mode" in df.columns else "unknown",
        "count":    len(latencies),
        "mean_ms":  latencies.mean(),
        "median_ms": latencies.median(),
        "std_ms":   latencies.std(),
        "min_ms":   latencies.min(),
        "max_ms":   latencies.max(),
        "p95_ms":   latencies.quantile(0.95),
        "p99_ms":   latencies.quantile(0.99),
    }


def main():
    TABLES_DIR.mkdir(parents=True, exist_ok=True)

    # --- Handshake latency ---
    print("=== Handshake Latency Statistics ===\n")
    handshake_files = sorted(RAW_DIR.glob("handshake_*.csv"))

    if not handshake_files:
        print("  No handshake CSV files found in", RAW_DIR)
    else:
        rows = []
        for f in handshake_files:
            stats = compute_latency_stats(f)
            if stats:
                rows.append(stats)
                print(f"  {stats['mode']:>6s}: mean={stats['mean_ms']:.3f}ms  "
                      f"median={stats['median_ms']:.3f}ms  "
                      f"p99={stats['p99_ms']:.3f}ms  "
                      f"(n={stats['count']})")

        if rows:
            df_summary = pd.DataFrame(rows)
            out_path = TABLES_DIR / "handshake_summary.csv"
            df_summary.to_csv(out_path, index=False)
            print(f"\n  ✓ Summary written to: {out_path}")

            # Also export markdown table
            md_path = TABLES_DIR / "handshake_summary.md"
            with open(md_path, "w") as f:
                f.write("# Handshake Latency Summary\n\n")
                f.write(df_summary.to_markdown(index=False))
            print(f"  ✓ Markdown table: {md_path}")

    # --- Throughput ---
    print("\n=== Throughput Statistics ===\n")
    tp_files = sorted(RAW_DIR.glob("throughput_*.csv"))

    if not tp_files:
        print("  No throughput CSV files found.")
    else:
        all_tp = pd.concat([pd.read_csv(f) for f in tp_files], ignore_index=True)
        out_path = TABLES_DIR / "throughput_summary.csv"
        all_tp.to_csv(out_path, index=False)
        print(f"  ✓ Combined throughput data: {out_path}")

        for _, row in all_tp.iterrows():
            print(f"  {row.get('mode', '?'):>6s} QoS{row.get('qos', '?')}: "
                  f"{row.get('msgs_per_sec', 0):.1f} msgs/sec")

    # --- Stress test ---
    print("\n=== Stress Test Statistics ===\n")
    stress_files = sorted(RAW_DIR.glob("stress_*.csv"))

    if not stress_files:
        print("  No stress test CSV files found.")
    else:
        for f in stress_files:
            stats = compute_latency_stats(f)
            if stats:
                print(f"  {stats['mode']:>6s}: mean={stats['mean_ms']:.3f}ms  "
                      f"max={stats['max_ms']:.3f}ms  (n={stats['count']})")

    print("\n=== Done ===")


if __name__ == "__main__":
    main()
