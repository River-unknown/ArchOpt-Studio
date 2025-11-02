#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <float.h>
#include "memory.h"

// =================================================================
// ==               TUNABLE PARAMETERS                            ==
// =================================================================
#define NUM_TREES 10          // Number of trees in the forest
#define MAX_DEPTH 12          // Maximum depth of each tree
#define MIN_SAMPLES_SPLIT 10  // Minimum number of samples required to split a node
// =================================================================

#define NODE_STRUCT_SIZE 24

#define MAX_ROWS 400
#define NUM_FEATURES 3 // Gender, Age, EstimatedSalary from Social_Network_Ads.csv

// --- Memory Address Allocation ---
// We need to pre-define memory locations for all our data.
unsigned int addr_data[MAX_ROWS][NUM_FEATURES + 1];
unsigned int addr_train_indices[MAX_ROWS];
unsigned int addr_test_indices[MAX_ROWS];
int train_size = 0;
int test_size = 0;
int total_rows = 0;

// --- Decision Tree Node Structure ---
// This struct defines the logical structure of a tree node.
typedef struct Node {
    int predicted_class;
    int feature_index;
    double threshold;
    unsigned int left_node_addr;  // Memory address of the left child
    unsigned int right_node_addr; // Memory address of the right child
} Node;

// --- Node Memory Pool ---
// Since we can't use malloc with the simulator, we pre-allocate a pool of memory for our nodes.
#define NODE_POOL_SIZE 2048 // Max number of nodes for all trees
Node node_pool[NODE_POOL_SIZE];
unsigned int addr_node_pool;
int next_node_idx = 0;

// --- Random Forest Structure ---
// Stores the memory addresses of the root nodes for each tree.
typedef struct {
    unsigned int root_addrs[NUM_TREES];
} RandomForest;


// =================================================================
// ==               MEMORY SIMULATOR HELPERS                      ==
// =================================================================

/**
 * @brief Writes a double to a specified memory address.
 */
void put_double(unsigned int addr, double val) {
    unsigned int *p = (unsigned int*)&val;
    memory_access(addr, 'w', p[0], 'i');
    memory_access(addr + 4, 'w', p[1], 'i');
}

/**
 * @brief Reads a double from a specified memory address.
 */
double get_double(unsigned int addr) {
    unsigned int temp[2];
    temp[0] = memory_access(addr, 'r', 0, 'i');
    temp[1] = memory_access(addr + 4, 'r', 0, 'i');
    return *(double*)temp;
}

/**
 * @brief Writes an integer to a specified memory address.
 */
void put_int(unsigned int addr, int val) {
    memory_access(addr, 'w', val, 'i');
}

/**
 * @brief Reads an integer from a specified memory address.
 */
int get_int(unsigned int addr) {
    return memory_access(addr, 'r', 0, 'i');
}

/**
 * @brief Writes a Node struct to the simulated memory pool. (UPDATED OFFSETS)
 */
void put_node(unsigned int node_addr, Node* node) {
    unsigned int base_addr = node_addr;
    put_int(base_addr, node->predicted_class);
    put_int(base_addr + 4, node->feature_index);
    put_double(base_addr + 8, node->threshold);
    put_int(base_addr + 16, node->left_node_addr);
    put_int(base_addr + 20, node->right_node_addr);
}

/**
 * @brief Reads a Node struct from the simulated memory pool. (UPDATED OFFSETS)
 */
Node get_node(unsigned int node_addr) {
    Node node;
    unsigned int base_addr = node_addr;
    node.predicted_class = get_int(base_addr);
    node.feature_index = get_int(base_addr + 4);
    node.threshold = get_double(base_addr + 8);
    node.left_node_addr = get_int(base_addr + 16);
    node.right_node_addr = get_int(base_addr + 20);
    return node;
}

/**
 * @brief Allocates a new node from the pool and returns its memory address.
 */
unsigned int allocate_node() {
    if (next_node_idx >= NODE_POOL_SIZE) {
        printf("Error: Node pool exhausted.\n");
        exit(1);
    }
    unsigned int node_addr = addr_node_pool + (next_node_idx * NODE_STRUCT_SIZE);
    next_node_idx++;
    return node_addr;
}

// =================================================================
// ==               CORE RANDOM FOREST LOGIC                      ==
// =================================================================

/**
 * @brief Calculates the Gini impurity for a set of samples.
 */
double gini_impurity(int* samples, int n_samples) {
    if (n_samples == 0) return 0.0;

    int counts[2] = {0}; // Counts for class 0 and 1
    for (int i = 0; i < n_samples; i++) {
        int row_idx = get_int(addr_train_indices[samples[i]]);
        int label = (int)get_double(addr_data[row_idx][NUM_FEATURES]);
        counts[label]++;
    }

    double impurity = 1.0;
    for (int i = 0; i < 2; i++) {
        double prob = (double)counts[i] / n_samples;
        impurity -= prob * prob;
    }
    return impurity;
}

/**
 * @brief Finds the best feature and threshold to split a set of samples.
 */
void find_best_split(int* samples, int n_samples, int* best_feature, double* best_threshold) {
    double best_gini = DBL_MAX;
    *best_feature = -1;
    *best_threshold = -1.0;

    double current_gini = gini_impurity(samples, n_samples);

    for (int feature = 0; feature < NUM_FEATURES; feature++) {
        for (int i = 0; i < n_samples; i++) {
            int row_idx = get_int(addr_train_indices[samples[i]]);
            double threshold = get_double(addr_data[row_idx][feature]);

            int left_samples[MAX_ROWS], right_samples[MAX_ROWS];
            int n_left = 0, n_right = 0;

            for (int j = 0; j < n_samples; j++) {
                int sample_row_idx = get_int(addr_train_indices[samples[j]]);
                if (get_double(addr_data[sample_row_idx][feature]) < threshold) {
                    left_samples[n_left++] = samples[j];
                } else {
                    right_samples[n_right++] = samples[j];
                }
            }

            if (n_left == 0 || n_right == 0) continue;

            double gini_left = gini_impurity(left_samples, n_left);
            double gini_right = gini_impurity(right_samples, n_right);
            double weighted_gini = ((double)n_left / n_samples) * gini_left + ((double)n_right / n_samples) * gini_right;

            if (weighted_gini < best_gini) {
                best_gini = weighted_gini;
                *best_feature = feature;
                *best_threshold = threshold;
            }
        }
    }
}

/**
 * @brief Determines the most common class in a set of samples (for leaf nodes).
 */
int majority_vote(int* samples, int n_samples) {
    int counts[2] = {0};
    for (int i = 0; i < n_samples; i++) {
        int row_idx = get_int(addr_train_indices[samples[i]]);
        int label = (int)get_double(addr_data[row_idx][NUM_FEATURES]);
        counts[label]++;
    }
    return (counts[1] > counts[0]) ? 1 : 0;
}

/**
 * @brief Recursively builds a decision tree.
 */
unsigned int build_tree(int* samples, int n_samples, int depth) {
    if (depth >= MAX_DEPTH || n_samples < MIN_SAMPLES_SPLIT) {
        unsigned int leaf_addr = allocate_node();
        // BUG FIX: Initializer order must match the reordered struct definition
        Node leaf_node = { majority_vote(samples, n_samples), -1, 0.0, 0, 0 };
        put_node(leaf_addr, &leaf_node);
        return leaf_addr;
    }

    int best_feature;
    double best_threshold;
    find_best_split(samples, n_samples, &best_feature, &best_threshold);

    if (best_feature == -1) {
        unsigned int leaf_addr = allocate_node();
        // BUG FIX: Initializer order must match the reordered struct definition
        Node leaf_node = { majority_vote(samples, n_samples), -1, 0.0, 0, 0 };
        put_node(leaf_addr, &leaf_node);
        return leaf_addr;
    }

    int left_samples[MAX_ROWS], right_samples[MAX_ROWS];
    int n_left = 0, n_right = 0;
    for (int i = 0; i < n_samples; i++) {
        int row_idx = get_int(addr_train_indices[samples[i]]);
        if (get_double(addr_data[row_idx][best_feature]) < best_threshold) {
            left_samples[n_left++] = samples[i];
        } else {
            right_samples[n_right++] = samples[i];
        }
    }

    unsigned int node_addr = allocate_node();
    // BUG FIX: Initializer order must match the reordered struct definition
    // {predicted_class, feature_index, threshold, left, right}
    Node current_node = { -1, best_feature, best_threshold, 0, 0 };
    
    current_node.left_node_addr = build_tree(left_samples, n_left, depth + 1);
    current_node.right_node_addr = build_tree(right_samples, n_right, depth + 1);

    put_node(node_addr, &current_node);
    return node_addr;
}


/**
 * @brief Builds all the trees in the random forest.
 */
void grow_forest(RandomForest* rf) {
    int* sample_indices = malloc(train_size * sizeof(int));
    for(int i = 0; i < train_size; ++i) sample_indices[i] = i;

    for (int i = 0; i < NUM_TREES; i++) {
        // NOTE: A true Random Forest uses bootstrap sampling here.
        // For simplicity and performance predictability, we use the whole training set.
        rf->root_addrs[i] = build_tree(sample_indices, train_size, 0);
        printf("Building tree %d...\n", i + 1);
    }
    free(sample_indices);
}

/**
 * @brief Predicts the class for a given set of features using a single decision tree.
 */
int predict_tree(unsigned int node_addr, double* features) {
    Node current_node = get_node(node_addr);
    
    if (current_node.predicted_class != -1) { // Is it a leaf node?
        return current_node.predicted_class;
    }

    if (features[current_node.feature_index] < current_node.threshold) {
        return predict_tree(current_node.left_node_addr, features);
    } else {
        return predict_tree(current_node.right_node_addr, features);
    }
}

/**
 * @brief Predicts the class by majority vote from all trees in the forest.
 */
int predict_forest(RandomForest* rf, double* features) {
    int votes[2] = {0};
    for (int i = 0; i < NUM_TREES; i++) {
        int prediction = predict_tree(rf->root_addrs[i], features);
        if (prediction != -1) {
            votes[prediction]++;
        }
    }
    return (votes[1] > votes[0]) ? 1 : 0;
}


// =================================================================
// ==               SETUP AND EVALUATION                          ==
// =================================================================

/**
 * @brief Reads data from CSV and stores it in simulated memory.
 */
void read_csv_sim() {
    FILE *file = fopen("D:/RP/dataset/Social_Network_Ads.csv", "r");
    if (!file) { perror("Failed to open CSV file"); exit(1); }
    char line[1024];
    fgets(line, 1024, file); // Skip header

    int row = 0;
    while (fgets(line, 1024, file) && row < MAX_ROWS) {
        char gender_str[10];
        double age, salary, purchased;
        sscanf(line, "%*[^,],%[^,],%lf,%lf,%lf", gender_str, &age, &salary, &purchased);
        
        put_double(addr_data[row][0], (strcmp(gender_str, "Male") == 0) ? 1.0 : 0.0);
        put_double(addr_data[row][1], age);
        put_double(addr_data[row][2], salary);
        put_double(addr_data[row][3], purchased);
        row++;
    }
    total_rows = row;
    fclose(file);
}

/**
 * @brief Splits data indices and stores them in simulated memory.
 */
void split_data_sim() {
    srand(time(NULL));
    for (int i = 0; i < total_rows; i++) {
        if ((double)rand() / RAND_MAX < 0.8) {
            put_int(addr_train_indices[train_size++], i);
        } else {
            put_int(addr_test_indices[test_size++], i);
        }
    }
}

/**
 * @brief Evaluates the forest's performance on the test set.
 */
void evaluate_performance_sim(RandomForest* rf) {
    int correct_predictions = 0;
    double features[NUM_FEATURES];

    for (int i = 0; i < test_size; i++) {
        int idx = get_int(addr_test_indices[i]);
        
        // Read features for one data point
        for (int f = 0; f < NUM_FEATURES; f++) {
            features[f] = get_double(addr_data[idx][f]);
        }
        
        int prediction = predict_forest(rf, features);
        int true_label = (int)get_double(addr_data[idx][NUM_FEATURES]);

        if (prediction == true_label) {
            correct_predictions++;
        }
    }
    double accuracy = (double)correct_predictions / test_size * 100.0;
    printf("Accuracy on test set: %.2f%%\n", accuracy);
}

/**
 * @brief Main function to run the simulation.
 */
int main() {
    // 1. Initialize the memory simulator and our address pointer
    unsigned int current_addr = 0;
    for (int i = 0; i < MAX_ROWS; i++) {
        for (int j = 0; j <= NUM_FEATURES; j++) {
            addr_data[i][j] = current_addr;
            current_addr += 8; // Size of a double
        }
        addr_train_indices[i] = current_addr;
        current_addr += 4; // Size of an int
        addr_test_indices[i] = current_addr;
        current_addr += 4; // Size of an int
    }
    addr_node_pool = current_addr;

    // 2. Load data into simulated memory
    printf("Reading and splitting data...\n");
    read_csv_sim();
    split_data_sim();

    // 3. Build the Random Forest model
    printf("Growing the random forest...\n");
    RandomForest rf;
    grow_forest(&rf);

    // 4. Evaluate the model's performance
    printf("Evaluating performance...\n");
    evaluate_performance_sim(&rf);

    // 5. Finalize simulation and print performance stats
    termination();
    get_performance();

    return 0;
}