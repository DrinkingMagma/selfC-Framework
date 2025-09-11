// fibonacci.cpp
#include <iostream>
#include <chrono>
#include <thread>

// 计算斐波那契数列的函数
int fibonacci(int n) {
    if (n == 0) {
        return 0;
    }
    if (n == 1) {
        return 1;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 模拟一个耗时操作的函数
void doSomethingTimeConsuming() {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);
}

int main(int argc, char* argv[]) {
    if (argc != 2)
    {
        printf("Usage: %s <num>\n", argv[0]);
        return -1;
    }
    
    int num = atoi(argv[1]);
    // 调用计算斐波那契数列的函数
    int result = fibonacci(num);
    std::cout << "Fibonacci(" << num << ") = " << result << std::endl;

    // 调用模拟耗时操作的函数
    doSomethingTimeConsuming();

    return 0;
}