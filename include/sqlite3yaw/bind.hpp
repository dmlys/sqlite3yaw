#pragma once
#include <cassert>
#include <string>
#include <sqlite3yaw/convert.hpp>

namespace sqlite3yaw
{
	struct no_such_named_param : std::invalid_argument
	{
		no_such_named_param(std::string const & pname)
			: std::invalid_argument("no such named param: " + pname) {}
	};

	template <class Type>
	void bind(statement & stmt, int idx, Type && val, bool forceCopy = false)
	{
		assert(idx != 0 && idx <= stmt.bind_parameter_count());
		convert::ibind b(stmt, idx);
		
		typedef typename std::decay<Type>::type PureType;

		// if you get here undefined type conv<Type>
		// then your type conversion is not registered. see convert.hpp
		convert::conv<PureType>::put(
			std::forward<Type>(val),
			std::is_rvalue_reference<Type &&>::value || forceCopy,
			b);
	}

	template <class Type>
	void bind(statement & stmt, const char * name, Type && val, bool forceCopy = false)
	{
		int idx = stmt.bind_parameter_index(name);
		if (idx == 0) throw no_such_named_param(name);

		bind(stmt, idx, std::forward<Type>(val), forceCopy);
	}

	template <class Type>
	void bind(statement & stmt, const std::string & name, Type && val, bool forceCopy = false)
	{
		int idx = stmt.bind_parameter_index(name);
		if (idx == 0) throw no_such_named_param(name);

		bind(stmt, idx, std::forward<Type>(val), forceCopy);
	}

	class binder
	{
		statement * stmt;
		bool forceCopy;
	
	public:
		typedef void result_type; //for bind, ref, cref compability

		binder(statement & stmt_, bool forceCopy_ = false)
			: stmt(&stmt_), forceCopy(forceCopy_) {}

		template <class Type>
		void operator ()(int idx, Type && val) const
		{
			bind(*stmt, idx, std::forward<Type>(val), forceCopy);
		}

		template <class Type>
		void operator ()(const char * name, Type && val) const
		{
			bind(*stmt, name, std::forward<Type>(val));
		}

		template <class Type>
		void operator ()(const std::string & name, Type && val) const
		{
			bind(*stmt, name, std::forward<Type>(val));
		}
	};

	class auto_binder
	{
		binder b;
		int idx;
	
	public:
		typedef void result_type; //for bind, ref, cref compability

		auto_binder(statement & stmt_, bool forceCopy_ = false)
			: b(stmt_, forceCopy_), idx(0) {}

		auto_binder(statement & stmt_, int startIdx_, bool forceCopy_ = false)
			: b(stmt_, forceCopy_), idx(startIdx_) { --idx; }

		template <class Type>
		void operator ()(Type && val)
		{
			//sqlite bind indexes start from 1, not 0
			b(++idx, std::forward<Type>(val));
		}
	};
	
	class bind_iterator :
		public std::iterator<
			std::output_iterator_tag,
			void, void, void, void
		>
	{
		auto_binder b;
		
		struct proxy
		{
			auto_binder & b;
			proxy(auto_binder & b_) : b(b_) {}
			
			template <class	Type>
			proxy & operator =(Type && val)
			{
				b(std::forward<Type>(val)); return *this;
			}
		};

	public:
		explicit bind_iterator(statement & stmt, bool forceCopy = false) : b(stmt, forceCopy) {}
		explicit bind_iterator(statement & stmt, int idx, bool forceCopy = false) : b(stmt, idx, forceCopy) {}

		bind_iterator & operator ++() { return *this; }
		bind_iterator & operator ++(int) { return *this; }
		proxy operator *() { return proxy(b); }
	};
}

#ifdef _MSC_VER
namespace std
{
	template <>
	struct _Is_checked_helper<sqlite3yaw::bind_iterator> :
		public std::true_type {};
}
#endif