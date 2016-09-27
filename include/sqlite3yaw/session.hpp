#pragma once

#include <memory>
#include <string>

#include <sqlite3yaw/config.hpp>
#include <sqlite3yaw/sqlite3inc.h>
#include <sqlite3yaw/exceptions.hpp>
#include <sqlite3yaw/statement.hpp>
#include <sqlite3yaw/to_int.hpp>

namespace sqlite3yaw
{
	class session
	{
		//functor that can close sqlite3 handles. For unique_ptr
		struct AutoClose
		{
			void operator ()(sqlite3 * db) const SQLITE3YAW_NOEXCEPT
			{
				//sqlite3_close for null is no-op
				int res = sqlite3_close(db);
				if (res == SQLITE_BUSY) {
					//sqlite3_close may fail if there some references,
					//but we really want to close.
					//v2 will close with references and made it zombie object
					//which will be deleted later when references gone
					sqlite3_close_v2(db);
				}
			}
		};


		std::unique_ptr<sqlite3, AutoClose> db;

	public:
		session(const session &) = delete;
		session & operator =(const session &) = delete;
		session(session && r) = default;
		session & operator =(session && r) = default;

		friend void swap(session & s1, session & s2) SQLITE3YAW_NOEXCEPT
		{
			std::swap(s1.db, s2.db);
		}

		session(sqlite3 * db_) SQLITE3YAW_NOEXCEPT
			: db(db_) {}
		session() SQLITE3YAW_NOEXCEPT
			: db(nullptr) {}

		session(const char * initString)        { open(initString); }
		session(const std::string & initString) { open(initString); }

		session(const char * initString, int flags, const char * vfs = nullptr)        { open(initString, flags, vfs); }
		session(const std::string & initString, int flags, const char * vfs = nullptr) { open(initString, flags, vfs); }

		sqlite3 * native() const SQLITE3YAW_NOEXCEPT { return db.get() ; }
		sqlite3 * release() SQLITE3YAW_NOEXCEPT { return db.release(); }
		/*explicit*/ operator bool() const SQLITE3YAW_NOEXCEPT { return (bool)db; }

		void assign(sqlite3 * db)
		{
			close();
			session::db.reset(db);
		}

		void open(const std::string & initString) { return open(initString.c_str()); }
		void open(const std::string & initString, int flags, const char * vfs = nullptr) { return open(initString.c_str(), flags, vfs); }

		void open(const char * initString)
		{
			close();

			sqlite3 * dbhandle = nullptr;
			int res = sqlite3_open(initString, &dbhandle);
			std::unique_ptr<sqlite3, AutoClose> db(dbhandle);

			if (res != SQLITE_OK)
				throw sqlite_exterror(res, dbhandle);

			swap(db, session::db);
		}

		void open(const char * initString, int flags, const char * vfs = nullptr)
		{
			close();

			sqlite3 * dbhandle = nullptr;
			int res = sqlite3_open_v2(initString, &dbhandle, flags, vfs);
			std::unique_ptr<sqlite3, AutoClose> db(dbhandle);

			if (res != SQLITE_OK)
				throw sqlite_exterror(res, dbhandle);

			swap(db, session::db);
		}

		void close()
		{
			if (db) {
				int res = sqlite3_close(db.get());
				check_result(res);
				release();
			}
		}

		//you probably should use close(), see doc for sqlite3_close_v2
		void close_v2()
		{
			if (db) {
				int res = sqlite3_close_v2(db.get());
				check_result(res);
				release();
			}
		}

		statement prepare(const char * command)
		{
			return prepare(command, static_cast<std::size_t>(-1), nullptr);
		}

		statement prepare(const std::string & command)
		{
			return prepare(command.c_str(), command.size(), nullptr);
		}

		statement prepare(const char * commands, std::size_t size, const char ** tail = nullptr)
		{
			sqlite3_stmt * stmt = nullptr;
			int res = sqlite3_prepare_v2(db.get(), commands, ToInt(size), &stmt, tail);
			check_result(res);
			return stmt;
		}

		int prepare_ex(const char * commands, std::size_t size, statement & stmt, const char ** tail = nullptr) SQLITE3YAW_NOEXCEPT
		{
			sqlite3_stmt * pstmt = nullptr;
			int res = sqlite3_prepare_v2(db.get(), commands, ToInt(size), &pstmt, tail);
			stmt.assign(pstmt);
			return res;
		}

		int prepare_ex(const char * command, statement & stmt) SQLITE3YAW_NOEXCEPT
		{
			return prepare_ex(command, static_cast<std::size_t>(-1), stmt, nullptr);
		}

		int prepare_ex(const std::string & command, statement & stmt) SQLITE3YAW_NOEXCEPT
		{
			return prepare_ex(command.c_str(), command.size(), stmt, nullptr);
		}

		void exec(const std::string & commands)
		{
			return exec(commands.c_str());
		}

		void exec(const char * commands)
		{
			int res = sqlite3_exec(db.get(), commands, nullptr, nullptr, nullptr);
			check_result(res);
		}

		int exec_ex(const std::string & commands) SQLITE3YAW_NOEXCEPT
		{
			return exec_ex(commands.c_str());
		}
		
		int exec_ex(const char * commands) SQLITE3YAW_NOEXCEPT
		{
			return sqlite3_exec(db.get(), commands, nullptr, nullptr, nullptr);
		}

		void busy_timeout(int ms) { check_result( sqlite3_busy_timeout(db.get(), ms) ); }
		//int (* hook)(Type * userArg, int numberOfCallsForThisLockingEvent)
		template <class Type>
		void busy_handler(int (* hook)(Type *, int), Type * arg)
		{
			int res = sqlite3_busy_handler(db.get(),
				reinterpret_cast<int (*)(void *, int)>(hook), arg);
			check_result(res);
		}

		template <class Type>
		void progress_handler(int ninst, int (* hook)(Type *), Type * arg)
		{
			sqlite3_progress_handler(db.get(), ninst, hook, arg);
		}

		void interrupt() { sqlite3_interrupt(db.get()); }

		int errcode() const                    { return sqlite3_errcode(db.get()); }
		const char * errmsg() const            { return sqlite3_errmsg(db.get()); }
		static const char * errstr(int err)    { return sqlite3_errstr(err); }

		sqlite3_int64 last_insert_rowid() const   { return sqlite3_last_insert_rowid(db.get()); }
		int changes() const                       { return sqlite3_changes(db.get()); }
		int total_changes() const                 { return sqlite3_total_changes(db.get()); }

		int limit(int id, int newVal)                  { return sqlite3_limit(db.get(), id, newVal); }
		int extended_result_codes(bool onoff)          { return sqlite3_extended_result_codes(db.get(), onoff); }
		bool enable_load_extension(bool onoff)         { return sqlite3_enable_load_extension(db.get(), onoff) != 0; }
		int get_autocommit() const                     { return sqlite3_get_autocommit(db.get()); }

		const char * /*db_*/filename(const char * zDbName = nullptr) const    { return sqlite3_db_filename(db.get(), zDbName); }
		bool /*db_*/readonly(const char * zDbName = nullptr) const            { return sqlite3_db_readonly(db.get(), zDbName) != 0; }

		//hooks
		//int (* hook)(Type * userArg)
		template <class Type>
		void * commit_hook(int (* hook)(Type *), Type * arg)
		{
			return sqlite3_commit_hook(db.get(),
				reinterpret_cast<int (*)(void *)>(hook), arg);
		}

		//void (* hook)(Type * userArg)
		template <class Type>
		void * rollback_hook(void (* hook)(Type *), Type * arg)
		{
			return sqlite3_rollback_hook(db.get(),
				reinterpret_cast<void (*)(void *)>(hook), arg);
		}

		//void (* hook)(Type * userArg, int operation, char const * dbname, char const * tablename, sqlite3_int64 rowid)
		template <class Type>
		void * update_hook(void (* hook)(Type *, int, char const *, char const *, sqlite3_int64),
		                   Type * arg)
		{
			return sqlite3_update_hook(db.get(),
				reinterpret_cast<void (*)(void *, int, char const *, char const *, sqlite3_int64)>(hook),
				arg);
		}

	private:
		void check_result(int code)
		{
			if (code != SQLITE_OK)
				throw sqlite_exterror(code, db.get());
		}

		void check_result_ex(int code)
		{
			if (code != SQLITE_OK && code != SQLITE_BUSY && code != SQLITE_LOCKED)
				throw sqlite_exterror(code, db.get());
		}
	};

	/*
		SQLITE_API int sqlite3_db_config(sqlite3*, int op, ...);
		SQLITE_API void *sqlite3_trace(sqlite3*, void(*xTrace)(void*,const char*), void*);
				
		SQLITE_API sqlite3_stmt *sqlite3_next_stmt(sqlite3 *pDb, sqlite3_stmt *pStmt);
		
		SQLITE_API int sqlite3_overload_function(sqlite3*, const char *zFuncName, int nArg);
		SQLITE_API sqlite3_mutex *sqlite3_db_mutex(sqlite3*);
		SQLITE_API int sqlite3_file_control(sqlite3*, const char *zDbName, int op, void*);
		SQLITE_API int sqlite3_db_status(sqlite3*, int op, int *pCur, int *pHiwtr, int resetFlg);
		SQLITE_API int sqlite3_wal_autocheckpoint(sqlite3 *db, int N);
		SQLITE_API int sqlite3_wal_checkpoint(sqlite3 *db, const char *zDb);
		SQLITE_API int sqlite3_vtab_config(sqlite3*, int op, ...);
		SQLITE_API int sqlite3_vtab_on_conflict(sqlite3 *);
	*/
}
