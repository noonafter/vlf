#include <stdio.h>
#include <stdlib.h>

// 交换两个float类型的值
void swap(float *a, float *b) {
    float temp = *a;
    *a = *b;
    *b = temp;
}

// 分区函数，返回分区点的索引
int partition(float arr[], int low, int high) {
    float pivot = arr[high];  // 选择最后一个元素作为基准
    int i = low - 1;  // i是小于基准的元素的最后一个索引

    for (int j = low; j < high; j++) {
        if (arr[j] < pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);  // 将基准元素放到正确的位置
    return i + 1;
}

// 快速选择算法,返回排序后数组中索引为i的元素
float quickselect(float arr[], int low, int high, int i) {
    if (low == high) {
        return arr[low];
    }

    int pivotIndex = partition(arr, low, high);

    if (i == pivotIndex) {
        return arr[pivotIndex];
    } else if (i < pivotIndex) {
        return quickselect(arr, low, pivotIndex - 1, i);
    } else {
        return quickselect(arr, pivotIndex + 1, high, i);
    }
}

// 找出中位数
float findMedian(float arr[], int n) {

    if (n % 2 == 1) {
        // 如果数组长度为奇数，直接返回中位数
        return quickselect(arr, 0, n - 1, n / 2);
    } else {
        // 如果数组长度为偶数，返回中间两个数的平均值
        float median1 = quickselect(arr, 0, n - 1, n / 2 - 1);
        float median2 = quickselect(arr, 0, n - 1, n / 2);
        return 0.5f * (median1 + median2);
    }
}
/*
int main() {
    int n, i;
    printf("please input array size: ");
    scanf("%d", &n);

    float *A = (float *)malloc(n * sizeof(float));
    printf("please input %d float:\n", n);
    for (int j = 0; j < n; j++) {
        scanf("%f", &A[j]);
    }

    printf("please input value of i (1 <= i < %d): ", n);
    scanf("%d", &i);

    if (i < 1 || i >= n) {
        printf("i is illegal\n");
        return 1;
    }

    float result = quickselect(A, 0, n - 1, i - 1);
    printf("No. %d minimum element is: %f\n", i, result);

    free(A);
    return 0;
}
*/
