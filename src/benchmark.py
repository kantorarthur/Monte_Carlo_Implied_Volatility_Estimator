import csv
import random
import statistics
import subprocess
import time
from pathlib import Path

runs = 30
project_dir = Path(__file__).resolve().parent
executable = project_dir / "build" / "monte_carlo"
results_file = project_dir / "runtime_results_10_threads.csv"

times = []

with results_file.open("w", newline="") as file:
    writer = csv.writer(file)
    writer.writerow([
        "run",
        "spot",
        "strike",
        "call_price",
        "risk_free_rate",
        "time_to_expiry",
        "runtime_seconds",
        "exit_code",
        "stdout",
        "stderr",
    ])

    for run in range(1, runs + 1):
        values = [
            random.uniform(80, 120),   # spot
            random.uniform(80, 120),   # strike
            random.uniform(5, 25),     # call price
            random.uniform(0.01, 0.08),# risk-free rate
            random.uniform(0.25, 2.0),  # time to expiry
        ]

        input_data = " ".join(map(str, values)) + "\n"

        start = time.perf_counter()
        result = subprocess.run(
            [str(executable)],
            input=input_data,
            text=True,
            capture_output=True,
            cwd=project_dir,
        )
        elapsed = time.perf_counter() - start

        if result.returncode == 0:
            times.append(elapsed)
        else:
            print(f"Run {run} failed with error code: {result.returncode}.")
            if result.stdout:
                print("stdout:", result.stdout.strip())
            if result.stderr:
                print("stderr:", result.stderr.strip())

        writer.writerow([
            run,
            *values,
            elapsed,
            result.returncode,
            result.stdout.strip(),
            result.stderr.strip(),
        ])

    writer.writerow([])
    if times:
        average = statistics.mean(times)
        writer.writerow(["Successful runs", len(times)])
        writer.writerow(["Average runtime (seconds)", average])
    else:
        average = None
        writer.writerow(["Average runtime (seconds)", "No successful runs"])

if average is not None:
    print(f"Successful runs: {len(times)}/{runs}")
    print(f"Average runtime: {average:.4f} seconds")
else:
    print("No run has succeded. Check the above errors.")

print(f"Results saved to {results_file}")

