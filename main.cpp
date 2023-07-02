#include <iostream>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <stack>
#include <tuple>
#include <algorithm>
#include <memory>
#include <thread>
#include <future>
#include <atomic>
#include <chrono>
#include <cxxabi.h>

#include "ThreadPool.h"

int main(int argc, const char *argv[]) {
    using namespace w130205;
    ThreadPool pool(4);
    auto [success1, result1] = pool.submit(
        [](int i, int j) {
            std::cout << "begin task" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            std::cout << "end task" << std::endl;
            return i + j;
        },
        3, 4);
    std::cout << "main going\n";
    std::cout << "result is " << result1.get() << std::endl;
    return 0;
}
