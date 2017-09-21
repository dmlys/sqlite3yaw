#pragma once
#include <sqlite3yaw/convert.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace sqlite3yaw
{
	namespace convert
	{
		template <>
		struct conv<boost::none_t>
		{
			static void put(boost::none_t, bool, ibind & b) { conv<std::nullptr_t>::put(nullptr, false, b); }
			//static void get(boost::none_t & val, iquery & b) { val = boost::none; }
		};

		template <class Type>
		struct conv<boost::optional<Type>>
		{
			typedef boost::optional<Type> optional;
			static void put(const optional & val, bool temp, ibind & b)
			{
				if (val.is_initialized())
					conv<Type>::put(val.get(), temp, b);
				else
					conv<boost::none_t>::put(boost::none, false, b);
			}

			static void get(optional & val, iquery & q)
			{
				if (q.get_type() == SQLITE_NULL)
					val = boost::none;
				else
					conv<Type>::get(q, val);
			}
		};


		struct conv_variant_visitor : boost::static_visitor<void>
		{
			ibind * bptr;
			bool temp;

			template <class Type>
			void operator()(const Type & val) const
			{
				return conv<Type>::put(val, temp, *bptr);
			}

			conv_variant_visitor(ibind & b, bool temp)
				: bptr(&b), temp(temp) {}
		};


		template <class ... Type>
		struct conv<boost::variant<Type...>>
		{
			typedef boost::variant<Type...> variant;

			static void put(const variant & var, bool temp, ibind & b)
			{
				return boost::apply_visitor(conv_variant_visitor(b, temp), var);
			}
		};
	}
}