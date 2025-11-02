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
#define NUM_RANDOM_FEATURES 2 // sqrt(3) is ~1.7, so we use 2
// =================================================================

#define MAX_ROWS 400
#define NUM_FEATURES 3

unsigned int addr_data[MAX_ROWS][NUM_FEATURES + 1];
unsigned int addr_train_indices[MAX_ROWS];
unsigned int addr_test_indices[MAX_ROWS];
int train_size = 0, test_size = 0, total_rows = 0;

typedef struct Node {
    int predicted_class;
    int feature_index;
    double threshold;
    unsigned int left_node_addr;
    unsigned int right_node_addr;
} Node;

#define NODE_POOL_SIZE 2048
#define NODE_STRUCT_SIZE 24 // No padding in this layout
unsigned int addr_node_pool;
int next_node_idx = 0;

typedef struct {
    unsigned int root_addrs[NUM_TREES];
} RandomForest;

// Memory helpers
void put_double(unsigned int addr, double val) { unsigned int *p = (unsigned int*)&val; memory_access(addr, 'w', p[0], 'i'); memory_access(addr + 4, 'w', p[1], 'i'); }
double get_double(unsigned int addr) { unsigned int temp[2]; temp[0] = memory_access(addr, 'r', 0, 'i'); temp[1] = memory_access(addr + 4, 'r', 0, 'i'); return *(double*)temp; }
void put_int(unsigned int addr, int val) { memory_access(addr, 'w', val, 'i'); }
int get_int(unsigned int addr) { return memory_access(addr, 'r', 0, 'i'); }

// Node management
unsigned int allocate_node() {
    if (next_node_idx >= NODE_POOL_SIZE) { printf("Node pool exhausted.\n"); exit(1); }
    unsigned int node_addr = addr_node_pool + (next_node_idx * NODE_STRUCT_SIZE);
    next_node_idx++;
    return node_addr;
}
void put_node(unsigned int node_addr, Node* node) {
    put_int(node_addr, node->predicted_class);
    put_int(node_addr + 4, node->feature_index);
    put_double(node_addr + 8, node->threshold);
    put_int(node_addr + 16, node->left_node_addr);
    put_int(node_addr + 20, node->right_node_addr);
}
Node get_node(unsigned int node_addr) {
    Node node;
    node.predicted_class = get_int(node_addr);
    node.feature_index = get_int(node_addr + 4);
    node.threshold = get_double(node_addr + 8);
    node.left_node_addr = get_int(node_addr + 16);
    node.right_node_addr = get_int(node_addr + 20);
    return node;
}

double gini_impurity(int* samples, int n_samples) {
    if (n_samples == 0) return 0.0;
    int counts[2] = {0};
    for (int i = 0; i < n_samples; i++) {
        int row_idx = get_int(addr_train_indices[samples[i]]);
        counts[(int)get_double(addr_data[row_idx][NUM_FEATURES])]++;
    }
    double impurity = 1.0;
    for (int i = 0; i < 2; i++) {
        double prob = (double)counts[i] / n_samples;
        impurity -= prob * prob;
    }
    return impurity;
}

void get_random_features(int* features_to_check) {
    int all_features[NUM_FEATURES] = {0, 1, 2};
    for (int i = 0; i < NUM_FEATURES; i++) {
        int j = rand() % NUM_FEATURES;
        int temp = all_features[i]; all_features[i] = all_features[j]; all_features[j] = temp;
    }
    for(int i=0; i<NUM_RANDOM_FEATURES; ++i) features_to_check[i] = all_features[i];
}

int majority_vote(int* samples, int n_samples) {
    int counts[2] = {0};
    for (int i = 0; i < n_samples; i++) {
        int row_idx = get_int(addr_train_indices[samples[i]]);
        counts[(int)get_double(addr_data[row_idx][NUM_FEATURES])]++;
    }
    return (counts[1] > counts[0]) ? 1 : 0;
}

void find_best_split(int* samples, int n_samples, int* best_feature, double* best_threshold) {
    double best_gini = DBL_MAX;
    *best_feature = -1;
    *best_threshold = -1.0;
    int features_to_check[NUM_RANDOM_FEATURES];
    get_random_features(features_to_check);

    for (int i = 0; i < NUM_RANDOM_FEATURES; i++) {
        int feature = features_to_check[i];
        for (int j = 0; j < n_samples; j++) {
            int row_idx = get_int(addr_train_indices[samples[j]]);
            double threshold = get_double(addr_data[row_idx][feature]);
            int left_samples[MAX_ROWS], right_samples[MAX_ROWS], n_left = 0, n_right = 0;
            for (int k = 0; k < n_samples; k++) {
                int sample_row_idx = get_int(addr_train_indices[samples[k]]);
                if (get_double(addr_data[sample_row_idx][feature]) < threshold) {
                    left_samples[n_left++] = samples[k];
                } else {
                    right_samples[n_right++] = samples[k];
                }
            }
            if (n_left == 0 || n_right == 0) continue;
            double weighted_gini = ((double)n_left / n_samples) * gini_impurity(left_samples, n_left) + ((double)n_right / n_samples) * gini_impurity(right_samples, n_right);
            if (weighted_gini < best_gini) {
                best_gini = weighted_gini;
                *best_feature = feature;
                *best_threshold = threshold;
            }
        }
    }
}

unsigned int build_tree(int* samples, int n_samples, int depth) {
    if (depth >= MAX_DEPTH || n_samples < MIN_SAMPLES_SPLIT) {
        unsigned int leaf_addr = allocate_node();
        Node leaf_node = {majority_vote(samples, n_samples), -1, 0.0, 0, 0};
        put_node(leaf_addr, &leaf_node);
        return leaf_addr;
    }
    int best_feature; double best_threshold;
    find_best_split(samples, n_samples, &best_feature, &best_threshold);
    if (best_feature == -1) {
        unsigned int leaf_addr = allocate_node();
        Node leaf_node = {majority_vote(samples, n_samples), -1, 0.0, 0, 0};
        put_node(leaf_addr, &leaf_node);
        return leaf_addr;
    }
    int left_samples[MAX_ROWS], right_samples[MAX_ROWS], n_left = 0, n_right = 0;
    for (int i = 0; i < n_samples; i++) {
        int row_idx = get_int(addr_train_indices[samples[i]]);
        if (get_double(addr_data[row_idx][best_feature]) < best_threshold) {
            left_samples[n_left++] = samples[i];
        } else {
            right_samples[n_right++] = samples[i];
        }
    }
    unsigned int node_addr = allocate_node();
    Node current_node = {-1, best_feature, best_threshold, 0, 0};
    current_node.left_node_addr = build_tree(left_samples, n_left, depth + 1);
    current_node.right_node_addr = build_tree(right_samples, n_right, depth + 1);
    put_node(node_addr, &current_node);
    return node_addr;
}

void grow_forest(RandomForest* rf) {
    int* bootstrapped_samples = malloc(train_size * sizeof(int));
    for (int i = 0; i < NUM_TREES; i++) {
        for(int j = 0; j < train_size; ++j) {
            bootstrapped_samples[j] = rand() % train_size;
        }
        rf->root_addrs[i] = build_tree(bootstrapped_samples, train_size, 0);
        printf("Building tree %d...\n", i + 1);
    }
    free(bootstrapped_samples);
}

int predict_tree(unsigned int node_addr, double* features) {
    Node current_node = get_node(node_addr);
    if (current_node.predicted_class != -1) return current_node.predicted_class;
    if (features[current_node.feature_index] < current_node.threshold) {
        return predict_tree(current_node.left_node_addr, features);
    } else {
        return predict_tree(current_node.right_node_addr, features);
    }
}
int predict_forest(RandomForest* rf, double* features) {
    int votes[2] = {0};
    for (int i = 0; i < NUM_TREES; i++) {
        if(rf->root_addrs[i] != 0) { // Safety check
             votes[predict_tree(rf->root_addrs[i], features)]++;
        }
    }
    return (votes[1] > votes[0]) ? 1 : 0;
}

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
        if (prediction == true_label) correct_predictions++;
    }
    if (test_size > 0) {
        double accuracy = (double)correct_predictions / test_size * 100.0;
        printf("Accuracy on test set: %.2f%%\n", accuracy);
    } else {
        printf("Accuracy on test set: N/A (no test data)\n");
    }
}
int main() {
    unsigned int current_addr = 0;
    for (int i = 0; i < MAX_ROWS; i++) {
        for (int j = 0; j <= NUM_FEATURES; j++) { addr_data[i][j] = current_addr; current_addr += 8; }
        addr_train_indices[i] = current_addr; current_addr += 4;
        addr_test_indices[i] = current_addr; current_addr += 4;
    }
    addr_node_pool = current_addr;
    read_csv_sim();
    split_data_sim();
    printf("Growing the Random Forest (Reordered AoS Layout)...\n");
    RandomForest rf = {0};
    grow_forest(&rf);
    evaluate_performance_sim(&rf);
    termination();
    get_performance();
    return 0;
}

