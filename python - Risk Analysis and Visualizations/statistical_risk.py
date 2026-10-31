
from pathlib import Path
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

PROJECT = Path(__file__).resolve().parent.parent
DATA = PROJECT / "data"

df = pd.read_csv(DATA / "market_history.csv")

# Convert prices from cents to dollars
df["best_bid"] = df["best_bid_cents"] / 100
df["best_ask"] = df["best_ask_cents"] / 100

# Calculate spread as a percentage of midpoint
midpoint = (df["best_bid"] + df["best_ask"]) / 2

df["spread_pct"] = np.where(
    (midpoint > 0) & (df["best_ask"] >= df["best_bid"]),
    100 * (df["best_ask"] - df["best_bid"]) / midpoint,
    np.nan
)

# Total displayed liquidity
df["total_depth"] = df["bid_depth"] + df["ask_depth"]

# Rolling statistics based on previous 50 events
window = 50

for metric in ["spread_pct", "total_depth"]:
    previous = df[metric].shift(1)

    rolling_mean = previous.rolling(
        window, min_periods=window
    ).mean()

    rolling_std = previous.rolling(
        window, min_periods=window
    ).std()

    df[f"{metric}_zscore"] = (
        (df[metric] - rolling_mean) /
        rolling_std.replace(0, np.nan)
    )

# Flag unusually wide spreads
df["spread_alert"] = df["spread_pct_zscore"] > 2

# Flag unusually low liquidity
df["liquidity_alert"] = df["total_depth_zscore"] < -2

# Combined execution risk
df["risk_alert"] = (
    df["spread_alert"] | df["liquidity_alert"]
)

# Export results
df.to_csv(DATA / "risk_signals.csv", index=False)

print("\nSTATISTICAL RISK REPORT")
print("--------------------------")
print("Market events:", len(df))
print("Wide-spread alerts:", int(df["spread_alert"].sum()))
print("Low-liquidity alerts:", int(df["liquidity_alert"].sum()))
print("Total flagged events:", int(df["risk_alert"].sum()))

# Visualize spread anomalies
fig, ax = plt.subplots(figsize=(12, 5))

ax.plot(
    df["event"],
    df["spread_pct"],
    label="Bid-Ask Spread (%)"
)

alerts = df[df["spread_alert"]]

ax.scatter(
    alerts["event"],
    alerts["spread_pct"],
    color="red",
    label="Unusually Wide Spread",
    zorder=3
)

ax.set_title("Statistical Market Risk Detection")
ax.set_xlabel("Simulated Market Event")
ax.set_ylabel("Spread (%)")
ax.legend()
ax.grid(alpha=0.3)

plt.tight_layout()
plt.savefig(DATA / "statistical_risk.png", dpi=200)
plt.show()
ss
print("\nRisk analysis completed successfully!")


