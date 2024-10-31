
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>

double double_abs(double x) {
    return x < 0 ? -x : x;
}

double two_shares_fn(double x, double x0, double y0, double x1, double y1) {
    return y0 * (x - x1) / (x0 - x1)
        +  y1 * (x - x0) / (x1 - x0);
}

void two_shares() {
    double b_original = 10;
    double x0 = 5, y0 = 5;
    double x1 = 6, y1 = 4;

    double b = two_shares_fn(0, x0, y0, x1, y1);
    assert(double_abs(b_original - b) < 1e-6);
}

double three_shares_fn(double x, double x0, double y0, double x1, double y1, double x2, double y2) {
    return y0 * (x - x1) * (x - x2) / ((x0 - x1) * (x0 - x2))
        +  y1 * (x - x0) * (x - x2) / ((x1 - x0) * (x1 - x2))
        +  y2 * (x - x0) * (x - x1) / ((x2 - x0) * (x2 - x1));
}

void three_shares() {
    double c_original = 14;
    // y = x^2 - 6x + 14
    double x0 = 1, y0 = 9;
    double x1 = 3, y1 = 5;
    double x2 = 4, y2 = 6;

    double c = three_shares_fn(0, x0, y0, x1, y1, x2, y2);
    assert(double_abs(c_original - c) < 1e-6);
}

typedef struct {
    double x, y;
} Point;

double n_shares_fn(double x, size_t n, Point *points) {
    double result = 0;
    for (size_t i = 0; i < n; i++) {
        double x_i = points[i].x;
        double y_i = points[i].y;
        double numerator_i = 1;
        double denominator_i = 1;
        for (size_t j = 0; j < n; j++) {
            if (i == j) continue;
            double x_j = points[j].x;
            numerator_i *= (x - x_j);
            denominator_i *= (x_i - x_j);
        }
        result += y_i * numerator_i / denominator_i;
    }
    return result;
}

void four_shares() {
    double d_original = -4;
    // y = x^3 - 7x^2 + 12x - 4
    Point points[4] = {
        {1, 2},
        {3, -4},
        {5, 6},
        {-1, -24}
    };

    double d = n_shares_fn(0, 4, (Point *)points);
    assert(double_abs(d_original - d) < 1e-6);
}


int rand_num() {
    int urandom_fd = open("/dev/urandom", O_RDONLY);
    if (urandom_fd < 0) {
        perror("Failed to open /dev/urandom");
        exit(1);
    }
    unsigned int random_number;
    ssize_t result = read(urandom_fd, &random_number, sizeof(random_number));
    if (result < 0) {
        perror("Failed to read from /dev/urandom");
        close(urandom_fd);
        exit(1);
    }
    close(urandom_fd);
    return random_number % 100;
}


double rand_n_degree_fn(double x, size_t n, double *coefficients) {
    double result = 0;
    for (size_t i = 0; i < n; i++) {
        result += coefficients[i] * pow(x, n - i - 1);
    }
    return result;
}

void rand_shares() {
    size_t n = rand_num() % 10 + 5;
    double *coefficients = malloc(n * sizeof(double));
    for (size_t i = 0; i < n; i++) {
        coefficients[i] = (double)rand_num();
    }
    Point *points = malloc(n * sizeof(Point));
    // Make sure you never divide by 0 in n_shares_fn
    for (size_t i = 0; i < n; i++) {
        points[i].x = i + 2;
        points[i].y = rand_n_degree_fn(points[i].x, n, coefficients);
    }

    double y_original = coefficients[n - 1];
    double y = n_shares_fn(0, n, points);
    assert(double_abs(y_original - y) < 1e-6);
    free(coefficients);
    free(points);
}

int main() {
    // 2 shares (2 points on the line)
    two_shares();

    // 3 shares (3 points on the parabola)
    three_shares();

    // 4 shares (4 points on the cubic curve)
    four_shares();

    // n shares (n points on the n-degree curve)
    rand_shares();

    return 0;
}

