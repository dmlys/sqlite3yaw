#pragma once
#include <cstring>
#include <sqlite3yaw/convert.hpp>

namespace sqlite3yaw
{
	struct no_such_column : std::invalid_argument
	{
		no_such_column(std::string const & columnName)
			: std::invalid_argument("no such column name: " + columnName) {}
	};
	
	//returns -1 if not found
	inline
	int column_name_to_index(statement & stmt, const char * name)
	{
		for (int i = 0; i < stmt.column_count(); ++i)
			if (std::strcmp(name, stmt.column_name(i)) == 0)
				return i;
		return -1;
	}

	//returns -1 if not found
	inline
	int column_name_to_index(statement & stmt, const std::string & name)
	{
		return column_name_to_index(stmt, name.c_str());
	}

	template <class Type>
	void get(statement & stmt, int idx, Type & val)
	{
		convert::iquery q(stmt, idx);
		typedef typename std::decay<Type>::type PureType;

		// if you get here undefined type conv<Type>
		// then your type conversion is not registered. see convert.hpp
		convert::conv<PureType>::get(val, q);
	}

	template <class Type>
	void get(statement & stmt, const std::string & columnName, Type & val)
	{
		int idx = column_name_to_index(stmt, columnName);
		if (idx == -1)
			throw no_such_column(columnName);

		get(stmt, idx, val);
	}
	

	template <class Type>
	Type get(statement & stmt, int idx)
	{
		Type val;
		get(stmt, idx, val);
		return val;
	}

	template <class Type>
	Type get(statement & stmt, const std::string & columnName)
	{
		Type val;
		get(stmt, columnName, val);
		return val;
	}

	class getter
	{
		statement * stmt;

	public:
		typedef void result_type; //for bind, ref, cref compability

		getter(statement & stmt_) : stmt(&stmt_) {}

		template <class Type>
		void operator()(int idx, Type & val) const
		{
			get(*stmt, idx, val);
		}

		template <class Type>
		void operator()(std::string const & columnName, Type & val) const
		{
			get(*stmt, columnName, val);
		}
	};
	
	class auto_getter
	{
		getter g;
		int idx;
	public:
		typedef void result_type; //for bind, ref, cref compability

		auto_getter(statement & stmt) : g(stmt), idx(0) {}

		template <class Type>
		void operator()(Type & val) { g(idx++, val); }
	};

	template <class Type>
	class getgen
	{
		auto_getter g;
	public:
		typedef Type result_type; //for bind, ref, cref compability

		getgen(statement & stmt) : g(stmt) {}

		Type operator()()
		{
			Type val;
			g(val);
			return val;
		}
	};
}