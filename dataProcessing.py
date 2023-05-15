import matplotlib.pyplot as plt
import re

NORMAL_COLOR = "r"
GRAVEYARD_COLOR = "b"
GRAVEYARD_COLOR1 = "g"
GRAVEYARD_COLOR2 = "y"
GRAVEYARD_COLOR3 = "m"
def graph_labels(filename):
    params = []
    test_name_parts = filename.split("_")
    filter_type = test_name_parts[0]
    test_name = test_name_parts[1]
    redist_type = test_name_parts[2]
    if filter_type == "normal":
        params.append("QUOTIENT FILTER")
    else:
        params.append("QUOTIENT FILTER WITH GRAVEYARD HASHING")
    if test_name == "perfInsert":
        params.append("Insertion Throughput")
    elif test_name == "perfDelete":
        params.append("Delete Throughput")
    else:
        params.append("Mixed Throughput")
    params.append(redist_type)
    return params

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
            matchObj = re.match(pattern="Current Fullness: ([\d\.]*) Number of deletions: (\d+) Time taken: (\d+) microseconds",string=clean_line)
            matchObj2 = re.match(pattern="(\d+) random queries in 60 seconds")
            matchObj3 = re.match(pattern="(\d+) successful queries in 60 seconds")
            if matchObj:
                nums = matchObj.groups()
                fullness.append(nums[0])
                num_deletions.append(nums[1])
                time_taken.append(nums[2])
            if matchObj2:
                nums = matchObj2.groups()
                num_success.append(nums[0])
            if matchObj3:
                nums = matchObj3.groups()
                num_random.append(nums[0])
    return fullness, num_deletions,time_taken, num_success, num_random

def construct_graph_deletions(fullness, time_taken, num_success, num_random):
    print(fullness[0])
    print(time_taken[0])
    plt.scatter(fullness[0], time_taken[0], color=NORMAL_COLOR, label = "normal")
    plt.scatter(fullness[1], time_taken[1], color=GRAVEYARD_COLOR, label="no-redist")
    plt.scatter(fullness[2], time_taken[2], color=GRAVEYARD_COLOR1, label="clean-up")
    plt.scatter(fullness[3], time_taken[3], color=GRAVEYARD_COLOR2, label="between-runs")
    plt.scatter(fullness[4], time_taken[4], color=GRAVEYARD_COLOR3, label="insert-redist")
    plt.xlabel("Proportion of Capacity filled")
    plt.ylabel("Time to insert in microseconds")
    plt.legend(loc="upper right")
    plt.title("Insertion throughput")
    plt.show()
    # plt.clf()
    # plt.plot(fullness[0], num_success[0], NORMAL_COLOR, label = "normal")
    # plt.plot(fullness[1], num_success[1], GRAVEYARD_COLOR, label="no-redist")
    # plt.plot(fullness[2], num_success[2], GRAVEYARD_COLOR1, label="clean-up")
    # plt.plot(fullness[3], num_success[3], GRAVEYARD_COLOR2, label="between-runs")
    # plt.plot(fullness[4], num_success[4], GRAVEYARD_COLOR3, label="insert-redist")
    # plt.xlabel("Proportion of Capacity filled")
    # plt.ylabel("Number of successful lookups")
    # plt.legend(loc="upper left")
    # plt.show()
    # plt.clf()
    # plt.plot(fullness[0], num_random[0], NORMAL_COLOR, label = "normal")
    # plt.plot(fullness[1], num_random[1], GRAVEYARD_COLOR, label="no-redist")
    # plt.plot(fullness[2], num_random[2], GRAVEYARD_COLOR1, label="clean-up")
    # plt.plot(fullness[3], num_random[3], GRAVEYARD_COLOR2, label="between-runs")
    # plt.plot(fullness[4], num_random[4], GRAVEYARD_COLOR3, label="insert-redist")
    # plt.xlabel("Proportion of Capacity filled")
    # plt.ylabel("Number of random lookups")
    # plt.legend(loc="upper left")
    # plt.show()

def parse_insertions(filename):
    fullness = []
    num_deletions = []
    time_taken = []
    num_success = []
    num_random = []
    with open(filename, "r") as f:
        all_lines = f.readlines()
        for line in all_lines:
            clean_line = line.strip()
            print(clean_line)
            print("line end")
            matchObj = re.match(pattern="Current Fullness: ([\d\.]*) Number inserted: (\d+) Time taken: (\d+) microseconds",string=clean_line)
            matchObj2 = re.match(pattern="(\d+) random queries in 60 seconds", string=clean_line)
            matchObj3 = re.match(pattern="(\d+) successful queries in 60 seconds", string=clean_line)
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
    # print(fullness, num_deletions, time_taken, num_success, num_random)
    return fullness, num_deletions,time_taken, num_success, num_random
    # fullness = []
    # num_insertions = []
    # time_taken = []
    # with open(filename, "r") as f:
    #     all_lines = f.readlines()
    #     for line in all_lines:
    #         clean_line = line.strip()
    #         matchObj = re.match(pattern="Current Fullness: ([\d\.]*). Number inserted: (\d+) Time taken: (\d+) microseconds",string=clean_line)
    #         print(matchObj)
    #         if matchObj:
    #             nums = matchObj.groups()
    #             print(nums)
    #             fullness.append(float(nums[0]))
    #             num_insertions.append(float(nums[1]))
    #             time_taken.append(float(nums[2]))
    # return fullness, num_insertions,time_taken


f,n,t,ns,nr = parse_insertions("normal_perfInsert.txt")
# print(f)
# print(n)
# print(t)
f1,n1,t1,ns1, nr1 = parse_insertions("graveyard_perfInsert_evenlydistribute.txt")
f2,n2,t2,ns2, nr2 = parse_insertions("graveyard_perfInsert_betweenruns.txt")
f3,n3,t3,ns3, nr3 = parse_insertions("graveyard_perfInsert_betweenrunsinsert.txt")
f4,n4,t4, ns4, nr4 = parse_insertions("graveyard_perfInsert_noredis.txt")

construct_graph_deletions([f,f1,f2,f3,f4], [t,t1,t2,t3,t4], [ns, ns1,ns2,ns3,ns4], [nr,nr1,nr2,nr3,nr4])


