#pragma once
#include <sqlite3yaw/session.hpp>

namespace sqlite3yaw
{
	namespace convert
	{
		//bind interface used by putters
		//this is used to not expose idx param for put methods
		class ibind
		{
			int idx;
			statement & stmt;

		public:
			ibind(statement & stmt_, int idx_) : idx(idx_), stmt(stmt_) {}

			void bind(std::nullptr_t)                               { stmt.bind_null(idx); }
			void bind(int i)                                        { stmt.bind_int(idx, i); }
			void bind(sqlite3_int64 i)                              { stmt.bind_int64(idx, i); }
			void bind(double d)                                     { stmt.bind_double(idx, d); }
			void bind(const char * str, int len, bool copy = true)  { stmt.bind_text(idx, str, len, copy); }
			
			//see also statement::bind_text
			template <class String>
			void bind(String const & str, bool copy = true)
			{
				stmt.bind_text(idx, str, copy);
			}
		};

		//query interface used by getters
		//this is used to not expose idx param for get methods
		class iquery
		{
			int idx;
			statement & stmt;

		public:
			iquery(statement & stmt_, int idx_) : idx(idx_), stmt(stmt_) {}

			int  get_type()                    { return stmt.column_type(idx); }
			int  get_int()                     { return stmt.column_int(idx); }
			sqlite3_int64  get_int64()         { return stmt.column_int64(idx); }
			double  get_double()               { return stmt.column_double(idx); }
			const char * get_text()            { return stmt.column_text(idx); }
			int  get_bytes()                   { return stmt.column_bytes(idx); }

			template <class String>
			void get_string(String & str)
			{
				stmt.column_string(idx, str);
			}
		};

		template <class Type>
		struct conv;

		template <>
		struct conv<int>
		{
			static void put(int val, bool temp, ibind & b) { b.bind(val); }
			static void get(int & val, iquery & q) { val = q.get_int(); }
		};

		template <>
		struct conv<double>
		{
			static void put(double val, bool temp, ibind & b) { b.bind(val); }
			static void get(double & val, iquery & q) { val = q.get_double(); }
		};

		template <>
		struct conv<sqlite3_int64>
		{
			static void put(sqlite3_int64 val, bool temp, ibind & b) { b.bind(val); }
			static void get(sqlite3_int64 & val, iquery & q) { val = q.get_int64(); }
		};


		template <>
		struct conv<const char *>
		{
			static void put(const char * val, bool temp, ibind & b) { b.bind(val, -1, temp); }
			static void get(const char * & val, iquery & q) { val = reinterpret_cast<const char *>(q.get_text()); }
		};

		template <class traits, class allocator>
		struct conv<std::basic_string<char, traits, allocator>>
		{
			typedef std::basic_string<char, traits, allocator> string;
			static void put(string const & val, bool temp, ibind & b) { b.bind(val, temp); }
			static void get(string & val, iquery & q) { q.get_string(val); }
		};

		template <>
		struct conv<std::nullptr_t>
		{
			static void put(std::nullptr_t, bool, ibind & b) { b.bind(nullptr); }
			// not needed
			// static void get(std::nullptr_t & val, iquery & q) { val = nullptr; }
		};
	}
}