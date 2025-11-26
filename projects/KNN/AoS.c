/*
 * KNN_AoS_analysis.c
 *
 * This is the baseline K-Nearest Neighbors implementation.
 * It uses the "Array of Structs" (AoS) data layout, where all
 * features for a single data point are stored contiguously in memory.
 *
 * This layout is expected to be EFFICIENT for KNN, because the
 * core 'calculate_distance' function needs to access all features
 * (Gender, Age, Salary) for a given data point at once.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "memory.h" // Your hardware analysis header

#define MAX_ROWS 400
#define NUM_FEATURES 3 // Gender (0/1), Age, EstimatedSalary
#define K 5 // Number of neighbors
#define MAX_LINE 100

// --- Memory Address Allocation (AoS) ---
// addr_data[row][feature]
// (Gender, Age, Salary, Purchased) are stored together for each row.
unsigned int addr_data[MAX_ROWS][NUM_FEATURES + 1]; // features + label
unsigned int addr_train_indices[MAX_ROWS]; // Stores 0 or 1
unsigned int addr_test_indices[MAX_ROWS];  // Stores 0 or 1
unsigned int addr_norm_params[4]; // min_age, max_age, min_salary, max_salary
unsigned int train_size, test_size, total_rows;

// === Memory Access Helpers (same as your other files) ===
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

// === KNN Core Logic (AoS) ===

/**
 * @brief Calculates Euclidean distance between two data points.
 * THIS IS THE KEY FUNCTION FOR AoS. It reads features contiguously.
 */
double calculate_distance_aos(unsigned int idx_a, unsigned int idx_b) {
    double sum = 0.0;
    for (int j = 0; j < NUM_FEATURES; j++) {
        // Accesses [idx_a][0], [idx_a][1], [idx_a][2]
        double val_a = get_double(addr_data[idx_a][j]);
        // Accesses [idx_b][0], [idx_b][1], [idx_b][2]
        double val_b = get_double(addr_data[idx_b][j]);
        
        double diff = val_a - val_b;
        sum += diff * diff;
        // NOTE: We are not tracking ALU ops (ADD/MUL) here,
        // as the bottleneck is memory access (compares).
    }
    return sqrt(sum); // sqrt is "free" in this analysis
}

int knn_predict(unsigned int input_idx) {
    typedef struct {
        unsigned int index;
        double distance;
    } Neighbor;
    
    Neighbor neighbors[K];
    for (int i = 0; i < K; i++) {
        neighbors[i].distance = DBL_MAX;
        neighbors[i].index = -1;
    }

    // Find K nearest neighbors
    for (int i = 0; i < total_rows; i++) {
        if (!get_int(addr_train_indices[i])) continue; // If not in training set
        
        double dist = calculate_distance_aos(input_idx, i);
        
        // This loop simulates the hardware 'compares' function
        for (int n = 0; n < K; n++) {
            compares((int)(dist * 1000), (int)(neighbors[n].distance * 1000));
            // flag_reg=4U means (dist < neighbors[n].distance)
            if (flag_reg == 4U) { 
                for (int m = K-1; m > n; m--) {
                    neighbors[m] = neighbors[m-1];
                }
                neighbors[n].index = i;
                neighbors[n].distance = dist;
                break;
            }
        }
    }

    // Count votes
    int votes[2] = {0};
    for (int i = 0; i < K; i++) {
        if (neighbors[i].index == -1) continue;
        int label = (int)get_double(addr_data[neighbors[i].index][NUM_FEATURES]);
        votes[label]++;
    }

    return (votes[1] > votes[0]) ? 1 : 0;
}

void evaluate_performance() {
    int correct = 0;
    for (int i = 0; i < total_rows; i++) {
        if (!get_int(addr_test_indices[i])) continue; // If not in test set
        
        int true_label = (int)get_double(addr_data[i][NUM_FEATURES]);
        int predicted_label = knn_predict(i);
        
        if (predicted_label == true_label) {
            correct++;
        }
    }

    double accuracy = (double)correct / test_size * 100;
    printf("\nPerformance Evaluation (AoS):\n");
    printf("Accuracy: %.2f%% (%d / %d)\n", accuracy, correct, test_size);
}

// === Setup and Main Function ===

void initialization() {
    unsigned int loc = 0;
    
    // Initialize data storage (AoS layout)
    for (int i = 0; i < MAX_ROWS; i++) {
        for (int j = 0; j <= NUM_FEATURES; j++) {
            addr_data[i][j] = loc;
            loc += 8; // Size of double
        }
    }
    
    // Initialize train/test indices
    for (int i = 0; i < MAX_ROWS; i++) {
        addr_train_indices[i] = loc;
        loc += 4; // Size of int
        addr_test_indices[i] = loc;
        loc += 4; // Size of int
    }
    
    // Initialize normalization parameters
    for (int i = 0; i < 4; i++) {
        addr_norm_params[i] = loc;
        loc += 8;
    }
}

void read_csv() {
    FILE *file = fopen("D:/RP/dataset/Social_Network_Ads.csv", "r");
    if (!file) { perror("CSV open"); exit(1); }
    char line[MAX_LINE];
    fgets(line, sizeof(line), file); // Skip header
    
    int row = 0;
    while (fgets(line, sizeof(line), file) && row < MAX_ROWS) {
        char gender[10];
        double age, salary, purchased;
        sscanf(line, "%*[^,],%[^,],%lf,%lf,%lf", gender, &age, &salary, &purchased);
        
        double gender_val = (strcmp(gender, "Male") == 0) ? 1.0 : 0.0;
        
        // Store in AoS layout
        put_double(addr_data[row][0], gender_val);
        put_double(addr_data[row][1], age);
        put_double(addr_data[row][2], salary);
        put_double(addr_data[row][3], purchased);
        
        row++;
    }
    total_rows = row;
    fclose(file);
    printf("Loaded %d records.\n", total_rows);
}

void split_data() {
    train_size = test_size = 0;
    srand(42); // Use a fixed seed for reproducible splits
    for (int i = 0; i < total_rows; i++) {
        if ((double)rand() / RAND_MAX < 0.8) {
            put_int(addr_train_indices[i], 1);
            put_int(addr_test_indices[i], 0);
            train_size++;
        } else {
            put_int(addr_train_indices[i], 0);
            put_int(addr_test_indices[i], 1);
            test_size++;
        }
    }
    printf("Split into %d training and %d test examples.\n", train_size, test_size);
}

void normalize_data() {
    double min_age = DBL_MAX, max_age = -DBL_MAX;
    double min_salary = DBL_MAX, max_salary = -DBL_MAX;

    // Find min/max from training set
    for (int i = 0; i < total_rows; i++) {
        if (!get_int(addr_train_indices[i])) continue;
        double age = get_double(addr_data[i][1]);
        double salary = get_double(addr_data[i][2]);
        if (age < min_age) min_age = age;
        if (age > max_age) max_age = age;
        if (salary < min_salary) min_salary = salary;
        if (salary > max_salary) max_salary = salary;
    }
    put_double(addr_norm_params[0], min_age);
    put_double(addr_norm_params[1], max_age);
    put_double(addr_norm_params[2], min_salary);
    put_double(addr_norm_params[3], max_salary);

    // Apply normalization to all data
    for (int i = 0; i < total_rows; i++) {
        double age = get_double(addr_data[i][1]);
        double norm_age = (age - min_age) / (max_age - min_age);
        put_double(addr_data[i][1], norm_age);
        
        double salary = get_double(addr_data[i][2]);
        double norm_salary = (salary - min_salary) / (max_salary - min_salary);
        put_double(addr_data[i][2], norm_salary);
    }
    printf("Data normalized.\n");
}

int main() {
    printf("Starting KNN Analysis (AoS Layout)...\n");
    initialization(); // Your hardware init
    read_csv();
    split_data();
    normalize_data();
    
    printf("Running performance evaluation...\n");
    evaluate_performance();
    
    termination(); // Your hardware termination
    get_performance(); // Your hardware report
    return 0;
}