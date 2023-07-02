import matplotlib.pyplot as plt
values = []
with open('element_shifting_time.txt', 'r') as f:
    for line in f.readlines():
        values.append(int(line.strip()))

values2 = []
with open('tombstone_shifting_time.txt', 'r') as f:
    for line in f.readlines():
        values2.append(int(line.strip()))

plt.plot(values, color='r', label='QF')
plt.plot(values2, color='g', label='QF-GH')
plt.legend()
plt.show()