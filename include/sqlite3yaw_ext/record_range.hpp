#pragma once
#include <type_traits> //for std::result_of
#include <sqlite3yaw/statement.hpp>
#include <ext/range/input_range_facade.hpp>

namespace sqlite3yaw
{
	template <class UnaryFunctor>
	class record_range :
		public ext::input_range_facade <
			record_range<UnaryFunctor>,
			typename boost::result_of<UnaryFunctor(sqlite3yaw::statement &)>::type,
			typename boost::result_of<UnaryFunctor(sqlite3yaw::statement &)>::type
		>
	{
		typedef ext::input_range_facade <
			record_range<UnaryFunctor>,
			typename boost::result_of<UnaryFunctor(sqlite3yaw::statement &)>::type,
			typename boost::result_of<UnaryFunctor(sqlite3yaw::statement &)>::type
		> base_type;

	public:
		using typename base_type::const_reference;

	private:
		UnaryFunctor uf;
		sqlite3yaw::statement * stmt;
		bool avail = true;

	public:
		const_reference front() { return uf(*stmt); }
		void pop_front() { avail = stmt->step(); }
		bool empty() const { return !avail; }

		record_range(sqlite3yaw::statement & stmt, UnaryFunctor uf = {})
			: uf(uf), stmt(&stmt)
		{
			pop_front();
		}

		record_range(const record_range &) = delete;
		record_range & operator =(const record_range &) = delete;

		record_range(record_range && op) /*= default;*/
			: uf(std::move(op.uf)), stmt(op.stmt), avail(op.avail)
		{}
	};

	template <class UnaryFunctor>
	record_range<UnaryFunctor> make_record_range(sqlite3yaw::statement & stmt, UnaryFunctor uf)
	{
		return record_range<UnaryFunctor>(stmt, uf);
	}
}