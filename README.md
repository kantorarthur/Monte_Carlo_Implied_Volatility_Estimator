# Monte Carlo Implied Volatility Estimator
C++ multithreaded program that estimates what implied volatility factor was used on a European call contract in about 0.037 seconds per contract while also running millions of simulations per contract.

The program approximates the implied volatility of a European call option using Monte Carlo simulation of geometric Brownian motion (GBM) under the Black–Scholes model, implemented in C++ using the new C++26 std::philox_engine and multithreading.

For each candidate volatility, the program simulates the asset price at expiration, computes the call payoff, discounts the average payoff at the risk-free rate, and compares the resulting price with the contract price you enter.

# Context
**European call option:** a contract that gives the buyer the right, but not the obligation, to buy an asset at a fixed price (the *strike*) on a fixed date (the *expiration*). "European" means it can only be exercised at expiration, not before.

- If the asset's price at expiration is **above** the strike, the buyer exercises and earns the difference.
- If it is **below**, the buyer lets the option expire and loses only the price paid for the contract (the *premium*).

- **Implied volatility (IV):** the premium depends on how much the asset's price is expected to move. The IV is the volatility that makes the Black–Scholes price match the premium actually quoted, but it is rarely shown to the buyer. This program estimates it from the contract's terms.

**Black–Scholes** it's a model that prices a European call without trying to predict where the stock will go. It assumes money has a time value (a dollar received at expiration is worth less than a dollar today, so future amounts are discounted at the risk-free rate) and that the stock grows on average at that same risk-free rate. The price is the average payoff over all the possible price paths, discounted back to today. The only unknown input is the volatility, which is what this program solves for.

## Use case

You want to buy a European call option, but the quoted premium doesn't tell you what volatility is built into the price. Enter the contract's spot price, strike, premium, risk-free rate and time to expiry, and the program estimates the volatility that price implies under Black–Scholes. You can then judge whether the contract looks expensive or cheap, for example by comparing the result with how volatile the asset has actually been.

# Motivation
While studying about the "Black-Scholes" model ([**Personal study papers on "Black-Scholes" model and Monte Carlo Simulation, wrote in Romanian**](https://github.com/kantorarthur/Monte_Carlo_Black_Scholes_Study_Papers)) used in pricing European call contracts I realized that when you buy a call option, the price you pay implies a level of volatility, but that figure is rarely shown to you.

So I though about making this tool that estimates it from the contract’s spot price, strike, price, risk-free rate and time to expiry, so you can see what volatility the market price implies under the Black–Scholes model and decide if you want to move further with the contract.
## How it works

1. Sweep volatility from `0.05` to `0.40` in steps of `0.05`.
2. Once the estimated price reaches or exceeds the contract price, step back down in increments of `0.01`.
3. Stop when an estimate is within the price tolerance (`0.15`) of the contract price, or when the estimate drops below the contract price (no match found).

Each estimate uses 10 threads with 100,000 simulations per thread (1,000,000 paths in total). Every tested volatility and its estimated price is written to `prices.csv`, and if a volatility is found, `plot.py` plots call price against volatility.

At the end of the program a plot will show up that shows the comparations made by the program vs. the actual contract numbers.

<img width="635" height="521" alt="Image" src="https://github.com/user-attachments/assets/df6fd0eb-a7bd-4345-8a3c-e0838aef0d5e" />

**Inputs used for this plot: Spot Price - 100, Strike Price - 100, Call Option Price - 10.45, Risk-Free Rate - 0.05, Time to expiry - 1**

Based on this example plot we can see that our program began to search for the volatility factor starting from 0.05, increased until he found a volatility factor greater than the actual one and began decreasing with 0.01 until he got to the actual volatility factor that was used in the contract.

## Requirements

- CMake 3.25 or later
- A C++ compiler that supports C++26 draft mode (`-std=c++2c`) and `std::philox4x32`
- A compatible C++ standard library at runtime
- Python 3 with `matplotlib` (for the plot)

## Build and run

```bash
#Clone the repository
git clone https://github.com/kantorarthur/Monte_Carlo_Implied_Volatility_Estimator

#Enter the main directory of the repository
cd Monte_Carlo_Implied_Volatility_Estimator

#Compile the executable
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release 
cmake --build build --clean-first -j 

#Execute the program
./build/implied_volatility

#Enter the inputs of the contract in this order:
#1.(Spot price) 2.(Strike Price) 3.(Call option price) 4.(Risk-free rate) 5.(Time to expiry (in years))
```

> **Note:** The CMake configuration adds the compiler's libstdc++ directory to the executable's RPATH so it can find the matching runtime library on the machine where it was built. It does not bundle libstdc++ and does not guarantee the executable runs on other machines. Rebuild on the target machine with a compatible compiler and standard library.


## Build and run video demo
https://github.com/user-attachments/assets/cf199ccc-7e26-4770-a8f5-e2d985475466

## Why Philox?

`std::philox4x32` is a counter-based random number generator added to the C++ standard library in C++26. Unlike `std::mt19937`, which advances a large internal state to produce each value, Philox generates values from a counter and a key. That suits parallel simulation: each thread can use a different key (here, `42 + thread_index`) to get an independent random stream with no shared generator state and no synchronization. The counter-based design also makes a given stream reproducible from the same key and counter.


**IMPORTANT NOTE: Philox shouldn't be used for cryptography**

Philox is designed for statistical quality and speed in simulations, not for security. It is fully deterministic: the same key and counter always produce the same output. That is what makes simulations reproducible, but it also means the output is predictable to anyone who knows or guesses the key, and the keys used here (`42 + thread_index`) are small and fixed. It must not be used to generate passwords, tokens, or encryption keys. Use a cryptographically secure generator for those.

The same determinism is a strength for simulation. Because Philox is counter-based, any value in a stream is computed directly from the key and a counter, so you can jump to any position without generating the values before it. With `std::mt19937`, reaching a given position means advancing the internal state step by step through every earlier value. This makes Philox well suited to splitting work across threads and to reproducing or revisiting a specific part of a simulation.
## Formulas

The program combines three formulas to price a European call option by Monte Carlo simulation under Black–Scholes.

### 1. Simulated asset price at expiration
<img width="1047" height="102" alt="Image" src="https://github.com/user-attachments/assets/89b72e2c-3a0e-4e21-8ae7-5cf134913d62" />

This is the closed-form solution of geometric Brownian motion under risk-neutral pricing. It gives one possible price of the asset at expiration, $T$, starting from today's price $S_0$.

- $S_0$: current asset price
- $r$: risk-free interest rate
- $\sigma$: volatility (the unknown the program searches for)
- $T$: time to expiration, in years
- $Z$: a random draw from the standard normal distribution

The exponent has two parts. The deterministic part, $\left(r-\tfrac{1}{2}\sigma^2\right)T$, is the drift: the asset grows at the risk-free rate, minus a $\tfrac{1}{2}\sigma^2$ correction that keeps the expected price consistent with risk-neutral pricing. The random part, $\sigma\sqrt{T}\,Z$, is the random movement, which grows with volatility and with the square root of time. Each simulation draws a fresh $Z$, so each one produces a different possible future price.

### 2. European call payoff
<img width="728" height="155" alt="Image" src="https://github.com/user-attachments/assets/69cd42cd-c603-4705-a7a3-721ff1e551f9" />

A European call gives the holder the right to buy the asset at the strike price $K$ at expiration. If $S_T > K$, exercising earns the difference $S_T-K$. If $S_T \le K$, the holder does not exercise and the payoff is zero, so it can never be negative.

### 3. Estimated call price today

<img width="999" height="219" alt="Image" src="https://github.com/user-attachments/assets/f48df841-ad42-46d8-92d8-2e02677b52ff" />

The inner sum adds up the payoffs of all N simulated paths, and dividing by N gives the average payoff, which estimates the expected payoff at expiration. Multiplying by $e^{-rT}$ discounts that expected future payoff back to today at the risk-free rate. The result, $C_0$ , is the model's price of the option for the chosen $\sigma$.

Higher volatility makes the call more valuable (the payoff is capped below at zero but unbounded above), so $C_0$ rises with $\sigma$. That is why the program can compare $C_0$ with the contract price for a series of volatilities and find the one that matches, which is the implied volatility.

## Code explained

**(1) - Simulate function**

<img width="685" height="437" alt="Image" src="https://github.com/user-attachments/assets/4b1b6837-2981-424d-9299-2916435743ab" />

**Before the simulate function we set 3 global variables which are: Number of simulations each thread needs to do per volatility variabile check, number of threads we are using and the error tolerance factor we accept between the call option price we found and the actual call option price from the contract.**

1. Each thread creates its own Philox generator using the seed **42 + i** where i is the thread number. Every thread has a different seed, so each one gets an independent stream of random numbers, and no generator state is shared between threads.

2. The normal distribution is used to turn the raw random numbers generated using the Philox generator, into draws *z* from the standard normal distribution.

3. For each of the 100,000 simulations, the thread draws a new z and computes the asset price at expiration with formula 1, using the current volatility $\sigma$ .

4. **expected_price** implements the formula **1** which can be seen above.

5. **Payoff**: it applies formula **2**, max(price - strike, 0), and adds the result to a running sum.

**(2) - Actual volatility factor Loop-check**

<img width="785" height="622" alt="Image" src="https://github.com/user-attachments/assets/887669e3-b1ed-490e-b79c-8be60dca9267" />

1. The loop starts using an arbitrary volatility factor of 0.05 and it increases up until 0.40 if the actual volatility factor wasn't found until yet.

2. The second loop starts (number_of_threads) threads. Thread i calls simulate with its own seed (42 + i) and writes its payoff sum into its own slot, payoff_sums[i]. The lambda captures everything by reference, except i, which is copied so each thread keeps its own value. Since every thread writes to a different array element, there is no data race.

3. After each thread finished his work, we compute the average_payoff using the arithmetic median on the sum of payoffs computed by each thread.

**(3) - Loop after finding the first volatility factor greater than the actual volatility factor we are looking for**

<img width="617" height="824" alt="Image" src="https://github.com/user-attachments/assets/cbc3cc79-f233-45f2-9479-7d3999ac79c9" />

Once we find the first volatility factor greater than the actual volatility factor of the contract, we first check if that's the actual volatility factor we are looking for.

If not, we begin decreasing the volatility factor by 0.01 each time until we either find the volatility factor of the contract or we get to 0.


## Core pinning

For the pinned benchmark, each worker thread binds itself to a single CPU core with `sched_setaffinity`, so thread `i` runs on CPU `i`:

```cpp
cpu_set_t set;
CPU_ZERO(&set);
CPU_SET(static_cast<int>(i), &set);
if (sched_setaffinity(0, sizeof(set), &set) == -1)
{
    std::cerr << "sched_setaffinity failed: "
              << std::strerror(errno) << '\n';
}
```
This trivial implementation was used only for the benchmarks, which showed worse results in terms of runtime speed.

## Runtime benchmarks

I measured wall-clock runtime with `benchmark.py`, which launches the executable 30 times per configuration with randomized inputs and records the mean.

I also benchmarked the program using a trivial implementation of "core pinning" technique, but I found it as being worse in terms of runtime. 
The best results I got was using 10 threads without core pinning. 

The median for estimating the volatility factor for 30 contracts using 10 threads without core pinning was 0.037 seconds/contract.

| Configuration | Successful runs | Mean runtime | Median |
|---|---|---|---|
| 4 threads | 30/30 | 0.0604 s | 0.0603 s |
| 10 threads, core pinning | 30/30 | 0.0412 s | 0.0418 s |
| 10 threads, no core pinning | 30/30 | 0.0375 s | 0.0375 s|

**Caveats**

- Inputs are randomized on every run and are not seeded, so each configuration saw different inputs. Because the search stops early in some cases, runtime depends on the input.
- Times are measured from outside the process, so they include process startup.
- This is a small, informal benchmark, not a controlled study. Results will vary by machine.
