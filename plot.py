import csv
import matplotlib.pyplot as plt

sigmas = []
estimated_prices = []
contract_prices = []

with open("prices.csv", newline="") as csvfile:
    reader = csv.DictReader(csvfile)

    for row in reader:
        sigmas.append(float(row["sigma"]))
        estimated_prices.append(float(row["estimated_call_price"]))
        contract_prices.append(float(row["contract_call_price"]))

plt.plot(sigmas, estimated_prices, marker="o", label="Monte Carlo estimate")
plt.axhline(contract_prices[0], color="red", linestyle="--",
            label="Contract price")

plt.xlabel("Volatility (sigma)")
plt.ylabel("Call option price")
plt.title("Call price vs. volatility")
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()
