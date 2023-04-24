import unittest
import time
from ctypes import *
from contextlib import contextmanager

# import c++
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
        # Initialize a QuotientFilter object with q = 8
        cls.qf = QuotientFilter(8)
        
    @classmethod
    def tearDownClass(cls):
        del cls.qf
        
    @contextmanager
    def time_test(self):
        start_time = time.monotonic()
        yield
        end_time = time.monotonic()
        print("Elapsed time:", end_time - start_time)
        
    def test_insertion_1(self):
        with self.time_test():
            for i in range(10000):
                self.qf.insertElement(i)
                
    def test_deletion_1(self):
        with self.time_test():
            for i in range(10000):
                self.qf.deleteElement(i)
                
    def test_query_1(self):
        with self.time_test():
            for i in range(10000):
                self.qf.query(i)
                
    def test_insertion_2(self):
        with self.time_test():
            for i in range(10000, 20000):
                self.qf.insertElement(i)
                
    def test_deletion_2(self):
        with self.time_test():
            for i in range(10000, 20000):
                self.qf.deleteElement(i)
                
    def test_query_2(self):
        with self.time_test():
            for i in range(10000, 20000):
                self.qf.query(i)
                
if __name__ == '__main__':
    unittest.main()
