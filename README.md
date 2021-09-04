
# Sample STL Container and Iterator

## About

A simple C++11 STL-like dynamic array container with a random-access iterator. Made for learning purposes.  
Heavily inspired by MSVC and GCC's implementation of std::vector.  
  
Inserting capabilities (push_back, insert, etc.) and some of allocator-aware-container requirements are not included for the sake of simplicity.  
Therefore range constructor and range assign are incomplete and do not work.  
  
I tried to follow the best practices and make it as readable as possible.  
Hopefully it can help you understand more about the implementation of standard containers and iterators.

## Example Usage

```cpp
#include <iostream>
#include <algorithm>

#include "demo_arr.hpp"

int main() {
    //There are 2 classes in demo_arr.hpp:
    //demo::Arr and demo::Iter (which has a private constructor)
    //And some helper metafunctions in demo::detail. (is_iterator, RebindAlloc, etc.)

    demo::Arr<int> array(4);
    array.assign(array.size(), 23);

    //ostream operator<< overload
    std::cout << array << '\n';

    //iterators
    demo::Arr<int>::iterator iter = array.begin();
    iter += 2;
    demo::Arr<int>::const_iterator cIter = iter;

    //use the STL
    array = {4, 3, 2, 1};
    std::sort(array.begin(), array.end());

    //range-based for loop
    for(int i : array) {
        std::cout << i << ' ';
    }
    std::cout << '\n';

    //various other standard functions
    if(!array.empty()) {
        array.at(0) = 10;
        array[1] = 20;
        *iter = 30;
        array.back() = 40;
    }

    //move semantics
    if(++iter > cIter) {
        demo::Arr<int> array2 = std::move(array);
        std::cout << array2;
    }

    return 0;
}
```
