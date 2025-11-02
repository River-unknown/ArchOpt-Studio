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
#define NUM_TREES 10
#define MAX_DEPTH 12
#define MIN_SAMPLES_SPLIT 10
// =================================================================



#define MAX_ROWS 400
#define NUM_FEATURES 3

// --- Memory Address Allocation ---
unsigned int addr_data[MAX_ROWS][NUM_FEATURES + 1];
unsigned int addr_train_indices[MAX_ROWS];
unsigned int addr_test_indices[MAX_ROWS];
int train_size = 0;
int test_size = 0;
int total_rows = 0;

// === SOA CHANGE: No longer need a Node struct for memory layout ===
// We will define base addresses for each feature array in the pool.
#define NODE_POOL_SIZE 2048
unsigned int addr_pool_feature_index;
unsigned int addr_pool_threshold;
unsigned int addr_pool_predicted_class;
unsigned int addr_pool_left_node;
unsigned int addr_pool_right_node;
int next_node_idx = 0;


// --- Random Forest Structure ---
typedef struct {
    unsigned int root_indices[NUM_TREES]; // Now stores indices, not addresses
} RandomForest;


// =================================================================
// ==               MEMORY SIMULATOR HELPERS                      ==
// =================================================================

void put_double(unsigned int addr, double val) {
    unsigned int *p = (unsigned int*)&val;
    memory_access(addr, 'w', p[0], 'i');
    memory_access(addr + 4, 'w', p[1], 'i');
}

double get_double(unsigned int addr) {
    unsigned int temp[2];
    temp[0] = memory_access(addr, 'r', 0, 'i');
    temp[1] = memory_access(addr + 4, 'r', 0, 'i');
    return *(double*)temp;
}

void put_int(unsigned int addr, int val) {
    memory_access(addr, 'w', val, 'i');
}

int get_int(unsigned int addr) {
    return memory_access(addr, 'r', 0, 'i');
}

// === SOA CHANGE: allocate_node now returns an index, not an address ===
int allocate_node_idx() {
    if (next_node_idx >= NODE_POOL_SIZE) {
        printf("Error: Node pool exhausted.\n");
        exit(1);
    }
    return next_node_idx++;
}

// =================================================================
// ==               CORE RANDOM FOREST LOGIC (SOA Adapted)        ==
// =================================================================

double gini_impurity(int* samples, int n_samples) {
    if (n_samples == 0) return 0.0;
    int counts[2] = {0};
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

void find_best_split(int* samples, int n_samples, int* best_feature, double* best_threshold) {
    double best_gini = DBL_MAX;
    *best_feature = -1;
    *best_threshold = -1.0;
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

int majority_vote(int* samples, int n_samples) {
    int counts[2] = {0};
    for (int i = 0; i < n_samples; i++) {
        int row_idx = get_int(addr_train_indices[samples[i]]);
        int label = (int)get_double(addr_data[row_idx][NUM_FEATURES]);
        counts[label]++;
    }
    return (counts[1] > counts[0]) ? 1 : 0;
}

// === SOA CHANGE: build_tree now returns an index ===
int build_tree(int* samples, int n_samples, int depth) {
    if (depth >= MAX_DEPTH || n_samples < MIN_SAMPLES_SPLIT) {
        int leaf_idx = allocate_node_idx();
        put_int(addr_pool_predicted_class + leaf_idx * 4, majority_vote(samples, n_samples));
        put_int(addr_pool_feature_index + leaf_idx * 4, -1);
        return leaf_idx;
    }
    int best_feature;
    double best_threshold;
    find_best_split(samples, n_samples, &best_feature, &best_threshold);
    if (best_feature == -1) {
        int leaf_idx = allocate_node_idx();
        put_int(addr_pool_predicted_class + leaf_idx * 4, majority_vote(samples, n_samples));
        put_int(addr_pool_feature_index + leaf_idx * 4, -1);
        return leaf_idx;
    }
    int left_samples[MAX_ROWS], right_samples[MAX_ROWS];
    int n_left = 0, n_right = 0;
    for (int i = 0; i < n_samples; i++) {
        int row_idx = get_int(addr_train_indices[samples[i]]);
        if (get_double(addr_data[row_idx][best_feature]) < best_threshold) {
            left_samples[n_left++] = samples[i];
        } else { right_samples[n_right++] = samples[i]; }
    }
    int node_idx = allocate_node_idx();
    put_int(addr_pool_feature_index + node_idx * 4, best_feature);
    put_double(addr_pool_threshold + node_idx * 8, best_threshold);
    put_int(addr_pool_predicted_class + node_idx * 4, -1);
    
    int left_child_idx = build_tree(left_samples, n_left, depth + 1);
    put_int(addr_pool_left_node + node_idx * 4, left_child_idx);
    
    int right_child_idx = build_tree(right_samples, n_right, depth + 1);
    put_int(addr_pool_right_node + node_idx * 4, right_child_idx);

    return node_idx;
}

void grow_forest(RandomForest* rf) {
    int* sample_indices = malloc(train_size * sizeof(int));
    for(int i = 0; i < train_size; ++i) sample_indices[i] = i;
    for (int i = 0; i < NUM_TREES; i++) {
        rf->root_indices[i] = build_tree(sample_indices, train_size, 0); // === SOA CHANGE ===
        printf("Building tree %d...\n", i + 1);
    }
    free(sample_indices);
}

// === SOA CHANGE: predict_tree now takes an index ===
int predict_tree(int node_idx, double* features) {
    int predicted_class = get_int(addr_pool_predicted_class + node_idx * 4);
    if (predicted_class != -1) {
        return predicted_class;
    }
    int feature_index = get_int(addr_pool_feature_index + node_idx * 4);
    double threshold = get_double(addr_pool_threshold + node_idx * 8);

    if (features[feature_index] < threshold) {
        int left_child_idx = get_int(addr_pool_left_node + node_idx * 4);
        return predict_tree(left_child_idx, features);
    } else {
        int right_child_idx = get_int(addr_pool_right_node + node_idx * 4);
        return predict_tree(right_child_idx, features);
    }
}

int predict_forest(RandomForest* rf, double* features) {
    int votes[2] = {0};
    for (int i = 0; i < NUM_TREES; i++) {
        int prediction = predict_tree(rf->root_indices[i], features); // === SOA CHANGE ===
        if (prediction != -1) {
            votes[prediction]++;
        }
    }
    return (votes[1] > votes[0]) ? 1 : 0;
}

// =================================================================
// ==               SETUP AND EVALUATION                          ==
// =================================================================

void read_csv_sim() {
    FILE *file = fopen("D:/RP/dataset/Social_Network_Ads.csv", "r");
    if (!file) { perror("Failed to open CSV file"); exit(1); }
    char line[1024];
    fgets(line, 1024, file);
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

void evaluate_performance_sim(RandomForest* rf) {
    int correct_predictions = 0;
    double features[NUM_FEATURES];
    for (int i = 0; i < test_size; i++) {
        int idx = get_int(addr_test_indices[i]);
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

int main() {
    // 1. Initialize memory addresses
    unsigned int current_addr = 0;
    for (int i = 0; i < MAX_ROWS; i++) {
        for (int j = 0; j <= NUM_FEATURES; j++) {
            addr_data[i][j] = current_addr;
            current_addr += 8;
        }
        addr_train_indices[i] = current_addr; current_addr += 4;
        addr_test_indices[i] = current_addr; current_addr += 4;
    }
    
    // === SOA CHANGE: Allocate base addresses for our SoA pools ===
    addr_pool_feature_index = current_addr;
    current_addr += NODE_POOL_SIZE * sizeof(int);
    
    addr_pool_threshold = current_addr;
    current_addr += NODE_POOL_SIZE * sizeof(double);
    
    addr_pool_predicted_class = current_addr;
    current_addr += NODE_POOL_SIZE * sizeof(int);

    addr_pool_left_node = current_addr;
    current_addr += NODE_POOL_SIZE * sizeof(unsigned int);

    addr_pool_right_node = current_addr;
    current_addr += NODE_POOL_SIZE * sizeof(unsigned int);

    // 2. Load data
    printf("Reading and splitting data...\n");
    read_csv_sim();
    split_data_sim();

    // 3. Build the model
    printf("Growing the random forest (SoA Layout)...\n");
    RandomForest rf;
    grow_forest(&rf);

    // 4. Evaluate
    printf("Evaluating performance...\n");
    evaluate_performance_sim(&rf);

    // 5. Finalize
    termination();
    get_performance();

    return 0;
}