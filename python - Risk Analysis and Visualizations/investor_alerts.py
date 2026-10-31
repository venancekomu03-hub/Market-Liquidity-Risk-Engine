
from pathlib import Path
import pandas as pd

PROJECT = Path(__file__).resolve().parent.parent
DATA = PROJECT / "data"

df = pd.read_csv(DATA / "market_history.csv")

# Convert prices from cents to dollars
df["best_bid"] = df["best_bid_cents"] / 100
df["best_ask"] = df["best_ask_cents"] / 100

# Calculate spread as a percentage of the midpoint
midpoint = (df["best_bid"] + df["best_ask"]) / 2

df["spread_pct"] = (
    (df["best_ask"] - df["best_bid"]) / midpoint * 100
)

# Only evaluate events with valid bid and ask quotes
valid = df[
    (df["best_bid"] > 0)
    & (df["best_ask"] > 0)
    & (df["best_ask"] >= df["best_bid"])
].copy()

if valid.empty:
    raise ValueError("No valid market quotes available.")

# Use the latest valid market snapshot
latest = valid.iloc[-1]

print("\n===== INVESTOR MARKET RISK REPORT =====")
print(f"Market event: {int(latest['event'])}")
print(f"Best bid: ${latest['best_bid']:.2f}")
print(f"Best ask: ${latest['best_ask']:.2f}")
print(f"Bid-ask spread: {latest['spread_pct']:.2f}%")
print(f"Order book imbalance: {latest['imbalance']:.3f}")

alerts = []

# Illustrative thresholds for synthetic market data
if latest["spread_pct"] > 1.0:
    alerts.append(
        "HIGH EXECUTION COST: The bid-ask spread is wide. "
        "Immediate trades may incur higher transaction costs."
    )

if latest["imbalance"] < -0.30:
    alerts.append(
        "SELL-SIDE DEPTH DOMINANCE: More resting quantity "
        "is available on the sell side than the buy side."
    )

if latest["imbalance"] > 0.30:
    alerts.append(
        "BUY-SIDE DEPTH DOMINANCE: More resting quantity "
        "is available on the buy side than the sell side."
    )

if latest["ask_depth"] < 500:
    alerts.append(
        "LOW ASK-SIDE DEPTH: A large buy order may "
        "experience partial fills or increased slippage."
    )

if not alerts:
    alerts.append(
        "No risk thresholds were triggered "
        "in the latest valid market snapshot."
    )

print("\n===== RISK ALERTS =====")

for i, alert in enumerate(alerts, start=1):
    print(f"{i}. {alert}")

print("\nNote: Synthetic market data; illustrative thresholds.")
