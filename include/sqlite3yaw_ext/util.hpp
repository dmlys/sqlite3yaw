#pragma once
#include <string>
#include <iterator>
#include <algorithm>
#include <cassert>

#include <boost/range.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/range/as_literal.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <ext/iterator/append_iterator.hpp>
#include <ext/functors.hpp>
#include <ext/functors/ctpred.hpp>
#include <ext/range/adaptors.hpp>

#include <sqlite3yaw.hpp>
#include <sqlite3yaw_ext/table_meta.hpp>

namespace sqlite3yaw
{
	/// copies name from range to out as '"' + range + '"', any '"' from range is doubled
	/// effectively escapes sql identifier
	template <class SinglePassRange, class OutIterator>
	OutIterator copy_sql_name(const SinglePassRange & sql_name, OutIterator out)
	{
		*out = '"'; ++out;
		out = boost::algorithm::replace_all_copy(out, sql_name, "\"", "\"\""); //" -> ""
		*out = '"'; ++out;
		return out;
	}

	template <class SinglePassRange, class Separator, class OutIterator>
	OutIterator join_sql_names(const SinglePassRange & sql_names, const Separator & sep, OutIterator out)
	{
		auto beg = boost::begin(sql_names);
		auto end = boost::end(sql_names);

		if (beg == end)
			return out;

		out = copy_sql_name(boost::as_literal(*beg), out);

		auto sepr = boost::as_literal(sep);
		for (auto it = ++beg; it != end; ++it)
		{
			out = boost::copy(sepr, out);
			out = copy_sql_name(boost::as_literal(*it), out);
		}

		return out;
	}

	/// copies name from range to out as '"' + range + '"', any '"' from range is doubled
	/// effectively escapes sql identifier
	template <class Range>
	Range copy_sql_name(const Range & sql_name)
	{
		Range ret;
		copy_sql_name(sql_name, std::back_inserter(ret));
		return ret;
	}

	/// creates update command, with binding places
	/// <update_word> <table> set <colName> = ? ...
	/// where is not added!
	/// colNames - SinglePass range, whose elements is some char range
	template <class UpdateWordRange, class CharRange, class SinglePassRange>
	std::string custom_update_command(const UpdateWordRange & update_word, const CharRange & table_name,
	                                  const SinglePassRange & col_names)
	{
		std::string command;
		command.reserve(256);
		auto bi = std::back_inserter(command);

		boost::push_back(command, boost::as_literal(update_word));
		command += ' ';
		copy_sql_name(boost::as_literal(table_name), bi);
		command += " set ";

		for (const auto & col : col_names)
		{
			copy_sql_name(boost::as_literal(col), bi);
			command += " = ?,";
		}
		command.pop_back(); //delete extra comma

		return command;
	}

	/// creates update command, with binding places
	/// <update_word> <table> set <colName> = ? ...
	/// where is not added!
	/// colNames - SinglePass range, whose elements is some char range
	template <class UpdateWordRange, class CharRange, class SinglePassRange, class PkCharRange>
	std::string custom_update_command(const UpdateWordRange & update_word, const CharRange & table_name,
	                                  const SinglePassRange & col_names, const PkCharRange & pk)
	{
		auto command = custom_update_command(update_word, table_name, col_names);
		auto pkr = boost::as_literal(pk);
		command += " where ";
		copy_sql_name(pkr, std::back_inserter(command));
		command += " = ?";

		return command;
	}	

	/// creates update command, with binding places
	/// update <table> set <colName> = ? ...
	/// where is not added!
	/// colNames - SinglePass range, whose elements is some char range
	template <class CharRange, class SinglePassRange>
	inline std::string update_command(const CharRange & table_name, const SinglePassRange & col_names)
	{
		return custom_update_command("update", table_name, col_names);
	}

	/// creates update command, with binding places
	/// update <table> set <colName> = ? ... where <pk> = ?
	/// colNames - SinglePass range, whose elements is some char range
	template <class CharRange, class SinglePassRange, class PkCharRange>
	inline std::string update_command(const CharRange & table_name, const SinglePassRange & col_names, const PkCharRange & pk)
	{
		return custom_update_command("update", table_name, col_names, pk);
	}

	/// creates insert command, with binding places
	/// <insert_word> into table(<colName>, <colName>, ...) values(?,?,...)
	/// colNames - SinglePass range, whose elements is some char range
	template <class InsertWordRange, class CharRange, class SinglePassRange>
	std::string custom_insert_command(const InsertWordRange & insert_word, const CharRange & table_name,
	                                  const SinglePassRange & col_names)
	{
		std::string command;
		command.reserve(256);
		auto bi = std::back_inserter(command);

		boost::push_back(command, boost::as_literal(insert_word));
		command += " into ";
		copy_sql_name(boost::as_literal(table_name), bi);
		command += " ( ";

		std::size_t count = 0;
		for (const auto & col : col_names)
		{
			copy_sql_name(boost::as_literal(col), bi);
			command.push_back(',');
			++count;
		}
		command.pop_back(); //delete extra comma

		command += ") values ( ";
		std::fill_n(ext::make_append_iterator(command), count, boost::as_literal("?,"));
		command.pop_back(); //delete extra comma

		command += ")";
		return command;
	}

	template <class CharRange, class SinglePassRange>
	inline std::string insert_command(const CharRange & table_name, const SinglePassRange & col_names)
	{
		return custom_insert_command("insert", table_name, col_names);
	}

	template <class CharRange, class SinglePassRange>
	std::string select_command(const CharRange & table_name, const SinglePassRange & col_names)
	{
		std::string command;
		command.reserve(256);

		command += "select ";
		join_sql_names(col_names, ", ", std::back_inserter(command));
		command += " from ";
		copy_sql_name(boost::as_literal(table_name), std::back_inserter(command));
		return command;
	}

	/// inserts data tagval_range into table represented by meta
	/// 
	/// tagval_range - range of tagval, which is pair or pair like class
	///   first element is some char range, representing tag
	///   second element is some char range, representing value
	template <class ForwardRange>
	void insert_record(session & ses, table_meta const & meta, ForwardRange const & tagval_range)
	{
		auto command = insert_command(meta.table_name, tagval_range | ext::firsts);
		auto stmt = ses.prepare(command);
		boost::for_each(tagval_range | ext::seconds, auto_binder(stmt));

		stmt.step();
		stmt.finalize();
	}

	/// updates data tagval_range into table represented by meta by primary key
	/// primary key is taken from meta.pk
	/// calling this function on table which is not having primary key - logic_error
	/// calling this function whith tagval_range not having primary key field - { err_action(meta.pk); return; }
	/// 
	/// tagval_range - range of tagval, which is pair or pair like class
	///   first element is some char range, representing tag
	///   second element is some char range, representing value
	template <class ForwardRange>
	void update_record(session & ses, table_meta const & meta, ForwardRange const & tagval_range)
	{
		assert(!meta.pk.empty());
		if (meta.pk.empty())
			throw std::invalid_argument("update_record called on table which does not have primary key");

		auto isPk = std::bind(ext::ctpred::equal_to<metastr_traits>(), meta.pk, std::placeholders::_1);
		auto iPkTagVal = boost::find_if(tagval_range | ext::firsts, isPk).base();
		assert(iPkTagVal != boost::end(tagval_range));
		if (iPkTagVal == boost::end(tagval_range))
			throw std::runtime_error("missing pk");

		auto command = update_command(meta.table_name, tagval_range | ext::firsts, meta.pk);
		auto stmt = ses.prepare(command);
		sqlite3yaw::auto_binder binder(stmt);
		boost::for_each(tagval_range | ext::seconds, std::ref(binder));
		binder(std::get<1>(*iPkTagVal));

		stmt.step();
		stmt.finalize();
	}

	///tries update record with update_record(ses, meta, tagval_range)
	///if record with this key was not in database - insert_record(ses, meta, tagval_range)
	///see also inser_record, update_record
	template <class ForwardRange>
	void upsert_record(session & ses, table_meta const & meta, ForwardRange const & tagval_range)
	{
		update_record(ses, meta, tagval_range);
		if (!ses.changes())
			insert_record(ses, meta, tagval_range);
	}
}