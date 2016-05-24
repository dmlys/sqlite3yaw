#pragma once

#ifndef SQLITE3YAW_NOSTDORD

#include <climits>
#include <type_traits>
#include <sqlite3yaw/convert.hpp>

namespace sqlite3yaw
{
	namespace convert
	{
		namespace detail
		{
			// it is not known what sqlite3_int64 is exactly,
			// it can be long long, but it can be long as well(at least via macro SQLITE_INT64_TYPE).
			// if we try to make a long specialization, but sqlite3_int64 is long - error.
			// so we check if they are the same, and if - fallback to dummy.
			// analogues for long long
			struct dummy {};

			typedef std::conditional<
				std::is_same<sqlite3_int64, long long>::value,
				dummy, long long
			>::type long_long;

			typedef std::conditional<
				std::is_same<sqlite3_int64, long>::value,
				dummy, long
			>::type long_;
		}

		//standard C++ types like unsigned, float, which are not supported by sqlite3
		template <>
		struct conv<unsigned>  //unsigned int as int
		{
			static void put(unsigned val, bool, ibind & b) { b.bind(static_cast<int>(val)); }
			static void get(unsigned & val, iquery & q) { val = q.get_int(); }
		};

		template <>
		struct conv<detail::long_long>
		{
			static void put(long long val, bool, ibind & b) { b.bind(static_cast<sqlite3_int64>(val)); }
			static void get(long long & val, iquery & q) { val = q.get_int64(); }
		};

		template <>
		struct conv<unsigned long long> // unsigned long long as sqlite3_int64
		{
			static void put(unsigned long long val, bool, ibind & b) { b.bind(static_cast<sqlite3_int64>(val)); }
			static void get(unsigned long long & val, iquery & q) { val = q.get_int64(); }
		};
		
#if LONG_MAX == INT_MAX //long as int
		template <>
		struct conv<detail::long_>
		{
			static void put(long val, bool, ibind & b) { b.bind(static_cast<int>(val)); }
			static void get(long & val, iquery & q) { val = q.get_int(); }
		};

		template <>  //unsigned long as long
		struct conv<unsigned long>
		{
			static void put(unsigned long val, bool, ibind & b) { b.bind(static_cast<int>(val)); }
			static void get(unsigned long & val, iquery & q) { val = q.get_int(); }
		};
#else //long as sqlite3_int64
		template <>
		struct conv<detail::long_>
		{
			static void put(long val, bool, ibind & b) { b.bind(static_cast<sqlite3_int64>(val)); }
			static void get(long & val, iquery & q) { val = q.get_int64(); }
		};

		template <>   //unsigned long as long
		struct conv<unsigned long>
		{
			static void put(unsigned long val, bool, ibind & b) { b.bind(static_cast<sqlite3_int64>(val)); }
			static void get(unsigned long & val, iquery & q) { val = q.get_int64(); }
		};
#endif

		template <>  //float as double
		struct conv<float>
		{
			static void put(float val, bool, ibind & b) { b.bind(static_cast<double>(val)); }
			static void get(float & val, iquery & q) { val = static_cast<float>(q.get_double()); }
		};

		template<>  //short as int
		struct conv<short>
		{
			static void put(short val, bool, ibind & b) { b.bind(static_cast<int>(val)); }
			static void get(short & val, iquery & q) { val = static_cast<short>(q.get_int()); }
		};

		template<> //unsigned short as int
		struct conv<unsigned short>
		{
			static void put(unsigned short val, bool, ibind & b) { b.bind(static_cast<int>(val)); }
			static void get(unsigned short & val, iquery & q) { val = static_cast<unsigned short>(q.get_int()); }
		};
	}
}

#endif