
from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt

PROJECT = Path(__file__).resolve().parent.parent
DATA = PROJECT / "data"

df = pd.read_csv(DATA / "slippage.csv")

fig, ax1 = plt.subplots(figsize=(11, 6))

# Slippage curve
ax1.plot(
    df["requested"],
    df["slippage_pct"],
    marker="o",
    linewidth=2,
    label="Slippage"
)
ax1.set_xlabel("Requested Buy Order Size (Shares)")
ax1.set_ylabel("Slippage on Filled Shares (%)")
ax1.grid(alpha=0.3)

# Fill rate on second axis
ax2 = ax1.twinx()
ax2.plot(
    df["requested"],
    df["fill_rate"],
    marker="s",
    linestyle="--",
    label="Fill Rate"
)
ax2.set_ylabel("Fill Rate (%)")
ax2.set_ylim(0, 105)

fig.suptitle("Execution Cost and Liquidity Analysis")

lines = ax1.get_lines() + ax2.get_lines()
ax1.legend(
    lines,
    [line.get_label() for line in lines],
    loc="upper left"
)

plt.tight_layout()
plt.savefig(DATA / "slippage_curve.png", dpi=200)

print(df.to_string(index=False))
print("\nSlippage chart saved successfully!")

plt.show()
