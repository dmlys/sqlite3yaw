#pragma once
#include <sqlite3yaw/query.hpp>
#include <boost/iterator/iterator_facade.hpp>

namespace sqlite3yaw
{
	template <class Type>
	class get_iterator :
		public boost::iterator_facade<
			get_iterator<Type>,
			Type,
			boost::single_pass_traversal_tag,
			Type
		>
	{
		friend boost::iterator_core_access;

		int idx;
		statement * stmt;
		
		void increment()
		{
			++idx;
			if (idx == stmt->column_count())
				set_at_end();
		}
		Type dereference() const
		{
			Type val;
			get(*stmt, idx, val);
			return val;
		}

		void set_at_end() { idx = 0; stmt = nullptr; }
		bool equal(get_iterator const & other) const { return stmt == other.stmt && idx == other.idx; }

	public:
		get_iterator(statement & stmt_) : idx(0), stmt(&stmt_) {}
		get_iterator() : idx(0), stmt(nullptr) {}
	};

}