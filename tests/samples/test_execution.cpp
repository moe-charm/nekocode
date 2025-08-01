#include <execution>
#include <algorithm>
#include <vector>
#include <iostream>
int main() { std::vector<int> v{1,2,3}; std::for_each(std::execution::par_unseq, v.begin(), v.end(), [](int& x){x*=2;}); std::cout << "parallel execution works\n"; return 0; }
