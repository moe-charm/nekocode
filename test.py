def simple_function():
    print("Simple function")
    return 42

class TestClass:
    def __init__(self):
        self.value = 0
    
    def method_one(self):
        print("Method one")
        if self.value > 0:
            print("Positive")
        else:
            print("Non-positive")
        return self.value
    
    def method_two(self, x):
        """Method with parameter"""
        self.value = x
        for i in range(x):
            print(f"Iteration {i}")
        return x * 2

def another_function(param1, param2=None):
    """Function with default parameter"""
    if param2 is None:
        param2 = []
    param2.append(param1)
    return param2

# Global call
simple_function()