import matplotlib.pyplot as plt
import re

NORMAL_COLOR = "r"
GRAVEYARD_COLOR = "b"
GRAVEYARD_COLOR1 = "g"
GRAVEYARD_COLOR2 = "y"
GRAVEYARD_COLOR3 = "m"

def construct_graph(fullness, num_insertions, time_taken, y_label, title, dvzp=0):
    throughput = [[o / (t+dvzp) for (o, t) in zip(num_insertions[i], time_taken[i])] for i in range(5)]

    plt.plot(fullness[0], throughput[0], color=NORMAL_COLOR, label = "Normal QF")
    plt.plot(fullness[1], throughput[1], color=GRAVEYARD_COLOR, label="GF (No Redistribution)")
    plt.plot(fullness[2], throughput[2], color=GRAVEYARD_COLOR1, label="GF (Graveyard Hashing Policy)")
    plt.plot(fullness[3], throughput[3], color=GRAVEYARD_COLOR2, label="GF (Between-Runs Policy)")
    plt.plot(fullness[4], throughput[4], color=GRAVEYARD_COLOR3, label="GF (Clean-Up Policy)")
    plt.xlabel("Percent Full")
    plt.ylabel(y_label)
    plt.legend(loc="upper right")
    plt.title(title)

def parse_insertions(filename):
    results = {
        "fullness": [],
        "insert_count": [],
        "insert_time": [],
        "rquery_count": [],
        "rquery_time": [],
        "squery_count": [],
        "squery_time": []
    }
    with open(filename, "r") as f:
        all_lines = f.readlines()
        for line in all_lines:
            clean_line = line.strip()
            # print(clean_line)
            # print("line end")
            matchObj = re.match(pattern="\s*Current Fullness: ([\d\.]*) Number inserted: (\d+) Time taken: (\d+) microseconds",string=clean_line)
            matchObj2 = re.match(pattern="\s*(\d+) random queries in (\d+) seconds", string=clean_line)
            matchObj3 = re.match(pattern="\s*(\d+) successful queries in (\d+) seconds", string=clean_line)
            if matchObj:
                nums = matchObj.groups()
                results["fullness"].append(float(nums[0]))
                results["insert_count"].append(float(nums[1]))
                results["insert_time"].append(float(nums[2])/1000000)
            if matchObj2:
                nums = matchObj2.groups()
                results["rquery_count"].append(float(nums[0]))
                results["rquery_time"].append(float(nums[1]))
            if matchObj3:
                nums = matchObj3.groups()
                results["squery_count"].append(float(nums[0]))
                results["squery_time"].append(float(nums[1]))
    return results

def parse_deletions(filename):
    results = {
        "fullness": [],
        "delete_count": [],
        "delete_time": [],
        "rquery_count": [],
        "rquery_time": [],
        "squery_count": [],
        "squery_time": []
    }
    with open(filename, "r") as f:
        all_lines = f.readlines()
        for line in all_lines:
            clean_line = line.strip()
            matchObj = re.match(pattern="\s*Current Fullness: ([\d\.]*) Number deleted: (\d+) Time taken: (\d+) microseconds",string=clean_line)
            matchObj2 = re.match(pattern="\s*(\d+) random queries in (\d+) seconds", string=clean_line)
            matchObj3 = re.match(pattern="\s*(\d+) successful queries in (\d+) seconds", string=clean_line)
            if matchObj:
                nums = matchObj.groups()
                results["fullness"].append(float(nums[0]))
                results["delete_count"].append(float(nums[1]))
                results["delete_time"].append(float(nums[2])/1000000)
            if matchObj2:
                nums = matchObj2.groups()
                results["rquery_count"].append(float(nums[0]))
                results["rquery_time"].append(float(nums[1]))
            if matchObj3:
                nums = matchObj3.groups()
                results["squery_count"].append(float(nums[0]))
                results["squery_time"].append(float(nums[1]))
    return results

def parse_mixed(filename):
    results = {
        "fullness": [],
        "insert_count": [],
        "insert_time": [],
        "delete_count": [],
        "delete_time": [],
        "rquery_count": [],
        "rquery_time": [],
        "squery_count": [],
        "squery_time": []
    }
    with open(filename, "r") as f:
        all_lines = f.readlines()
        for line in all_lines:
            clean_line = line.strip()
            matchObj = re.match(pattern="\s*Current Fullness: ([\d\.]*) Number deleted: (\d+) Time taken: (\d+) microseconds",string=clean_line)
            matchObj2 = re.match(pattern="\s*Current Fullness: ([\d\.]*) Number inserted: (\d+) Time taken: (\d+) microseconds",string=clean_line)
            matchObj3 = re.match(pattern="\s*(\d+) random queries in (\d+) seconds", string=clean_line)
            matchObj4 = re.match(pattern="\s*(\d+) successful queries in (\d+) seconds", string=clean_line)
            if matchObj:
                nums = matchObj.groups()
                results["fullness"].append(float(nums[0]))
                results["delete_count"].append(float(nums[1]))
                results["delete_time"].append(float(nums[2])/1000000)
            if matchObj2:
                nums = matchObj2.groups()
                results["insert_count"].append(float(nums[1]))
                results["insert_time"].append(float(nums[2])/1000000)
            if matchObj3:
                nums = matchObj3.groups()
                results["rquery_count"].append(float(nums[0]))
                results["rquery_time"].append(float(nums[1]))
            if matchObj4:
                nums = matchObj4.groups()
                results["squery_count"].append(float(nums[0]))
                results["squery_time"].append(float(nums[1]))
    return results


testData = [
    ("perfInsert", parse_insertions, "Insertions Only", True, False),
    ("perfDelete", parse_deletions, "Deletions Only", False, True),
    ("perfMixed", parse_mixed, "Mixed Insertions and Deletions", True, True)
]

for (current_test, parse_function, test_subtitle, graph_inserts, graph_deletes) in testData:
    perfResults = [
        parse_function(f"perf_tests/results/normal_{current_test}.txt"),
        parse_function(f"perf_tests/results/graveyard_noredis_{current_test}.txt"),
        parse_function(f"perf_tests/results/graveyard_evenlydist_{current_test}.txt"),
        parse_function(f"perf_tests/results/graveyard_betweenruns_{current_test}.txt"),
        parse_function(f"perf_tests/results/graveyard_betweenrunsinsert_{current_test}.txt")]

    if (graph_inserts):
        construct_graph(
            [r["fullness"] for r in perfResults],
            [r["insert_count"] for r in perfResults],
            [r["insert_time"] for r in perfResults],
            "Insertion Throughput", f"{test_subtitle}: Inserts", 0.0000001)
        plt.show()
        plt.clf()
    if (graph_deletes):
        construct_graph(
            [r["fullness"] for r in perfResults],
            [r["delete_count"] for r in perfResults],
            [r["delete_time"] for r in perfResults],
            "Deletion Throughput", f"{test_subtitle}: Deletes", 0.0000001)
        plt.show()
        plt.clf()

    construct_graph(
        [r["fullness"] for r in perfResults],
        [r["rquery_count"] for r in perfResults],
        [([10] * len(r["rquery_time"])) for r in perfResults],
        "Query Throughput", f"{test_subtitle}: Uniform Random Lookups")
    plt.show()
    plt.clf()
    construct_graph(
        [r["fullness"] for r in perfResults],
        [r["squery_count"] for r in perfResults],
        [([10] * len(r["squery_time"])) for r in perfResults],
        "Query Throughput", f"{test_subtitle}: Successful Lookups")
    plt.show()
    plt.clf()


