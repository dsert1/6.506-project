from ctypes import *
from contextlib import contextmanager
import unittest
import random
import time

quotient_filter = cdll.LoadLibrary("./quotient_filter.so")

@CFUNCTYPE(c_int, c_int)
def hashFunction(value):
    # TODO: replace with our hash fn
    return value % 100

# define quotient filter using ctypes
class QuotientFilter(object):
    def __init__(self, q):
        self.obj = quotient_filter.QuotientFilter_new(q, hashFunction)
        
    def insertElement(self, value):
        quotient_filter.QuotientFilter_insertElement(self.obj, value)
        
    def deleteElement(self, value):
        quotient_filter.QuotientFilter_deleteElement(self.obj, value)
        
    def query(self, value):
        return quotient_filter.QuotientFilter_query(self.obj, value)

    def __del__(self):
        quotient_filter.QuotientFilter_delete(self.obj)

class TestQuotientFilter(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Initialize a QuotientFilter object with q = 100000
        cls.qf = QuotientFilter(100000)

    @classmethod
    def tearDownClass(cls):
        del cls.qf

    # insert elements until 5% filled, then perform uniform random lookups and successful lookups
    @contextmanager
    def test_1(self, n):
        cf = self.qf(capacity=100000)
        start = time.time()
        for i in range(n):
            x = random.randint(0, 2**64-1)
            cf.insert(x)
        insert_time = time.time() - start

        start = time.time()
        for i in range(n):
            x = random.randint(0, 2**64-1)
            cf.query(x)
        uniform_time = time.time() - start

        start = time.time()
        for i in range(n):
            x = random.choice(cf._items)
            cf.query(x)
        successful_time = time.time() - start

        yield (insert_time, uniform_time, successful_time)

    # insert elements until 95% filled, then perform uniform random lookups and successful lookups
    def test_2(self, n):
        cf = self.qf(capacity=100000)
        fill_target = 0.95
        fill = 0.0
        while fill < fill_target:
            item = str(random.randint(0, n))
            cf.insert(item)
            fill = cf.size() / cf.capacity()
        
        start_time = time.time()
        for i in range(n):
            item = str(random.randint(0, n))
            cf.contains(item)
        uniform_time = time.time() - start_time
        
        start_time = time.time()
        for item in cf:
            cf.contains(item)
        successful_time = time.time() - start_time
        
        return uniform_time, successful_time

    # mix of inserts, deletes, and lookups for a fixed amount of time
    def test_3(self, n):
        cf = self.qf
        mix_time = 60 # seconds
        start_time = time.time()
        while time.time() - start_time < mix_time:
            # Insert 10%
            insert_target = cf.capacity() * 0.1
            insert_count = 0
            while insert_count < insert_target:
                item = str(random.randint(0, n))
                if cf.insert(item):
                    insert_count += 1
            
            # Delete 5%
            delete_target = cf.size() * 0.05
            delete_count = 0
            for item in cf:
                if delete_count >= delete_target:
                    break
                if cf.delete(item):
                    delete_count += 1
            
            # Perform lookups
            for i in range(100):
                item = str(random.randint(0, n))
                cf.contains(item)
        
        return




# Initialize the TestQuotientFilter object
qf_test = TestQuotientFilter()

# run tests
n = 10000
test_1_results = qf_test.test_1(n)
test_2_results = qf_test.test_2(n)
test_3_results = qf_test.test_3(n)

# results
print("Test #1: Insert time = {:.4f}s, uniform time = {:.4f}s, successful time = {:.4f}s".format(*test_1_results))
print("Test #2: Delete time = {:.4f}s, uniform time = {:.4f}s, successful time = {:.4f}s".format(*test_2_results))
print("Test #3: Mix time = {:.4f}s, uniform time = {:.4f}s, successful time = {:.4f}s".format(*test_3_results))
