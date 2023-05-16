import matplotlib.pyplot as plt
import re

NORMAL_COLOR = "r"
GRAVEYARD_COLOR = "b"
GRAVEYARD_COLOR1 = "g"
GRAVEYARD_COLOR2 = "y"
GRAVEYARD_COLOR3 = "m"
def construct_graph(fullness, time_taken, num_success, num_random,  title, ylabel):
    # print(fullness[1])
    # print(num_success[1])
    plt.scatter(fullness[0], time_taken[0], color=NORMAL_COLOR, label = "normal")
    plt.scatter(fullness[1], time_taken[1], color=GRAVEYARD_COLOR, label="insert-redist")
    plt.scatter(fullness[2], time_taken[2], color=GRAVEYARD_COLOR1, label="clean-up")
    plt.scatter(fullness[3], time_taken[3], color=GRAVEYARD_COLOR2, label="between-runs")
    plt.scatter(fullness[4], time_taken[4], color=GRAVEYARD_COLOR3, label="no-redist")
    plt.xlabel("Proportion of Capacity filled")
    plt.ylabel(ylabel)
    plt.legend(loc="upper right")
    plt.title(title)
    plt.show()
    plt.clf()
    plt.scatter(fullness[0], num_success[0], color=NORMAL_COLOR, label = "normal")
    plt.scatter(fullness[1], num_success[1], color=GRAVEYARD_COLOR, label="insert-redist")
    plt.scatter(fullness[2], num_success[2], color=GRAVEYARD_COLOR1, label="clean-up")
    plt.scatter(fullness[3], num_success[3], color=GRAVEYARD_COLOR2, label="between-runs")
    plt.scatter(fullness[4], num_success[4], color=GRAVEYARD_COLOR3, label="no-redist")
    plt.xlabel("Proportion of Capacity filled")
    plt.ylabel("Number of successful lookups")
    plt.legend(loc="upper right")
    plt.title("Successful lookup throughput")
    plt.show()
    plt.clf()
    plt.scatter(fullness[0], num_random[0], color=NORMAL_COLOR, label = "normal")
    plt.scatter(fullness[1], num_random[1], color=GRAVEYARD_COLOR, label="insert-redist")
    plt.scatter(fullness[2], num_random[2], color=GRAVEYARD_COLOR1, label="clean-up")
    plt.scatter(fullness[3], num_random[3], color=GRAVEYARD_COLOR2, label="between-runs")
    plt.scatter(fullness[4], num_random[4], color=GRAVEYARD_COLOR3, label="no-redist")
    plt.xlabel("Proportion of Capacity filled")
    plt.ylabel("Number of random lookups")
    plt.legend(loc="upper right")
    plt.title("Random lookup throughput")
    plt.show()

def parse_insertions(filename):
    fullness = []
    num_insertions = []
    time_taken = []
    num_success = []
    num_random = []
    with open(filename, "r") as f:
        all_lines = f.readlines()
        for line in all_lines:
            clean_line = line.strip()
            print(clean_line)
            print("line end")
            matchObj = re.match(pattern="\s*Current Fullness: ([\d\.]*) Number inserted: (\d+) Time taken: (\d+) microseconds",string=clean_line)
            matchObj2 = re.match(pattern="\s*(\d+) random queries in \d+ seconds", string=clean_line)
            matchObj3 = re.match(pattern="\s*(\d+) successful queries in \d+ seconds", string=clean_line)
            if matchObj:
                nums = matchObj.groups()
                fullness.append(float(nums[0]))
                num_insertions.append(float(nums[1]))
                time_taken.append(float(nums[2]))
            if matchObj2:
                nums = matchObj2.groups()
                num_success.append(float(nums[0]))
            if matchObj3:
                nums = matchObj3.groups()
                num_random.append(float(nums[0]))
    return fullness, num_insertions,time_taken, num_success, num_random

def parse_deletions(filename):
    fullness = []
    num_deletions = []
    time_taken = []
    num_success = []
    num_random = []
    with open(filename, "r") as f:
        all_lines = f.readlines()
        for line in all_lines:
            clean_line = line.strip()
            matchObj = re.match(pattern="\s*Current Fullness: ([\d\.]*). Number deleted: (\d+) Time taken: (\d+) microseconds",string=clean_line)
            matchObj2 = re.match(pattern="\s*(\d+) random queries in \d+ seconds", string=clean_line)
            matchObj3 = re.match(pattern="\s*(\d+) successful queries in \d+ seconds", string=clean_line)
            if matchObj:
                nums = matchObj.groups()
                fullness.append(float(nums[0]))
                num_deletions.append(float(nums[1]))
                time_taken.append(float(nums[2]))
            if matchObj2:
                nums = matchObj2.groups()
                num_success.append(float(nums[0]))
            if matchObj3:
                nums = matchObj3.groups()
                num_random.append(float(nums[0]))
    return fullness, num_deletions,time_taken, num_success, num_random

def parse_mixed(filename):
    fullness = []
    num_deletions = []
    num_insertions = []
    time_taken = []
    num_success = []
    num_random = []
    with open(filename, "r") as f:
        all_lines = f.readlines()
        for line in all_lines:
            clean_line = line.strip()
            matchObj = re.match(pattern="\s*Current Fullness: ([\d\.]*). Number deleted: (\d+) Time taken: (\d+) microseconds",string=clean_line)
            matchObj2 = re.match(pattern="\s*Current Fullness: ([\d\.]*) Number inserted: (\d+) Time taken: (\d+) microseconds",string=clean_line)
            matchObj3 = re.match(pattern="\s*(\d+) random queries in \d+ seconds", string=clean_line)
            matchObj4 = re.match(pattern="\s*(\d+) successful queries in \d+ seconds", string=clean_line)
            if matchObj:
                nums = matchObj.groups()
                fullness.append(float(nums[0]))
                num_deletions.append(float(nums[1]))
                time_taken.append(float(nums[2]))
            if matchObj2:
                nums = matchObj.groups()
                fullness.append(float(nums[0]))
                num_insertions.append(float(nums[1]))
                time_taken.append(float(nums[2]))
            if matchObj3:
                nums = matchObj2.groups()
                num_success.append(float(nums[0]))
            if matchObj4:
                nums = matchObj3.groups()
                num_random.append(float(nums[0]))
    return fullness, num_deletions,time_taken, num_success, num_random

#Example processing
f,n,t,ns,nr = parse_insertions("normal_perfInsert.txt")
f1,n1,t1,ns1, nr1 = parse_insertions("graveyard_perfInsert_evenlydistribute.txt")
f2,n2,t2,ns2, nr2 = parse_insertions("graveyard_perfInsert_betweenruns.txt")
f3,n3,t3,ns3, nr3 = parse_insertions("graveyard_perfInsert_betweenrunsinsert.txt")
f4,n4,t4, ns4, nr4 = parse_insertions("graveyard_perfInsert_noredis.txt")

construct_graph(fullness=[f,f1,f2,f3,f4], time_taken=[t, t1, t2,t3,t4], num_success=[ns, ns1, ns2, ns3, ns4], num_random=[nr, nr1, nr2, nr3, nr4], 
                title="Insertion throughput", ylabel="Insertion throughput")



