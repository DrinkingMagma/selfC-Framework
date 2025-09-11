/* 
 * 内存泄漏示例代码与正确代码对比
 * 左侧为错误代码，右侧为修复后的正确代码
 */

#include <iostream>
#include <cstring>

// 1. 直接泄漏对比
// 错误版本
void directLeakWrong() {
    int* data = new int[10];  // 分配内存
    // 忘记释放内存
    std::cout << "错误: 直接泄漏 - 未释放int数组" << std::endl;
}

// 正确版本
void directLeakCorrect() {
    int* data = new int[10];  // 分配内存
    delete[] data;            // 释放内存
    std::cout << "正确: 已释放int数组内存" << std::endl;
}

// 2. 丢失指针对比
// 错误版本
void lostPointerWrong() {
    char* str = new char[100];
    strcpy(str, "Hello, World!");
    
    str = new char[50];  // 原始指针被覆盖，内存泄漏
    strcpy(str, "New string");
    
    delete[] str;  // 只释放了最后分配的内存
    std::cout << "错误: 丢失指针 - 最初分配的内存未释放" << std::endl;
}

// 正确版本
void lostPointerCorrect() {
    char* str = new char[100];
    strcpy(str, "Hello, World!");
    
    delete[] str;        // 先释放原有内存
    str = new char[50];  // 再重新赋值
    strcpy(str, "New string");
    
    delete[] str;        // 释放最后分配的内存
    std::cout << "正确: 所有分配的char数组均已释放" << std::endl;
}

// 3. 类实例泄漏对比
// 错误版本的类
class LeakyClassWrong {
private:
    int* buffer;
public:
    LeakyClassWrong() {
        buffer = new int[50];  // 分配内存
    }
    // 没有析构函数释放内存
};

// 正确版本的类
class LeakyClassCorrect {
private:
    int* buffer;
public:
    LeakyClassCorrect() {
        buffer = new int[50];  // 分配内存
    }
    
    ~LeakyClassCorrect() {
        delete[] buffer;       // 析构函数中释放内存
    }
};

// 错误的类使用方式
void classLeakWrong() {
    LeakyClassWrong* obj = new LeakyClassWrong();
    // 忘记删除对象
    std::cout << "错误: 类实例泄漏 - 未删除对象且无析构函数" << std::endl;
}

// 正确的类使用方式
void classLeakCorrect() {
    LeakyClassCorrect* obj = new LeakyClassCorrect();
    delete obj;  // 删除对象，会自动调用析构函数释放内部内存
    std::cout << "正确: 类实例已删除，内部内存通过析构函数释放" << std::endl;
}

// 4. 条件性泄漏对比
// 错误版本
void conditionalLeakWrong(bool shouldLeak) {
    double* values = new double[20];
    
    if (shouldLeak) {
        // 提前返回，未释放内存
        std::cout << "错误: 条件性泄漏 - 提前返回未释放内存" << std::endl;
        return;
    }
    
    delete[] values;  // 只有在特定条件下才释放
}

// 正确版本
void conditionalLeakCorrect(bool shouldLeak) {
    double* values = new double[20];
    
    if (shouldLeak) {
        delete[] values;  // 提前返回前释放内存
        std::cout << "正确: 条件性分支中已释放内存" << std::endl;
        return;
    }
    
    delete[] values;  // 正常路径释放内存
}

int main() { 
    std::cout << "=== 演示错误代码 ===" << std::endl;
    directLeakWrong();
    lostPointerWrong();
    classLeakWrong();
    conditionalLeakWrong(true);
    
    std::cout << "\n=== 演示正确代码 ===" << std::endl;
    directLeakCorrect();
    lostPointerCorrect();
    classLeakCorrect();
    conditionalLeakCorrect(true);
    
    return 0;
}
