#include <iostream>
#include <fstream>
#include <random>

using namespace std;

int main() {
    // Set the parameters for the dataset
    int num_items = 100000; // number of items
    int max_item_size = 50; // maximum item size in bytes
    int max_fp_rate = 5; // maximum false positive rate
    int max_filter_size = 2000000; // maximum filter size in bytes
    
    // Initialize the random number generator
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis_size(1, max_item_size);
    uniform_int_distribution<> dis_fp_rate(1, max_fp_rate);
    uniform_int_distribution<> dis_items(1, num_items);
    
    // Create the dataset
    ofstream outfile("dataset.txt");
    int item_sizes[num_items];
    for (int i = 0; i < num_items; i++) {
        item_sizes[i] = dis_size(gen);
        outfile << "item" << i << " ";
        for (int j = 0; j < item_sizes[i]; j++) {
            outfile << static_cast<char>(dis_items(gen) % 26 + 'a');
        }
        outfile << "\n";
    }
    outfile.close();
    
    // Calculate the expected number of items in the filter
    double fp_rates[num_items];
    for (int i = 0; i < num_items; i++) {
        fp_rates[i] = dis_fp_rate(gen) / 100.0;
    }
    double exp_items = 0;
    for (int i = 0; i < num_items; i++) {
        exp_items += 1 / (1 - fp_rates[i]);
    }
    exp_items /= num_items;
    
    // Calculate the filter size based on the expected number of items
    int filter_size = exp_items * max_item_size * num_items;
    if (filter_size > max_filter_size) {
        filter_size = max_filter_size;
    }
    
    // Print the dataset parameters
    cout << "Dataset parameters:\n";
    cout << "Number of items: " << num_items << "\n";
    cout << "Maximum item size: " << max_item_size << " bytes\n";
    cout << "Maximum false positive rate: " << max_fp_rate << "%\n";
    cout << "Maximum filter size: " << max_filter_size << " bytes\n\n";
    
    // Print the filter parameters
    cout << "Filter parameters:\n";
    cout << "Expected number of items in the filter: " << exp_items << "\n";
    cout << "Filter size: " << filter_size << " bytes\n\n";
    
    return 0;
}
