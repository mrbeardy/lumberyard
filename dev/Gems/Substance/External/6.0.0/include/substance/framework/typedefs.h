//! @file typedefs.h
//! @author Antoine Gonzalez - Allegorithmic
//! @copyright Allegorithmic. All rights reserved.

#ifndef _SUBSTANCE_AIR_FRAMEWORK_TYPEDEFS_H
#define _SUBSTANCE_AIR_FRAMEWORK_TYPEDEFS_H

#include <assert.h>
#include <cstddef>

// Detect Modern C++ Features
#if (__cplusplus >= 201103L) || (_MSC_VER >= 1700)	//MSVC 2012
#	define AIR_NULLPTR   std::nullptr_t
#endif

#if (__cplusplus >= 201103L) || (_MSC_VER >= 1800)	//MSVC 2013
#	define AIR_EXPLICIT  explicit
#	define AIR_ALIAS_TEMPLATES
#endif

#if (__cplusplus >= 201103L) || (_MSC_VER >= 1900)	//MSVC 2015
#	define AIR_CONSTEXPR constexpr
#	define AIR_TYPENAME typename
#	define AIR_CPP_MODERN_MEMORY
#endif

// Fallbacks for C++98 and older
#if !defined(AIR_CONSTEXPR)
#	define AIR_CONSTEXPR
#endif

#if !defined(AIR_EXPLICIT)
#	define AIR_EXPLICIT
#endif

#if !defined(AIR_NULLPTR)
#	define AIR_NULLPTR	const void*
#endif

#if !defined(AIR_TYPENAME)
#	define AIR_TYPENAME
#endif

// Detect TR1 support
#if defined(AIR_CPP_MODERN_MEMORY) || defined(__EMSCRIPTEN__) || defined(__GXX_EXPERIMENTAL_CXX0X__)
#	include <memory>               // C++0x support
#	define SBS_TR1_NAMESPACE std
#elif defined (__GNUC__) && __GNUC__ >= 4 && defined (__GLIBCXX__)
#	include <tr1/memory>           // GCC 4.0 TR1
#	define SBS_TR1_NAMESPACE std::tr1
#elif defined (_MSC_VER) && _MSC_FULL_VER>=150030729
#	include <memory>               // VS 2008 SP1 and above TR1
#	define SBS_TR1_NAMESPACE std::tr1
#else
#	include <boost/tr1/memory.hpp> // Boost fallback
#	define SBS_TR1_NAMESPACE std::tr1
#endif

#include <deque>
#include <map>
#include <new>
#include <string>
#include <sstream>
#include <vector>

#include <stdlib.h>

#ifdef __linux__
#	include <string.h>
#endif

#include "memory.h"

namespace SubstanceAir
{

//! @brief basic types used in the integration framework
typedef unsigned int UInt;
typedef unsigned long long UInt64;

// Shared Pointer namespace
namespace Tr1 = SBS_TR1_NAMESPACE;

//! @brief shared_ptr implementation
template<typename T> class shared_ptr : private Tr1::shared_ptr<T>
{
public:
	AIR_CONSTEXPR shared_ptr()
		: Tr1::shared_ptr<T>()
	{
	}
	AIR_CONSTEXPR shared_ptr(AIR_NULLPTR null)
		: Tr1::shared_ptr<T>(null)
	{
	}
	shared_ptr(T* ptr)
	{
		reset(ptr);
	}
	template<typename Y> shared_ptr(Y* ptr)
	{
		reset<Y>(ptr);
	}
	inline T* operator->() const
	{
		return get();
	}
	inline T* get() const
	{
		return Tr1::shared_ptr<T>::get();
	}
	inline void reset()
	{
		Tr1::shared_ptr<T>::reset();
	}
	inline void reset(T* ptr)
	{
		Tr1::shared_ptr<T>::reset(ptr, deleter<T>());
	}
	template<class Y> inline void reset(Y* ptr)
	{
		Tr1::shared_ptr<T>::reset(ptr, deleter<Y>());
	}
	AIR_EXPLICIT inline operator bool() const
	{
		return (get() != NULL) ? true : false;
	}
};

template < class T, class U > bool operator==(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs)
{
	return lhs.get() == rhs.get();
}
template< class T > bool operator==(const shared_ptr<T>& lhs, AIR_NULLPTR rhs)
{
	return lhs.get() == rhs;
}
template< class T > bool operator==(AIR_NULLPTR lhs, const shared_ptr<T>& rhs)
{
	return lhs == rhs.get();
}
template< class T, class U > bool operator!=(const shared_ptr<T>& lhs, const shared_ptr<U>& rhs)
{
	return lhs.get() != rhs.get();
}
template< class T > bool operator!=(const shared_ptr<T>& lhs, AIR_NULLPTR rhs)
{
	return lhs.get() != rhs;
}
template< class T > bool operator!=(AIR_NULLPTR lhs, const shared_ptr<T>& rhs)
{
	return lhs != rhs.get();
}

#if defined(AIR_ALIAS_TEMPLATES)
template<typename T> using unique_ptr = std::unique_ptr<T, deleter<T> >;
#else
template<typename T> class unique_ptr
{
public:
	typedef T* pointer;

public:
	unique_ptr() : mPtr(NULL) {}
	template<typename U> inline unique_ptr(unique_ptr<U>& p) : mPtr(NULL)
	{
		swap(p);
	}
	inline explicit unique_ptr(typename unique_ptr::pointer p) : mPtr(p) {}
	inline ~unique_ptr()
	{
		reset();
	}

	inline typename unique_ptr::pointer get() const
	{
		return mPtr;
	}

	inline typename unique_ptr::pointer release()
	{
		pointer returnPtr = mPtr;
		mPtr = NULL;
		return returnPtr;
	}

	inline T* operator->() const
	{
		return mPtr;
	}

	inline operator bool() const
	{
		return (get() != NULL) ? true : false;
	}

	inline void reset(pointer p = pointer())
	{
		if (mPtr != p)
		{
			if (mPtr)
			{
				SubstanceAir::deleter<T> del;
				del(mPtr);
			}

			mPtr = p;
		}
	}

	inline void swap(unique_ptr& u)
	{
		std::swap(mPtr, u.mPtr);
	}

private:
	/** do not implement copy constructor or assignment */
	template<typename U> unique_ptr(const unique_ptr<U>& p);
	template<typename U> inline unique_ptr& operator=(unique_ptr<U>& u);

private:
	pointer		mPtr;
};

template<class T, class U> bool operator==(const unique_ptr<T>& x, const unique_ptr<U>& y)
{
	return x.get() == y.get();
}

template<class T, class U> bool operator!=(const unique_ptr<T>& x, const unique_ptr<U>& y)
{
	return x.get() != y.get();
}

template<class T, class U> bool operator<(const unique_ptr<T>& x, const unique_ptr<U>& y)
{
	return x.get() < y.get();
}

template<class T, class U> bool operator<=(const unique_ptr<T>& x, const unique_ptr<U>& y)
{
	return x.get() <= y.get();
}

template<class T, class U> bool operator>(const unique_ptr<T>& x, const unique_ptr<U>& y)
{
	return x.get() > y.get();
}

template<class T, class U> bool operator>=(const unique_ptr<T>& x, const unique_ptr<U>& y)
{
	return x.get() >= y.get();
}


#endif

//! @brief STL aliases mapped to substance air allocators
#if defined(AIR_ALIAS_TEMPLATES)
template<typename T>             using deque  = std::deque<T, aligned_allocator<T, AIR_DEFAULT_ALIGNMENT> >;
template<typename K, typename V> using map    = std::map<K, V, std::less<K>, aligned_allocator<std::pair<const K, V>, AIR_DEFAULT_ALIGNMENT> >;
template<typename T>             using vector = std::vector<T, aligned_allocator<T, AIR_DEFAULT_ALIGNMENT> >;
#else
template<typename T> class deque : public std::deque<T, aligned_allocator<T, AIR_DEFAULT_ALIGNMENT> > {};
template<typename K, typename V> class map : public std::map<K, V, std::less<K>, aligned_allocator<std::pair<const K, V>, AIR_DEFAULT_ALIGNMENT> > {};

template<typename T> class vector : public std::vector<T, aligned_allocator<T, AIR_DEFAULT_ALIGNMENT> >
{
public:
	explicit vector() : std::vector<T, aligned_allocator<T, AIR_DEFAULT_ALIGNMENT> >()
	{
	}
	template< class InputIt >
	vector(InputIt first, InputIt last)
		: std::vector<T, aligned_allocator<T, AIR_DEFAULT_ALIGNMENT> >(first, last)
	{
	}
};
#endif

typedef std::basic_string<char, std::char_traits<char>, aligned_allocator<char, AIR_DEFAULT_ALIGNMENT> >
string;
typedef std::basic_stringstream<char, std::char_traits<char>, aligned_allocator<char, AIR_DEFAULT_ALIGNMENT> >
stringstream;

static inline string to_string(std::string& str)
{
	return string(str.c_str());
}

static inline string to_string(const std::string& str)
{
	return string(str.c_str());
}

static inline string to_string(const char* str)
{
	return string(str);
}

static inline std::string to_std_string(string& str)
{
	return std::string(str.c_str());
}

static inline std::string to_std_string(const string& str)
{
	return std::string(str.c_str());
}

//! @brief object that automatically deep copies memory assigned to it,
//		and frees the memory when it is destroyed
template<typename T, class Allocator = aligned_allocator<T, AIR_DEFAULT_ALIGNMENT> >
class unique_memory
{
public:
	unique_memory()
		: mData(NULL)
		, mElements(0)
	{
	}
	~unique_memory()
	{
		clear();
	}

	inline void assign(T* data, size_t elements)
	{
		clear();

		mData = mAllocator.allocate(elements);
		mElements = elements;

		memcpy(mData, data, elements * sizeof(T));
	}

	inline void clear()
	{
		if (mData != NULL)
		{
			mAllocator.deallocate(mData, mElements);
		}

		mData = NULL;
		mElements = 0;
	}

	inline size_t count() const
	{
		return mElements;
	}

	inline T* data() const
	{
		return mData;
	}

	inline bool empty() const
	{
		return (mData == NULL) ? true : false;
	}

	inline size_t size() const
	{
		return mElements * sizeof(T);
	}

private:
	T*				mData;
	size_t			mElements;
	Allocator		mAllocator;
};

}  // namespace SubstanceAir

#include "vector.h"

namespace SubstanceAir
{
//! @brief containers used in the integration framework
typedef vector<unsigned char> BinaryData;
typedef vector<UInt> Uids;

// Vectors
typedef Math::Vector2<float> Vec2Float;
typedef Math::Vector3<float> Vec3Float;
typedef Math::Vector4<float> Vec4Float;
typedef Math::Vector2<int> Vec2Int;
typedef Math::Vector3<int> Vec3Int;
typedef Math::Vector4<int> Vec4Int;
}

#endif //_SUBSTANCE_AIR_FRAMEWORK_TYPEDEFS_H
