#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include <sqlite3yaw/config.hpp>
#include <sqlite3yaw/sqlite3inc.h>
#include <sqlite3yaw/exceptions.hpp>
#include <sqlite3yaw/to_int.hpp>

namespace sqlite3yaw
{
	class statement
	{
		sqlite3_stmt * stmt;


	public:
		statement(const statement &) = delete;
		statement & operator =(const statement &) = delete;

		statement() noexcept : stmt(nullptr) {};
		statement(sqlite3_stmt * stmt_) noexcept : stmt(stmt_) {};
		statement(statement && r) noexcept : stmt(r.release()) {}

		statement & operator =(statement && r) noexcept
		{
			if (this != &r)
			{
				finalize();
				stmt = std::exchange(r.stmt, nullptr);
			}
			
			return *this;
		}

		friend void swap(statement & s1, statement & s2) noexcept
		{
			std::swap(s1.stmt, s2.stmt);
		}

		~statement() noexcept
		{
			if (stmt)
				sqlite3_finalize(stmt); //nothrow
		}

		sqlite3_stmt * native() const noexcept { return stmt; }
		sqlite3_stmt * release() noexcept
		{
			sqlite3_stmt * ret = stmt;
			stmt = nullptr;
			return ret;
		}

		explicit operator bool() const noexcept 
		{ 
			return stmt != nullptr;
		}

		void assign(sqlite3_stmt * stmt)
		{
			finalize();
			statement::stmt = stmt;
		}

		/// sqlite3_finilize returns most recent error code if any
		/// pass it to user, it's not a error
		int finalize() noexcept
		{
			int res = SQLITE_OK;
			if (stmt) {
				res = sqlite3_finalize(stmt);
				release();
			}
			return res;
		}

		sqlite3 * db_handle() const { return sqlite3_db_handle(stmt); }
		const char * sql() const    { return sqlite3_sql(stmt); }
		//raw binders. For more sophisticated bind see binders in bind.hpp
		//remember that bind index starts from 1, not 0
		void bind_null(int idx)                      { check_result(sqlite3_bind_null(stmt, idx)); }
		void bind_int(int idx, int val)              { check_result(sqlite3_bind_int(stmt, idx, val)); }
		void bind_int64(int idx, sqlite3_int64 val)   { check_result(sqlite3_bind_int64(stmt, idx, val)); }
		void bind_double(int idx, double val)        { check_result(sqlite3_bind_double(stmt, idx, val)); }

		void bind_text(int idx, const char * str, int len, void (* ffree)(void *))
		{
			check_result(sqlite3_bind_text(stmt, idx, str, len, ffree));
		}

		void bind_text(int idx, const char * str, int len, bool copy)
		{
			check_result(sqlite3_bind_text(stmt, idx, str, len, copy ? SQLITE_TRANSIENT : SQLITE_STATIC));
		}

		/// small extension for std::basic_string.
		/// we accept any char based std::basic_string,
		/// don't care about allocator and char_traits, cause they are not used
		/// the only demand is that string have to be utf-8
		/// 
		/// also traits can specify some meta property, for example, case sensitivity,
		/// which would be lost. That programmer should keep in mind.
		/// 
		/// if we wouldn't accept traits, than programmer could still easily call .c_str()
		/// through it would be explicit
		/// 
		/// @Param copy - SQLITE_STATIC or SQLITE_TRANSIENT
		template <class String>
		void bind_text(int idx, String const & str, bool copy)
		{
			bind_text(idx, str.data(), ToInt(str.size()), copy);
		}
		
		/// sqlite3_reset returns most recent error code if any
		/// pass it to user, it's not a error
		int reset() noexcept {return sqlite3_reset(stmt);}
		void clear_bindings()           { check_result( sqlite3_clear_bindings(stmt) ); }

		/// executes step
		/// returns true if there are more records, false otherwise
		/// throws error if any other code
		bool step()
		{
			int res = sqlite3_step(stmt);

			switch (res) {
			case SQLITE_ROW: return true;
			case SQLITE_DONE: return false;
			case SQLITE_OK: throw std::logic_error("unexpected SQLITE_OK");
			default:
				throw sqlite_exterror(res, db_handle());
			}
		}

		int step_ex()
		{
			return sqlite3_step(stmt);
		}

		int bind_parameter_index(const std::string & name) const
		{
			return bind_parameter_index(name.c_str());
		}

		int bind_parameter_index(const char * name) const
		{
			return sqlite3_bind_parameter_index(stmt, name);
		}

		const char * bind_parameter_name(int idx) const
		{
			return sqlite3_bind_parameter_name(stmt, idx);
		}

		int bind_parameter_count() const
		{
			return sqlite3_bind_parameter_count(stmt);
		}

		/// result functions
		const char * column_name(int idx) const        { return sqlite3_column_name(stmt, idx); }
		int column_count() const                       { return sqlite3_column_count(stmt); }
		int data_count() const                         { return sqlite3_data_count(stmt); }
		int column_type(int idx) const                 { return sqlite3_column_type(stmt, idx); }
		const char * column_decltype(int idx) const    { return sqlite3_column_decltype(stmt, idx); }

		int column_int(int idx) const                  { return sqlite3_column_int(stmt, idx); }
		sqlite3_int64 column_int64(int idx) const      { return sqlite3_column_int64(stmt, idx); }
		double column_double(int idx) const            { return sqlite3_column_double(stmt, idx); }
		int column_bytes(int idx) const                { return sqlite3_column_bytes(stmt, idx); }

		const char * column_text(int idx) const   { return reinterpret_cast<const char *>(sqlite3_column_text(stmt, idx)); }

		template <class String>
		void column_string(int idx, String & res) const
		{
			auto text = column_text(idx);
			auto sz = column_bytes(idx);
			res.assign(text, text + sz);
		}

		//template <class String /*= std::string*/>
		std::string column_string(int idx) const
		{
			std::string str;
			column_string(idx, str);
			return str;
		}

		/// metadata interface
		const char * column_database_name(int n) const { return sqlite3_column_database_name(stmt, n); }
		const char * column_table_name(int n) const    { return sqlite3_column_table_name(stmt, n); }
		const char * column_origin_name(int n) const   { return sqlite3_column_origin_name(stmt, n); }

	private:
		void check_result(int res)
		{
			if (res != SQLITE_OK)
				throw sqlite_exterror(res, db_handle());
		}

	};
}
