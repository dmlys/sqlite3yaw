#pragma once
#include <sqlite3yaw/convert.hpp>
#include <boost/optional.hpp>

namespace sqlite3yaw
{
	namespace convert
	{
		template <class Type>
		struct conv<boost::optional<Type>>
		{
			typedef boost::optional<Type> Opt;
			static void put(Opt const & val, bool temp, ibind & b)
			{
				if (val.is_initialized())
					conv<Type>::put(val.get(), temp, b);
				else
					conv<std::nullptr_t>::put(nullptr, false, b);
			}

			static void get(Opt & val, iquery & q)
			{
				if (q.get_type() == SQLITE_NULL)
					val = boost::none;
				else
					conv<Type>::get(q, val);
			}
		};

		template <>
		struct conv<boost::none_t>
		{
			static void put(boost::none_t, bool, ibind & b) { conv<std::nullptr_t>::put(nullptr, false, b); }
			//static void get(boost::none_t & val, iquery & b) { val = boost::none; }
		};
	}
}