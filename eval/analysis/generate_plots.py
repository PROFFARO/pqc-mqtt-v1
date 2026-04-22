#!/usr/bin/env python3
"""
generate_plots.py — Generate publication-quality charts from benchmark data.

Produces:
  1. Handshake latency box plot (PQC vs RSA vs ECDSA)
  2. Handshake latency CDF
  3. Throughput bar chart (by mode and QoS)
  4. Stress test latency distribution
  5. Certificate size comparison bar chart

Usage:
    python3 eval/analysis/generate_plots.py
"""

import os
import sys
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from pathlib import Path

# --- Style ---
sns.set_theme(style="whitegrid", palette="muted", font_scale=1.2)
plt.rcParams["figure.dpi"] = 150
plt.rcParams["savefig.bbox"] = "tight"

# --- Paths ---
PROJECT_ROOT = Path(__file__).resolve().parents[2]
RAW_DIR      = PROJECT_ROOT / "eval" / "results" / "raw"
FIG_DIR      = PROJECT_ROOT / "eval" / "results" / "figures"


def plot_handshake_boxplot():
    """Box plot comparing handshake latency across TLS modes."""
    files = sorted(RAW_DIR.glob("handshake_*.csv"))
    if not files:
        print("  [SKIP] No handshake data found.")
        return

    frames = []
    for f in files:
        df = pd.read_csv(f)
        if "latency_ms" in df.columns:
            frames.append(df)

    if not frames:
        return

    data = pd.concat(frames, ignore_index=True)

    fig, ax = plt.subplots(figsize=(8, 5))
    mode_order = ["pqc", "rsa", "ecdsa"]
    colors = {"pqc": "#e74c3c", "rsa": "#3498db", "ecdsa": "#2ecc71"}

    sns.boxplot(
        data=data, x="mode", y="latency_ms",
        order=[m for m in mode_order if m in data["mode"].values],
        palette=colors, ax=ax, width=0.5
    )

    ax.set_title("TLS Handshake Latency: PQC vs Classical", fontweight="bold")
    ax.set_xlabel("TLS Mode")
    ax.set_ylabel("Latency (ms)")

    out = FIG_DIR / "handshake_boxplot.png"
    fig.savefig(out)
    plt.close(fig)
    print(f"  ✓ {out}")


def plot_handshake_cdf():
    """CDF of handshake latencies."""
    files = sorted(RAW_DIR.glob("handshake_*.csv"))
    if not files:
        return

    fig, ax = plt.subplots(figsize=(8, 5))
    colors = {"pqc": "#e74c3c", "rsa": "#3498db", "ecdsa": "#2ecc71"}

    for f in files:
        df = pd.read_csv(f)
        if "latency_ms" not in df.columns:
            continue
        mode = df["mode"].iloc[0] if "mode" in df.columns else f.stem
        latencies = np.sort(df["latency_ms"].dropna().values)
        cdf = np.arange(1, len(latencies) + 1) / len(latencies)
        ax.plot(latencies, cdf, label=mode.upper(),
                color=colors.get(mode, None), linewidth=2)

    ax.set_title("Handshake Latency CDF", fontweight="bold")
    ax.set_xlabel("Latency (ms)")
    ax.set_ylabel("Cumulative Probability")
    ax.legend()
    ax.grid(True, alpha=0.3)

    out = FIG_DIR / "handshake_cdf.png"
    fig.savefig(out)
    plt.close(fig)
    print(f"  ✓ {out}")


def plot_throughput_bar():
    """Bar chart of message throughput by mode."""
    files = sorted(RAW_DIR.glob("throughput_*.csv"))
    if not files:
        print("  [SKIP] No throughput data found.")
        return

    data = pd.concat([pd.read_csv(f) for f in files], ignore_index=True)

    if "msgs_per_sec" not in data.columns:
        return

    fig, ax = plt.subplots(figsize=(8, 5))
    colors = {"pqc": "#e74c3c", "rsa": "#3498db", "ecdsa": "#2ecc71"}

    mode_order = ["pqc", "rsa", "ecdsa"]
    present = [m for m in mode_order if m in data["mode"].values]

    sns.barplot(
        data=data, x="mode", y="msgs_per_sec",
        order=present, palette=colors, ax=ax
    )

    ax.set_title("MQTT Throughput: PQC vs Classical (QoS 1)", fontweight="bold")
    ax.set_xlabel("TLS Mode")
    ax.set_ylabel("Messages / Second")

    out = FIG_DIR / "throughput_bar.png"
    fig.savefig(out)
    plt.close(fig)
    print(f"  ✓ {out}")


def plot_stress_distribution():
    """Histogram of stress test handshake latencies."""
    files = sorted(RAW_DIR.glob("stress_*.csv"))
    if not files:
        print("  [SKIP] No stress test data found.")
        return

    fig, ax = plt.subplots(figsize=(8, 5))
    colors = {"pqc": "#e74c3c", "rsa": "#3498db", "ecdsa": "#2ecc71"}

    for f in files:
        df = pd.read_csv(f)
        if "latency_ms" not in df.columns:
            continue
        mode = df["mode"].iloc[0] if "mode" in df.columns else f.stem
        successful = df[df["success"] == 1] if "success" in df.columns else df
        ax.hist(successful["latency_ms"], bins=20, alpha=0.6,
                label=mode.upper(), color=colors.get(mode, None))

    ax.set_title("Stress Test: Concurrent Handshake Latency Distribution",
                 fontweight="bold")
    ax.set_xlabel("Latency (ms)")
    ax.set_ylabel("Count")
    ax.legend()

    out = FIG_DIR / "stress_histogram.png"
    fig.savefig(out)
    plt.close(fig)
    print(f"  ✓ {out}")


def main():
    FIG_DIR.mkdir(parents=True, exist_ok=True)

    print("=== Generating Publication Charts ===\n")

    plot_handshake_boxplot()
    plot_handshake_cdf()
    plot_throughput_bar()
    plot_stress_distribution()

    print(f"\n✓ All figures saved to: {FIG_DIR}")


if __name__ == "__main__":
    main()
