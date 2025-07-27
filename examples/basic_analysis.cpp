/**
 * @file basic_analysis.cpp
 * @brief Example: Basic file analysis with NekoCode C++
 * 
 * This example demonstrates how to analyze a single C++ file
 * and extract basic information like lines of code, complexity, etc.
 */

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// Simple class to demonstrate analysis
class DataProcessor {
private:
    std::vector<int> data_;
    
public:
    explicit DataProcessor(const std::vector<int>& data) : data_(data) {}
    
    // Function with moderate complexity
    double calculateAverage() const {
        if (data_.empty()) {
            return 0.0;
        }
        
        double sum = 0.0;
        for (const auto& value : data_) {
            sum += value;
        }
        
        return sum / data_.size();
    }
    
    // Function with higher complexity
    std::vector<int> filterAndSort(int threshold) {
        std::vector<int> filtered;
        
        // Filter values above threshold
        for (const auto& value : data_) {
            if (value > threshold) {
                filtered.push_back(value);
            }
        }
        
        // Sort the filtered values
        std::sort(filtered.begin(), filtered.end());
        
        return filtered;
    }
    
    // Template function example
    template<typename T>
    T findMax(const std::vector<T>& values) const {
        if (values.empty()) {
            throw std::runtime_error("Empty vector");
        }
        
        T max_val = values[0];
        for (const auto& val : values) {
            if (val > max_val) {
                max_val = val;
            }
        }
        
        return max_val;
    }
};

// Macro example
#define LOG_INFO(msg) std::cout << "[INFO] " << msg << std::endl
#define SQUARE(x) ((x) * (x))

int main() {
    LOG_INFO("Starting basic analysis example");
    
    // Create sample data
    std::vector<int> numbers = {1, 5, 3, 9, 2, 8, 4, 7, 6};
    
    DataProcessor processor(numbers);
    
    // Calculate average
    double avg = processor.calculateAverage();
    LOG_INFO("Average: " << avg);
    
    // Filter and sort
    auto filtered = processor.filterAndSort(4);
    LOG_INFO("Filtered values (>4): ");
    for (const auto& val : filtered) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    
    // Use macro
    int x = 5;
    LOG_INFO("Square of " << x << " is " << SQUARE(x));
    
    return 0;
}

/*
 * Expected NekoCode Analysis Results:
 * 
 * To analyze this file with NekoCode:
 * 
 * 1. Basic Analysis:
 *    ./nekocode_ai basic_analysis.cpp
 * 
 * 2. Session Mode:
 *    ./nekocode_ai session-create examples/
 *    ./nekocode_ai session-cmd <session_id> stats
 *    ./nekocode_ai session-cmd <session_id> complexity
 *    ./nekocode_ai session-cmd <session_id> template-analysis
 *    ./nekocode_ai session-cmd <session_id> macro-analysis
 * 
 * Expected metrics:
 * - Lines of code: ~100
 * - Functions: 4
 * - Classes: 1
 * - Templates: 1
 * - Macros: 2
 * - Complexity: Moderate (due to loops and conditionals)
 */