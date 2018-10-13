#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <functional>

#include <boost/config.hpp>
#include <boost/range/functions.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/range/adaptors.hpp>

#include <ext/algorithm.hpp>
#include <ext/range.hpp>
#include <ext/functors/ctpred.hpp>
#include <ext/lrucache.hpp>

#include <sqlite3yaw_ext/util.hpp>
#include <sqlite3yaw_ext/table_meta.hpp>

namespace sqlite3yaw
{
	namespace detail
	{
		typedef boost::iterator_range<const char *> CharRange;
		typedef std::vector<CharRange> CharRangeVec;
		
		struct ToCharPtrRangeImpl
		{
			typedef CharRange result_type;

			template <class CharRange>
			result_type operator ()(const CharRange & rng) const
			{
				auto * first = ext::data(rng);
				auto * last = first + boost::size(rng);
				return {first, last};
			}

		} const MakeCharRange;
		

		struct CharPtrRangeVecEqual
		{
			typedef bool result_type;
			result_type operator()(const CharRangeVec & v1, const CharRangeVec & v2) const
			{
				return boost::range::equal(v1, v2, ext::ctpred::less<metastr_traits>());
			}
		};

		struct CharPtrRangeVecHasher
		{
			std::size_t operator()(const CharRangeVec & v) const
			{
				std::size_t seed = 0;
				for (auto r : v) boost::hash_range(seed, r.begin(), r.end());
				return seed;
			}
		};

		BOOST_NORETURN inline
		void ThrowNoPrimaryKey(const table_meta & meta)
		{
			std::string err = "table ";
			err += meta.table_name.c_str();
			err += " has no primary key";
			throw std::invalid_argument(err);
		}

		BOOST_NORETURN inline
		void ThrowRecordHasNoPk()
		{
			throw std::runtime_error("batch_upsert: record missing pk");
		}

		BOOST_NORETURN inline
		void ThrowUnknownField(const CharRange & val)
		{
			std::string err = "unknown field found: ";
			err.append(val.begin(), val.size());
			throw std::runtime_error(err);
		}

		struct CacheItem
		{
			statement stmt;
			int       pkPos;

			CacheItem(statement && stmt_, int pkPos_) : stmt(std::move(stmt_)), pkPos(pkPos_) {}
			CacheItem(CacheItem && r) : stmt(std::move(r.stmt)), pkPos(r.pkPos) {}

			friend void swap(CacheItem & ci1, CacheItem & ci2)
			{ swap(ci1.stmt, ci2.stmt); std::swap(ci1.pkPos, ci2.pkPos); }
		};


		template <class RandomAccessIterator>
		void bind_helper_ll(statement & stmt, RandomAccessIterator first, RandomAccessIterator last, int keyIdx, std::random_access_iterator_tag)
		{
			bind(stmt, stmt.bind_parameter_count(), *(first + keyIdx));
			for (int idx = 0; first != last; ++first)
				bind(stmt, ++idx, *first);
		}

		template <class SinglePassIterator>
		void bind_helper_ll(statement & stmt, SinglePassIterator first, SinglePassIterator last, int keyIdx, std::input_iterator_tag)
		{
			for (int idx = 0; first != last; ++first)
			{
				if (idx == keyIdx)
					bind(stmt, stmt.bind_parameter_count(), *first);
				bind(stmt, ++idx, *first);
			}
		}

		/// helper function
		/// it binds values from range to a statement like
		/// "update test set f1 = ? ... set kf = ? ... where kf = ?"
		template <class Range>
		void bind_helper(statement & stmt, const Range & rng, int keyIdx)
		{
			typedef typename boost::range_iterator<Range>::type iterator;
			typedef typename boost::iterator_category<iterator>::type category;
			bind_helper_ll(stmt, boost::begin(rng), boost::end(rng), keyIdx, category());
		}
	}
	
	/// inserts records into table described by meta in one transaction
	/// record is a range of pair or pair like type. It accessed throw std::get<0>/std::get<1>
	/// expression std::get<0/1>(*records.begin().begin()) must be valid
	template <class ForwardRange>
	void batch_insert(const ForwardRange & records, session & ses, const table_meta & meta)
	{
		using namespace detail;
		CharRangeVec fields;
		fields.reserve(meta.fields.size());
		for (auto & f : meta.fields)
			fields.push_back(MakeCharRange(f.name));

		ext::ctpred::less<metastr_traits> less;
		boost::sort(fields, less);
		auto cmd = insert_command(meta.table_name, fields);

		transaction tr(ses);
		auto stmt = ses.prepare(cmd);

		for (const auto & rec : records)
		{
			for (auto && valPair : rec)
			{
				using std::get;
				auto fname = MakeCharRange(get<0>(valPair));

				auto pos = ext::binary_find(fields.begin(), fields.end(), fname, less)
				           - fields.begin();

				if (pos == fields.size())
					ThrowUnknownField(fname);

				sqlite3yaw::bind(stmt, static_cast<int>(pos + 1), get<1>(std::forward<decltype(valPair)>(valPair)));
			}

			stmt.step();
			stmt.reset();
			stmt.clear_bindings();
		}

		stmt.finalize();
		tr.commit();
	}

	/// upserts records into table described by meta in one transaction
	/// record is a range of pair or pair like type. It accessed throw std::get<0>/std::get<1>
	/// expression std::get<0/1>(*records.begin().begin()) must be valid
	/// 
	/// upsert means try update, if no such record - insert
	///
	/// IMPL NOTE: each record in records traversed twice(so if you use some transforming iterator, you may be better buffer it)
	template <class ForwardRange>
	void batch_upsert(const ForwardRange & records, session & ses, const table_meta & meta)
	{
		using namespace detail;
		typedef ext::manual_lru_cache<
			CharRangeVec, CacheItem, 
			CharPtrRangeVecHasher, CharPtrRangeVecEqual
		> cache_type;

		if (meta.pk.empty())
			ThrowNoPrimaryKey(meta);

		auto isPk = boost::bind(ext::ctpred::equal_to<metastr_traits>(), boost::cref(meta.pk), _1);
		// moved outside for performance
		// vector will not realloc each iteration
		CharRangeVec curFieldNames;
		curFieldNames.reserve(meta.fields.size());

		transaction tr(ses);
		cache_type cache(500);

		for (const auto & rec : records)
		{
			curFieldNames.clear();
			boost::push_back(curFieldNames, rec | ext::firsts | boost::adaptors::transformed(MakeCharRange));

			auto * item = cache.find_ptr(curFieldNames);
			if (!item) // create new command
			{
				auto pk = boost::find_if(curFieldNames, isPk);
				if (pk == curFieldNames.end())
					ThrowRecordHasNoPk();

				int pkPos = static_cast<int>(pk - curFieldNames.begin());
				auto new_stmt = ses.prepare(update_command(meta.table_name, curFieldNames, *pk));
				item = &cache.insert(curFieldNames, CacheItem(std::move(new_stmt), pkPos));
			}

			auto & stmt = item->stmt;
			// bind both values and where <pk> = ?
			detail::bind_helper(stmt, rec | ext::seconds, item->pkPos);
			stmt.step();

			if (!ses.changes()) // no such records - insert
			{
				auto stmt = ses.prepare(insert_command(meta.table_name, curFieldNames));
				boost::for_each(rec | ext::seconds, auto_binder(stmt));
				stmt.step();
			}

			// clear command for reuse from cache
			stmt.reset();
			stmt.clear_bindings();
		}

		cache.clear();
		tr.commit();
	}
}
