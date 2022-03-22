#ifndef DEMO_ARR_HPP_INCLUDE
#define DEMO_ARR_HPP_INCLUDE

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace demo {

// demo::Iter class
template <class Pointer, class MyArr>
class Iter {
private:
    using IterTraits = std::iterator_traits<Pointer>;

public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = typename IterTraits::value_type; // T
    using pointer           = typename IterTraits::pointer;    // (const) T*
    using reference         = typename IterTraits::reference;  // (const) T&
    using difference_type   = typename MyArr::difference_type; // std::ptrdiff_t

    // make const_iterator a friend of iterator
    friend typename std::conditional<std::is_same<Iter, typename MyArr::iterator>::value,
                                     typename MyArr::const_iterator, void>::type;

    friend MyArr;

private:
    explicit Iter(pointer ptr) noexcept : ptr_(ptr) {}

public:
    // enable conversion from iterator to const_iterator
    template <class Ptr,
              class = typename std::enable_if<std::is_same<Ptr, typename MyArr::pointer>::value>::type>
    Iter(const Iter<Ptr, MyArr>& itr) noexcept
        : Iter(static_cast<pointer>(itr.ptr_)) {}

    // forward_iterator_tag
    friend bool operator==(const Iter& lhs, const Iter& rhs) noexcept { return lhs.ptr_ == rhs.ptr_; }
    friend bool operator!=(const Iter& lhs, const Iter& rhs) noexcept { return !(lhs == rhs); }

    reference operator*()  const noexcept { return *ptr_; }
    pointer   operator->() const noexcept { return ptr_; }

    Iter& operator++() noexcept {
        ++ptr_;
        return *this;
    }
    Iter operator++(int) noexcept {
        Iter temp(*this);
        ++(*this);
        return temp;
    }

    // bidirectional_iterator_tag
    Iter& operator--() noexcept {
        --ptr_;
        return *this;
    }
    Iter operator--(int) noexcept {
        Iter temp(*this);
        --(*this);
        return temp;
    }

    // random_access_iterator_tag
    reference operator[](difference_type n) const noexcept { return ptr_[n]; }

    Iter& operator+=(difference_type rhs) noexcept {
        ptr_ += rhs;
        return *this;
    }
    friend Iter operator+(Iter lhs, difference_type rhs) noexcept { return lhs += rhs; }
    friend Iter operator+(difference_type lhs, Iter rhs) noexcept { return rhs += lhs; }

    Iter& operator-=(difference_type rhs) noexcept {
        ptr_ -= rhs;
        return *this;
    }
    friend difference_type operator-(Iter lhs, Iter rhs) noexcept { return lhs.ptr_ - rhs.ptr_; }
    friend Iter operator-(Iter lhs, difference_type rhs) noexcept { return lhs -= rhs; }

    friend bool operator< (const Iter& lhs, const Iter& rhs) noexcept { return lhs.ptr_ < rhs.ptr_; }
    friend bool operator> (const Iter& lhs, const Iter& rhs) noexcept { return rhs < lhs; }
    friend bool operator<=(const Iter& lhs, const Iter& rhs) noexcept { return !(rhs < lhs); }
    friend bool operator>=(const Iter& lhs, const Iter& rhs) noexcept { return !(lhs < rhs); }

private:
    pointer ptr_;
};

namespace detail {

    template <class Allocator, class Type>
    using RebindAlloc = typename std::allocator_traits<Allocator>::template rebind_alloc<Type>;

    template <class T>
    using IterCategory = typename std::iterator_traits<T>::iterator_category;

    template <class...>
    using void_t = void;

    template <class T, class = void>
    struct is_iterator : std::false_type {};
    template <class T>
    struct is_iterator<T, void_t<IterCategory<T>>> : std::true_type {};

    template <class T>
    using is_input_iterator = std::is_convertible<IterCategory<T>, std::input_iterator_tag>;

} // namespace detail

// demo::Arr class
template <class T, class Allocator_ = std::allocator<T>>
class Arr {
private:
    // rebind to Allocator_<T> in case it's not T
    using Allocator   = detail::RebindAlloc<Allocator_, T>;
    using AllocTraits = std::allocator_traits<Allocator>;
    Allocator alloc_;

public:
    using allocator_type  = Allocator;
    using value_type      = typename AllocTraits::value_type;      // T
    using reference       = value_type&;                           // T&
    using const_reference = const value_type&;                     // const T&
    using pointer         = typename AllocTraits::pointer;         // T*
    using const_pointer   = typename AllocTraits::const_pointer;   // const T*
    using size_type       = typename AllocTraits::size_type;       // std::size_t
    using difference_type = typename AllocTraits::difference_type; // std::ptrdiff_t

    using iterator               = Iter<pointer, Arr>;
    using const_iterator         = Iter<const_pointer, Arr>;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

public:
    Arr() noexcept
        : size_(),
          arr_(nullptr) {}

    template <class VT = value_type,
              typename std::enable_if<!std::is_default_constructible<VT>::value>::type* = nullptr>
    explicit Arr(size_type size) {
        ctor_impl(size);
    }

    template <class VT = value_type,
              typename std::enable_if<std::is_default_constructible<VT>::value>::type* = nullptr>
    explicit Arr(size_type size) {
        ctor_impl(size);
        construct_range(begin(), end());
    }

    Arr(size_type size, const_reference value) {
        ctor_impl(size);
        std::fill(arr_, arr_ + size_, value);
    }
    Arr(const Arr& other) {
        ctor_impl(other.size_);
        std::copy(other.arr_, other.arr_ + size_, arr_);
    }
    Arr(Arr&& other) noexcept
        : Arr() {
        swap(*this, other);
    }
    Arr(std::initializer_list<value_type> list) {
        ctor_impl(static_cast<size_type>(list.size()));
        std::copy(list.begin(), list.end(), arr_);
    }
    template <class Iter_,
              class = typename std::enable_if<detail::is_input_iterator<Iter_>::value>::type>
    Arr(Iter_ first, Iter_ last) {
        assign_range(first, last, detail::IterCategory<Iter_> {});
    }

    ~Arr() { dtor_impl(); }

    Arr& operator=(const Arr& rhs) {
        Arr temp(rhs);
        swap(*this, temp);
        return *this;
    }
    Arr& operator=(Arr&& rhs) noexcept {
        swap(*this, rhs);
        return *this;
    }
    Arr& operator=(std::initializer_list<value_type> list) {
        if (size_ != static_cast<size_type>(list.size())) {
            realloc_to_size_empty(static_cast<size_type>(list.size()));
        }
        else destroy_range(begin(), end());
        std::copy(list.begin(), list.end(), arr_);
        return *this;
    }

    template <class Iter_,
              class = typename std::enable_if<detail::is_input_iterator<Iter_>::value>::type>
    void assign(Iter_ first, Iter_ last) {
        assign_range(first, last, detail::IterCategory<Iter_> {});
    }
    void assign(size_type n, const_reference value) {
        if (size_ != n) realloc_to_size_empty(n);
        else destroy_range(begin(), end());
        std::fill(arr_, arr_ + size_, value);
    }
    void assign(std::initializer_list<value_type> list) {
        assign_range(list.begin(), list.end(), std::random_access_iterator_tag {});
    }

    reference       operator[](size_type index)       noexcept { return arr_[index]; }
    const_reference operator[](size_type index) const noexcept { return arr_[index]; }

    bool      empty()    const noexcept { return size_ == 0; }
    size_type size()     const noexcept { return size_; }
    size_type max_size() const noexcept {
        return std::min<size_type>(AllocTraits::max_size(alloc_),
                                   std::numeric_limits<difference_type>::max());
    }

    const_reference at(size_type index) const {
        if (index >= size_) throw_out_of_range();
        return arr_[index];
    }
    reference at(size_type index) {
        return const_cast<reference>(static_cast<const Arr&>(*this).at(index));
    }

    reference       front()       noexcept { return *begin(); }
    reference       back()        noexcept { return *(--end()); }
    const_reference front() const noexcept { return *begin(); }
    const_reference back()  const noexcept { return *(--end()); }

    allocator_type get_allocator() const noexcept { return alloc_; }

    pointer       data()       noexcept { return arr_; }
    const_pointer data() const noexcept { return arr_; }

    iterator       begin()        noexcept { return iterator(&arr_[0]); }
    iterator       end()          noexcept { return iterator(&arr_[size_]); }
    const_iterator begin()  const noexcept { return const_iterator(&arr_[0]); }
    const_iterator end()    const noexcept { return const_iterator(&arr_[size_]); }
    const_iterator cbegin() const noexcept { return begin(); }
    const_iterator cend()   const noexcept { return end(); }

    reverse_iterator       rbegin()        noexcept { return reverse_iterator(end()); }
    reverse_iterator       rend()          noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin()  const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator rend()    const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    const_reverse_iterator crend()   const noexcept { return rend(); }

    friend void swap(Arr& a, Arr& b) noexcept {
        using std::swap;
        swap(a.size_, b.size_);
        swap(a.arr_, b.arr_);
    }

    friend std::ostream& operator<<(std::ostream& os, const Arr& arr) noexcept {
        os << '[';
        if (!arr.empty()) {
            for (const_iterator itr = arr.begin();;) {
                os << *itr;
                if (++itr == arr.end()) break;
                os << ", ";
            }
        }
        return os << ']';
    }

    friend bool operator==(const Arr& lhs, const Arr& rhs) noexcept {
        return lhs.size() == rhs.size() &&
               std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }
    friend bool operator!=(const Arr& lhs, const Arr& rhs) noexcept { return !(lhs == rhs); }

    friend bool operator< (const Arr& lhs, const Arr& rhs) noexcept {
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }
    friend bool operator> (const Arr& lhs, const Arr& rhs) noexcept { return rhs < lhs; }
    friend bool operator<=(const Arr& lhs, const Arr& rhs) noexcept { return !(rhs < lhs); }
    friend bool operator>=(const Arr& lhs, const Arr& rhs) noexcept { return !(lhs < rhs); }

private:
    size_type size_;
    pointer arr_;

    [[noreturn]] static void throw_out_of_range() {
        throw std::out_of_range("demo::Arr index out of range.");
    }

    void ctor_impl(size_type size) {
        arr_ = size ? AllocTraits::allocate(alloc_, size) : nullptr;
        size_ = size;
    }

    void dtor_impl() noexcept {
        destroy_range(begin(), end());
        AllocTraits::deallocate(alloc_, arr_, size_);
    }

    template <class... Args>
    void construct_range(iterator first, iterator last, Args&&... constructParams)
                         noexcept(std::is_nothrow_constructible<value_type, Args...>::value) {
        for (; first != last; ++first) {
            AllocTraits::construct(alloc_, first.ptr_, std::forward<Args>(constructParams)...);
        }
    }

    void destroy_range(iterator first, iterator last) noexcept {
        for (; first != last; ++first) {
            AllocTraits::destroy(alloc_, first.ptr_);
        }
    }

    void realloc_to_size_empty(size_type size) {
        pointer temp = AllocTraits::allocate(alloc_, size);
        dtor_impl();
        arr_ = temp;
        size_ = size;
    }

    template <class Iter_>
    void assign_range(Iter_ first, Iter_ last, std::input_iterator_tag) {
        iterator curr = begin();
        for (; first != last && curr != end(); ++first, (void)++curr) {
            *curr = *first;
        }
        destroy_range(curr, end());
        for (; first != last; ++first) {
            // emplace_back(*first);
        }
    }
    template <class Iter_>
    void assign_range(Iter_ first, Iter_ last, std::forward_iterator_tag) {
        const size_type distance = static_cast<size_type>(std::distance(first, last));
        if (size_ != distance) realloc_to_size_empty(distance);
        else destroy_range(begin(), end());
        std::copy(first, last, arr_);
    }
};

} // namespace demo

#endif // DEMO_ARR_HPP_INCLUDE