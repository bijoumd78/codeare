#ifndef __ALLOCATOR__
#define __ALLOCATOR__

#include <limits>
#include <exception>
#include <cstddef>
#include <new>
#include <complex>
#include <xmmintrin.h>

template<size_t alignment>
struct static_allocator {

    static void* allocate(size_t n) {

        if(n == 0)
            return 0;

        if(n > max_size())
            throw std::bad_alloc();
        void* ret =
#if defined(__GNUC__) || defined (__INTEL_COMPILER)
            _mm_malloc
#elif defined (_MSC_VER)
			_aligned_malloc
#else
            _malloc
#endif
            (n,alignment);

        if(!ret)
            throw std::bad_alloc();

        return ret;

    }

    static void deallocate (void* p) {

#if defined(__GNUC__) || defined (__INTEL_COMPILER) 
        _mm_free
#elif defined (_MSC_VER)
		_aligned_free
#else
        _free
#endif
        (p);

    }

    static size_t max_size () {
        #undef max
        return std::numeric_limits<size_t>::max();
    }

};

/// allocate and de-allocate unaligned memory
template<>
struct static_allocator<1> {

    static size_t max_size ()  {
        return std::numeric_limits<size_t>::max();
    }

    static void* allocate (size_t n) {

        if(n == 0)
            return 0;

        void* ret = new char[n];

        return ret;

    }

    static void deallocate(void*p) {
        delete[] static_cast<char*>(p);
    }

};

template<> struct static_allocator<0>;

/// allocator with explicit alignment
template<typename _Tp, size_t alignment = 16>
class Allocator {

    typedef static_allocator<alignment> static_alloc;

public:
    typedef size_t     size_type;
    typedef ptrdiff_t  difference_type;
    typedef _Tp*       pointer;
    typedef const _Tp* const_pointer;
    typedef _Tp&       reference;
    typedef const _Tp& const_reference;
    typedef _Tp        value_type;

    template <typename _Tp1>
    struct rebind {
        typedef Allocator<_Tp1, alignment> other;
    };

    Allocator() {}

    Allocator(const Allocator&) {}

    template <typename _Tp1>
    Allocator(const Allocator<_Tp1, alignment> &) {}

    ~Allocator() {}

    pointer address (reference x) const {

#if __cplusplus >= 201103L
        return std::addressof(x);
#else
        return reinterpret_cast<_Tp*>(&reinterpret_cast<char&>(x));
#endif

    }

    const_pointer address (const_reference x) const {

#if __cplusplus >= 201103L
        return std::addressof(x);
#else
        return reinterpret_cast<const _Tp*>(&reinterpret_cast<const char&>(x));
#endif

    }

    pointer allocate (size_type n, const void* = 0) {
        return static_cast<pointer>(static_alloc::allocate(n*sizeof(value_type)));
    }

    void deallocate (pointer p, size_type) {
        static_alloc::deallocate(p);
    }

    size_type max_size () const {
        return static_alloc::max_size() / sizeof (value_type);
    }

#if __cplusplus >= 201103L

    template<typename _Up, typename... _Args>
    void construct(_Up* p, _Args&&... args) {
        ::new(static_cast<void*>(p)) _Up(std::forward<_Args>(args)...);
    }

    template<typename _Up>
    void destroy(_Up* p) {
        p->~_Up();
    }

#else

    void construct (pointer p, const_reference val) {
        ::new(static_cast<void*>(p)) value_type(val);
    }

    void destroy (pointer p) {
        p->~value_type ();
    }

#endif

    bool operator!=(const Allocator&) const {
        return false;
    }

    // Returns true if and only if storage allocated from *this
    // can be deallocated from other, and vice versa.
    // Always returns true for stateless allocators.
    bool operator==(const Allocator&) const  {
        return true;
    }

};// class Allocator<>

/// Allocator<void> specialization.
template<size_t alignment>
class Allocator<void, alignment> {

public:

    typedef size_t      size_type;
    typedef ptrdiff_t   difference_type;
    typedef void*       pointer;
    typedef const void* const_pointer;
    typedef void        value_type;

    template<typename _Tp1>
    struct rebind {
        typedef Allocator<_Tp1, alignment> other;
    };

};

#endif // __ALLIGNED_ALLOCATOR__
