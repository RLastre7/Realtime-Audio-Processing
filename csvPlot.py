import matplotlib.pyplot as plt
import csv
import time

plt.ion()
fig, ax = plt.subplots()
ax.set_ylim(-0.01, 0.01)
ax.set_title("Audio Window")

while True:
    # Exit if window is closed
    if not plt.fignum_exists(fig.number):
        print("Window closed, exiting.")
        break

    xs, ys = [], []

    try:
        with open("data.csv") as f:
            reader = csv.reader(f)
            for row in reader:
                xs.append(float(row[0]))
                ys.append(float(row[1]))
    except FileNotFoundError:
        print("file not found")
        time.sleep(0.05)
        continue
    except Exception:
        continue

    ax.clear()
    ax.plot(xs, ys)


    plt.pause(0.01)
    time.sleep(0.5)

plt.close()
