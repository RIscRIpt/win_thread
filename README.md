# win_thread
Implementation of std::thread using WinAPI.

# Example of usage

```c++
#include <iostream>
#include <win_thread/win_thread.h>

void pow(double a, double b, double &c) {
    using namespace std::chrono_literals;

    c = std::pow(a, b);

    // std::this_thread::get_id();
    std::cout << "pow thread id: " << win::this_thread::get_id() << '\n';

    // std::this_thread::yield();
    win::this_thread::yield();

    // std::this_thread::sleep_for();
    win::this_thread::sleep_for(0.5s);

    // std::this_thread::sleep_until();
    win::this_thread::sleep_until(std::chrono::system_clock::now() + 0.5s);
}

int main() {
    std::ios::sync_with_stdio(false);

    unsigned int n = win::thread::hardware_concurrency();
    std::cout << n << " concurrent threads are supported.\n";

    double result;
    win::thread t(pow, 2, 3, std::ref(result));
    std::cout << "pow thread id: " << t.get_id() << '\n';
    t.join();

    win::thread lambda_thread([result]() {
        std::cout << "2**3 == " << result << '\n';
    });
    lambda_thread.join();

    return 0;
}
```

## Output:
```
8 concurrent threads are supported.
pow thread id: <id>
pow thread id: <id>
2**3 == 8
```
