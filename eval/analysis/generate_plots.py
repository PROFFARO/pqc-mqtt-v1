#!/usr/bin/env python3
"""
generate_plots.py — Comprehensive PQC-MQTT benchmark visualization.

Generates 15 publication-quality charts from benchmark CSV data.
Usage: python3 eval/analysis/generate_plots.py
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import seaborn as sns
from pathlib import Path
from scipy import stats

# --- Style ---
sns.set_theme(style="whitegrid", font_scale=1.15)
plt.rcParams.update({
    "figure.dpi": 150, "savefig.bbox": "tight", "savefig.dpi": 200,
    "font.family": "sans-serif", "axes.titleweight": "bold"
})

PROJECT = Path(__file__).resolve().parents[2]
RAW     = PROJECT / "eval" / "results" / "raw"
FIG     = PROJECT / "eval" / "results" / "figures"

COLORS = {"pqc": "#e74c3c", "rsa": "#3498db", "ecdsa": "#2ecc71"}
LABELS = {"pqc": "PQC (ML-KEM/ML-DSA)", "rsa": "RSA-2048", "ecdsa": "ECDSA P-256"}
ORDER  = ["pqc", "rsa", "ecdsa"]

def _load_handshake():
    frames = []
    for f in sorted(RAW.glob("handshake_*.csv")):
        df = pd.read_csv(f)
        if "latency_ms" in df.columns:
            frames.append(df)
    return pd.concat(frames, ignore_index=True) if frames else None

def _present(modes, data):
    return [m for m in modes if m in data["mode"].values]

# ── 1. Handshake Box Plot ──
def plot_01_handshake_boxplot(data):
    if data is None: return
    fig, ax = plt.subplots(figsize=(8, 5))
    sns.boxplot(data=data, x="mode", y="latency_ms", order=_present(ORDER, data),
                palette=COLORS, ax=ax, width=0.5, fliersize=3)
    ax.set_title("TLS Handshake Latency Distribution")
    ax.set_xlabel("TLS Mode"); ax.set_ylabel("Latency (ms)")
    ax.set_xticklabels([LABELS.get(t.get_text(), t.get_text()) for t in ax.get_xticklabels()])
    fig.savefig(FIG / "01_handshake_boxplot.png"); plt.close(fig)
    print(f"  ✓ 01_handshake_boxplot.png")

# ── 2. Handshake Violin Plot ──
def plot_02_handshake_violin(data):
    if data is None: return
    fig, ax = plt.subplots(figsize=(8, 5))
    sns.violinplot(data=data, x="mode", y="latency_ms", order=_present(ORDER, data),
                   palette=COLORS, ax=ax, inner="quartile", cut=0)
    ax.set_title("Handshake Latency Violin Plot")
    ax.set_xlabel("TLS Mode"); ax.set_ylabel("Latency (ms)")
    ax.set_xticklabels([LABELS.get(t.get_text(), t.get_text()) for t in ax.get_xticklabels()])
    fig.savefig(FIG / "02_handshake_violin.png"); plt.close(fig)
    print(f"  ✓ 02_handshake_violin.png")

# ── 3. Handshake CDF ──
def plot_03_handshake_cdf(data):
    if data is None: return
    fig, ax = plt.subplots(figsize=(8, 5))
    for m in _present(ORDER, data):
        vals = np.sort(data[data["mode"] == m]["latency_ms"].values)
        cdf = np.arange(1, len(vals)+1) / len(vals)
        ax.plot(vals, cdf, label=LABELS[m], color=COLORS[m], linewidth=2)
    ax.set_title("Handshake Latency CDF"); ax.set_xlabel("Latency (ms)")
    ax.set_ylabel("Cumulative Probability"); ax.legend(); ax.grid(True, alpha=0.3)
    ax.axhline(y=0.95, color="gray", linestyle="--", alpha=0.5, label="P95")
    fig.savefig(FIG / "03_handshake_cdf.png"); plt.close(fig)
    print(f"  ✓ 03_handshake_cdf.png")

# ── 4. Handshake Scatter (per-iteration) ──
def plot_04_handshake_scatter(data):
    if data is None: return
    fig, ax = plt.subplots(figsize=(10, 5))
    for m in _present(ORDER, data):
        subset = data[data["mode"] == m]
        ax.scatter(subset["iteration"], subset["latency_ms"], label=LABELS[m],
                   color=COLORS[m], alpha=0.5, s=15)
    ax.set_title("Handshake Latency per Iteration"); ax.set_xlabel("Iteration")
    ax.set_ylabel("Latency (ms)"); ax.legend()
    fig.savefig(FIG / "04_handshake_scatter.png"); plt.close(fig)
    print(f"  ✓ 04_handshake_scatter.png")

# ── 5. Handshake Bar (Mean + Std) ──
def plot_05_handshake_bar(data):
    if data is None: return
    fig, ax = plt.subplots(figsize=(8, 5))
    modes = _present(ORDER, data)
    means = [data[data["mode"]==m]["latency_ms"].mean() for m in modes]
    stds  = [data[data["mode"]==m]["latency_ms"].std() for m in modes]
    bars = ax.bar([LABELS[m] for m in modes], means, yerr=stds, capsize=8,
                  color=[COLORS[m] for m in modes], edgecolor="black", linewidth=0.5)
    for bar, mean in zip(bars, means):
        ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + max(stds)*0.1,
                f"{mean:.2f} ms", ha="center", va="bottom", fontweight="bold", fontsize=11)
    ax.set_title("Average Handshake Latency (± σ)"); ax.set_ylabel("Latency (ms)")
    fig.savefig(FIG / "05_handshake_bar.png"); plt.close(fig)
    print(f"  ✓ 05_handshake_bar.png")

# ── 6. Handshake Overhead % ──
def plot_06_overhead_bar(data):
    if data is None: return
    modes = _present(ORDER, data)
    if "ecdsa" not in modes: return
    baseline = data[data["mode"]=="ecdsa"]["latency_ms"].mean()
    fig, ax = plt.subplots(figsize=(7, 5))
    overheads = []
    labels = []
    for m in modes:
        mean = data[data["mode"]==m]["latency_ms"].mean()
        pct = ((mean - baseline) / baseline) * 100
        overheads.append(pct); labels.append(LABELS[m])
    bars = ax.bar(labels, overheads, color=[COLORS[m] for m in modes], edgecolor="black", linewidth=0.5)
    for bar, pct in zip(bars, overheads):
        ax.text(bar.get_x()+bar.get_width()/2, bar.get_height()+1,
                f"{pct:+.1f}%", ha="center", va="bottom", fontweight="bold")
    ax.set_title("Handshake Overhead vs ECDSA Baseline"); ax.set_ylabel("Overhead (%)")
    ax.axhline(y=0, color="black", linewidth=0.8)
    fig.savefig(FIG / "06_overhead_bar.png"); plt.close(fig)
    print(f"  ✓ 06_overhead_bar.png")

# ── 7. Throughput Bar ──
def plot_07_throughput_bar():
    files = sorted(RAW.glob("throughput_*.csv"))
    if not files: print("  [SKIP] No throughput data"); return
    data = pd.concat([pd.read_csv(f) for f in files], ignore_index=True)
    if "msgs_per_sec" not in data.columns: return
    fig, ax = plt.subplots(figsize=(8, 5))
    modes = _present(ORDER, data)
    vals = [data[data["mode"]==m]["msgs_per_sec"].mean() for m in modes]
    bars = ax.bar([LABELS[m] for m in modes], vals, color=[COLORS[m] for m in modes],
                  edgecolor="black", linewidth=0.5)
    for bar, v in zip(bars, vals):
        ax.text(bar.get_x()+bar.get_width()/2, bar.get_height()+max(vals)*0.02,
                f"{v:.0f}", ha="center", fontweight="bold")
    ax.set_title("MQTT Message Throughput (QoS 1, 256B payload)")
    ax.set_ylabel("Messages / Second")
    fig.savefig(FIG / "07_throughput_bar.png"); plt.close(fig)
    print(f"  ✓ 07_throughput_bar.png")

# ── 8. Stress Test Histogram ──
def plot_08_stress_histogram():
    files = sorted(RAW.glob("stress_*.csv"))
    if not files: print("  [SKIP] No stress data"); return
    fig, ax = plt.subplots(figsize=(8, 5))
    for f in files:
        df = pd.read_csv(f)
        if "latency_ms" not in df.columns: continue
        m = df["mode"].iloc[0] if "mode" in df.columns else f.stem.split("_")[-1]
        ok = df[df["success"]==1] if "success" in df.columns else df
        ax.hist(ok["latency_ms"], bins=25, alpha=0.6, label=LABELS.get(m,m), color=COLORS.get(m))
    ax.set_title("Stress Test: 50 Concurrent Handshakes")
    ax.set_xlabel("Latency (ms)"); ax.set_ylabel("Count"); ax.legend()
    fig.savefig(FIG / "08_stress_histogram.png"); plt.close(fig)
    print(f"  ✓ 08_stress_histogram.png")

# ── 9. Stress Box Plot ──
def plot_09_stress_boxplot():
    files = sorted(RAW.glob("stress_*.csv"))
    if not files: return
    data = pd.concat([pd.read_csv(f) for f in files], ignore_index=True)
    data = data[data["success"]==1] if "success" in data.columns else data
    if data.empty: return
    fig, ax = plt.subplots(figsize=(8, 5))
    sns.boxplot(data=data, x="mode", y="latency_ms", order=_present(ORDER, data),
                palette=COLORS, ax=ax, width=0.5)
    ax.set_title("Concurrent Handshake Latency (50 clients)")
    ax.set_xlabel("TLS Mode"); ax.set_ylabel("Latency (ms)")
    fig.savefig(FIG / "09_stress_boxplot.png"); plt.close(fig)
    print(f"  ✓ 09_stress_boxplot.png")

# ── 10. Certificate Size Comparison ──
def plot_10_cert_sizes():
    f = RAW / "cert_sizes.csv"
    if not f.exists(): print("  [SKIP] No cert size data"); return
    df = pd.read_csv(f)
    fig, ax = plt.subplots(figsize=(10, 6))
    types = ["ca_cert", "broker_cert", "broker_key", "client_cert", "client_key"]
    x = np.arange(len(types)); w = 0.25
    for i, m in enumerate(_present(ORDER, df)):
        subset = df[df["mode"]==m]
        vals = [subset[subset["file_type"]==t]["size_bytes"].values[0]/1024 if len(subset[subset["file_type"]==t]) else 0 for t in types]
        ax.bar(x + i*w, vals, w, label=LABELS[m], color=COLORS[m], edgecolor="black", linewidth=0.5)
    ax.set_xticks(x + w); ax.set_xticklabels(["CA Cert", "Broker Cert", "Broker Key", "Client Cert", "Client Key"])
    ax.set_title("Certificate & Key File Sizes"); ax.set_ylabel("Size (KB)"); ax.legend()
    fig.savefig(FIG / "10_cert_sizes.png"); plt.close(fig)
    print(f"  ✓ 10_cert_sizes.png")

# ── 11. Total Chain Size ──
def plot_11_chain_size():
    f = RAW / "cert_sizes.csv"
    if not f.exists(): return
    df = pd.read_csv(f)
    fig, ax = plt.subplots(figsize=(7, 5))
    modes = _present(ORDER, df)
    totals = []
    for m in modes:
        total = df[(df["mode"]==m) & (df["file_type"].str.contains("cert"))]["size_bytes"].sum() / 1024
        totals.append(total)
    bars = ax.bar([LABELS[m] for m in modes], totals, color=[COLORS[m] for m in modes], edgecolor="black", linewidth=0.5)
    for bar, t in zip(bars, totals):
        ax.text(bar.get_x()+bar.get_width()/2, bar.get_height()+0.5,
                f"{t:.1f} KB", ha="center", fontweight="bold")
    ax.set_title("Total Certificate Chain Size"); ax.set_ylabel("Size (KB)")
    fig.savefig(FIG / "11_chain_size.png"); plt.close(fig)
    print(f"  ✓ 11_chain_size.png")

# ── 12. Payload Sweep (throughput vs size) ──
def plot_12_payload_sweep():
    files = sorted(RAW.glob("payload_sweep_*.csv"))
    if not files: print("  [SKIP] No payload sweep data"); return
    data = pd.concat([pd.read_csv(f) for f in files], ignore_index=True)
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    for m in _present(ORDER, data):
        sub = data[data["mode"]==m].sort_values("payload_bytes")
        ax1.plot(sub["payload_bytes"], sub["msgs_per_sec"], "o-", label=LABELS[m], color=COLORS[m], linewidth=2)
        ax2.plot(sub["payload_bytes"], sub["mbps"], "s-", label=LABELS[m], color=COLORS[m], linewidth=2)
    ax1.set_xscale("log", base=2); ax1.set_title("Throughput vs Payload Size")
    ax1.set_xlabel("Payload (bytes)"); ax1.set_ylabel("Messages/sec"); ax1.legend()
    ax1.xaxis.set_major_formatter(ticker.FuncFormatter(lambda x,_: f"{int(x)}"))
    ax2.set_xscale("log", base=2); ax2.set_title("Bandwidth vs Payload Size")
    ax2.set_xlabel("Payload (bytes)"); ax2.set_ylabel("Mbps"); ax2.legend()
    ax2.xaxis.set_major_formatter(ticker.FuncFormatter(lambda x,_: f"{int(x)}"))
    fig.suptitle("Payload Size Impact on PQC-TLS Performance", fontweight="bold", y=1.02)
    fig.savefig(FIG / "12_payload_sweep.png"); plt.close(fig)
    print(f"  ✓ 12_payload_sweep.png")

# ── 13. QoS Comparison ──
def plot_13_qos_comparison():
    files = sorted(RAW.glob("qos_comparison_*.csv"))
    if not files: print("  [SKIP] No QoS data"); return
    data = pd.concat([pd.read_csv(f) for f in files], ignore_index=True)
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    modes = _present(ORDER, data)
    qos_levels = sorted(data["qos"].unique())
    x = np.arange(len(qos_levels)); w = 0.25
    for i, m in enumerate(modes):
        sub = data[data["mode"]==m].sort_values("qos")
        ax1.bar(x+i*w, sub["msgs_per_sec"].values, w, label=LABELS[m], color=COLORS[m], edgecolor="black", linewidth=0.5)
        ax2.bar(x+i*w, sub["avg_latency_ms"].values, w, label=LABELS[m], color=COLORS[m], edgecolor="black", linewidth=0.5)
    ax1.set_xticks(x+w); ax1.set_xticklabels([f"QoS {q}" for q in qos_levels])
    ax1.set_title("Throughput by QoS Level"); ax1.set_ylabel("Messages/sec"); ax1.legend()
    ax2.set_xticks(x+w); ax2.set_xticklabels([f"QoS {q}" for q in qos_levels])
    ax2.set_title("Avg Message Latency by QoS"); ax2.set_ylabel("Latency (ms)"); ax2.legend()
    fig.suptitle("QoS Level Impact on PQC vs Classical", fontweight="bold", y=1.02)
    fig.savefig(FIG / "13_qos_comparison.png"); plt.close(fig)
    print(f"  ✓ 13_qos_comparison.png")

# ── 14. Radar / Spider Chart ──
def plot_14_radar(hs_data):
    if hs_data is None: return
    modes = _present(ORDER, hs_data)
    if len(modes) < 2: return
    metrics = {}
    for m in modes:
        sub = hs_data[hs_data["mode"]==m]["latency_ms"]
        metrics[m] = {"Mean Latency": sub.mean(), "P99 Latency": sub.quantile(0.99),
                      "Std Dev": sub.std(), "Max Latency": sub.max(), "Min Latency": sub.min()}
    tp_files = sorted(RAW.glob("throughput_*.csv"))
    if tp_files:
        tp = pd.concat([pd.read_csv(f) for f in tp_files], ignore_index=True)
        for m in modes:
            sub = tp[tp["mode"]==m]
            metrics[m]["Throughput"] = sub["msgs_per_sec"].mean() if len(sub) else 0
    labels = list(list(metrics.values())[0].keys())
    N = len(labels)
    angles = np.linspace(0, 2*np.pi, N, endpoint=False).tolist() + [0]
    fig, ax = plt.subplots(figsize=(8, 8), subplot_kw=dict(projection="polar"))
    for m in modes:
        vals = [metrics[m][l] for l in labels]
        max_vals = [max(metrics[mm][l] for mm in modes) for l in labels]
        norm = [v/mx if mx > 0 else 0 for v, mx in zip(vals, max_vals)]
        norm += [norm[0]]
        ax.plot(angles, norm, "o-", label=LABELS[m], color=COLORS[m], linewidth=2)
        ax.fill(angles, norm, alpha=0.1, color=COLORS[m])
    ax.set_xticks(angles[:-1]); ax.set_xticklabels(labels, size=9)
    ax.set_title("Multi-Dimensional Performance Comparison", pad=20, fontweight="bold")
    ax.legend(loc="upper right", bbox_to_anchor=(1.3, 1.1))
    fig.savefig(FIG / "14_radar_chart.png"); plt.close(fig)
    print(f"  ✓ 14_radar_chart.png")

# ── 15. Summary Comparison Table ──
def plot_15_summary_table(hs_data):
    if hs_data is None: return
    modes = _present(ORDER, hs_data)
    rows = []
    for m in modes:
        sub = hs_data[hs_data["mode"]==m]["latency_ms"]
        row = {"Algorithm": LABELS[m], "Mean (ms)": f"{sub.mean():.2f}",
               "Median (ms)": f"{sub.median():.2f}", "Std (ms)": f"{sub.std():.2f}",
               "P95 (ms)": f"{sub.quantile(0.95):.2f}", "P99 (ms)": f"{sub.quantile(0.99):.2f}",
               "Min (ms)": f"{sub.min():.2f}", "Max (ms)": f"{sub.max():.2f}"}
        rows.append(row)
    df = pd.DataFrame(rows)
    fig, ax = plt.subplots(figsize=(12, 2 + len(rows)*0.6))
    ax.axis("off")
    colors_list = [[COLORS.get(m, "#ccc")] + ["white"]*(len(df.columns)-1) for m in modes]
    table = ax.table(cellText=df.values, colLabels=df.columns, loc="center",
                     cellLoc="center", colColours=["#2c3e50"]*len(df.columns))
    table.auto_set_font_size(False); table.set_fontsize(11); table.scale(1.2, 1.8)
    for j in range(len(df.columns)):
        table[0, j].set_text_props(color="white", fontweight="bold")
    ax.set_title("Handshake Latency Summary Statistics", fontweight="bold", fontsize=14, pad=20)
    fig.savefig(FIG / "15_summary_table.png"); plt.close(fig)
    print(f"  ✓ 15_summary_table.png")

def main():
    FIG.mkdir(parents=True, exist_ok=True)
    print("=" * 50)
    print("  PQC-MQTT Chart Generator (15 visualizations)")
    print("=" * 50 + "\n")
    hs = _load_handshake()
    plot_01_handshake_boxplot(hs)
    plot_02_handshake_violin(hs)
    plot_03_handshake_cdf(hs)
    plot_04_handshake_scatter(hs)
    plot_05_handshake_bar(hs)
    plot_06_overhead_bar(hs)
    plot_07_throughput_bar()
    plot_08_stress_histogram()
    plot_09_stress_boxplot()
    plot_10_cert_sizes()
    plot_11_chain_size()
    plot_12_payload_sweep()
    plot_13_qos_comparison()
    plot_14_radar(hs)
    plot_15_summary_table(hs)
    print(f"\n✓ All charts saved to: {FIG}")

if __name__ == "__main__":
    main()
