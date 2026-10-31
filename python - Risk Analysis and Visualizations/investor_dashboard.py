
from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt

PROJECT = Path(__file__).resolve().parent.parent
DATA = PROJECT / "data"

df = pd.read_csv(DATA / "market_history.csv")

# Keep only valid market quotes
df = df[
    (df["best_bid_cents"] > 0)
    & (df["best_ask_cents"] > 0)
    & (df["best_ask_cents"] >= df["best_bid_cents"])
].copy()

if df.empty:
    raise ValueError("No valid market snapshots found.")

df["midpoint"] = (
    df["best_bid_cents"] + df["best_ask_cents"]
) / 2

df["spread_pct"] = (
    (df["best_ask_cents"] - df["best_bid_cents"])
    / df["midpoint"] * 100
)

latest = df.iloc[-1]

# Illustrative risk scoring, not a calibrated prediction
risk_score = 0

if latest["spread_pct"] > 1:
    risk_score += 40

if abs(latest["imbalance"]) > 0.30:
    risk_score += 30

if latest["ask_depth"] < 500:
    risk_score += 30

if risk_score >= 70:
    risk_level = "HIGH"
elif risk_score >= 30:
    risk_level = "MODERATE"
else:
    risk_level = "LOW"

fig, axes = plt.subplots(2, 2, figsize=(13, 8))

fig.suptitle(
    "Market Liquidity & Investor Risk Intelligence",
    fontsize=17,
    fontweight="bold"
)

# 1. Market spread
axes[0, 0].plot(df["event"], df["spread_pct"])
axes[0, 0].set_title("Bid-Ask Spread (%)")
axes[0, 0].set_xlabel("Market Event")
axes[0, 0].set_ylabel("Spread (%)")
axes[0, 0].grid(alpha=0.3)

# 2. Order book imbalance
axes[0, 1].plot(df["event"], df["imbalance"])
axes[0, 1].axhline(0, linestyle="--")
axes[0, 1].set_title("Order Book Imbalance")
axes[0, 1].set_xlabel("Market Event")
axes[0, 1].grid(alpha=0.3)

# 3. Market depth
axes[1, 0].plot(
    df["event"], df["bid_depth"], label="Buy Depth"
)
axes[1, 0].plot(
    df["event"], df["ask_depth"], label="Sell Depth"
)
axes[1, 0].set_title("Available Market Depth")
axes[1, 0].set_xlabel("Market Event")
axes[1, 0].legend()
axes[1, 0].grid(alpha=0.3)

# 4. Risk summary
axes[1, 1].axis("off")

summary = (
    f"CURRENT SIMULATED MARKET STATUS\n\n"
    f"Risk Level: {risk_level}\n"
    f"Illustrative Risk Score: {risk_score}/100\n\n"
    f"Best Bid: ${latest['best_bid_cents'] / 100:.2f}\n"
    f"Best Ask: ${latest['best_ask_cents'] / 100:.2f}\n"
    f"Spread: {latest['spread_pct']:.2f}%\n"
    f"Imbalance: {latest['imbalance']:.3f}\n\n"
    f"Educational analysis only.\n"
    f"Not a price forecast or investment advice."
)

axes[1, 1].text(
    0.05, 0.95, summary,
    transform=axes[1, 1].transAxes,
    fontsize=12,
    va="top",
    family="monospace"
)

plt.tight_layout()

output = DATA / "investor_dashboard.png"
plt.savefig(output, dpi=200)
print(f"Dashboard saved: {output}")

plt.show()
