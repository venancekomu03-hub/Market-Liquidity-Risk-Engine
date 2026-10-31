
from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt

PROJECT = Path(__file__).resolve().parent.parent
DATA = PROJECT / "data"

# Load 1,000 simulated market snapshots
df = pd.read_csv(DATA / "market_history.csv")

# Calculate market indicators
df["bid_price"] = df["best_bid_cents"] / 100
df["ask_price"] = df["best_ask_cents"] / 100

# An empty side of the book has no valid quote
valid = (df["bid_price"] > 0) & (df["ask_price"] > 0)

df["spread_pct"] = float("nan")
midpoint = (df.loc[valid, "bid_price"] +
            df.loc[valid, "ask_price"]) / 2

df.loc[valid, "spread_pct"] = (
    (df.loc[valid, "ask_price"] -
     df.loc[valid, "bid_price"]) / midpoint * 100
)

# Create a four-panel historical dashboard
fig, axes = plt.subplots(2, 2, figsize=(14, 9))

# 1. Bid-ask spread
axes[0, 0].plot(df["event"], df["spread_pct"])
axes[0, 0].set_title("Bid-Ask Spread Over Time")
axes[0, 0].set_ylabel("Spread (%)")

# 2. Market depth
axes[0, 1].plot(
    df["event"], df["bid_depth"], label="Buy Depth"
)
axes[0, 1].plot(
    df["event"], df["ask_depth"], label="Sell Depth"
)
axes[0, 1].set_title("Order Book Depth")
axes[0, 1].set_ylabel("Available Shares")
axes[0, 1].legend()

# 3. Order book imbalance
axes[1, 0].plot(df["event"], df["imbalance"])
axes[1, 0].axhline(0, linestyle="--")
axes[1, 0].set_title("Order Book Imbalance")
axes[1, 0].set_ylabel("Imbalance")
axes[1, 0].set_ylim(-1.05, 1.05)

# 4. Market price movement
axes[1, 1].plot(
    df["event"],
    df["bid_price"].where(valid),
    label="Best Bid"
)
axes[1, 1].plot(
    df["event"],
    df["ask_price"].where(valid),
    label="Best Ask"
)
axes[1, 1].set_title("Best Bid and Ask Prices")
axes[1, 1].set_ylabel("Price ($)")
axes[1, 1].legend()

for ax in axes.flat:
    ax.set_xlabel("Simulated Event Number")
    ax.grid(alpha=0.3)

fig.suptitle(
    "Market Liquidity & Investment Risk Intelligence",
    fontsize=16
)

plt.tight_layout()

output = DATA / "historical_dashboard.png"
plt.savefig(output, dpi=200)

print(f"Analyzed {len(df)} market snapshots.")
print(f"Dashboard saved to: {output}")

plt.show()
