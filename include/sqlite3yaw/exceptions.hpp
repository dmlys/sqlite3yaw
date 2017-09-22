#pragma once
#include <system_error>
#include <string>

#include <sqlite3yaw/sqlite3inc.h>
#include <sqlite3yaw/fwd.hpp>

namespace sqlite3yaw
{
	namespace detail
	{
		struct sqlite_category_impl_t : std::error_category
		{
			const char * name() const noexcept override { return "sqlite3_error"; }
			std::string message(int err) const override
			{
				return sqlite3_errstr(err);
			}
		};

		const sqlite_category_impl_t sqlite_category_impl;
	}

	inline
	std::error_category const & sqlite_category() { return detail::sqlite_category_impl; }

	class sqlite_error : public std::system_error
	{
	public:
		sqlite_error(int errCode)
			: std::system_error(std::error_code(errCode, sqlite_category()))
		{}
	};

	class sqlite_exterror : public sqlite_error
	{
		std::string whatMsg;

	public:
		sqlite_exterror(int errCode, sqlite3 * db = nullptr)
			: sqlite_error(errCode)
		{
			auto err = code();
			whatMsg += err.category().name();
			whatMsg += ":";
			whatMsg += std::to_string(err.value());
			whatMsg += " <";
			whatMsg += err.message();
			whatMsg += ">";
			
			if (db) {
				whatMsg += ", errmsg: '";
				whatMsg += sqlite3_errmsg(db);
				whatMsg += "'";
			}
		}

		const char * what() const noexcept override
		{
			return whatMsg.c_str();
		}
	};
}