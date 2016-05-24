#pragma once
#include <sqlite3yaw/session.hpp>
#include <sqlite3yaw/statement.hpp>

namespace sqlite3yaw
{
	class session_handle
	{
		session ses;
	public:
		session_handle() {}
		~session_handle() { ses.release(); }

		session_handle(sqlite3 * db) : ses(db) {}
		session_handle(session_handle const & copy) : ses(copy.ses.native()) {}
		session_handle(session & s) : ses(s.native()) {}
		
		session_handle & operator =(session_handle const & copy) { ses.assign(copy.ses.native()); return *this; }
		session_handle & operator = (session & s)                { ses.assign(s.native()); return *this; }

		session & operator *()  { return ses; }
		session * operator ->() { return &ses; }
	};

	class statement_handle
	{
		statement stmt;

	public:
		statement_handle() {}
		~statement_handle() { stmt.reset(); }

		statement_handle(sqlite3_stmt * stmt) : stmt(stmt) {}
		statement_handle(statement_handle const & copy) : stmt(copy.stmt.native()) {}
		statement_handle(statement & op) : stmt(op.native()) {}
		
		statement_handle & operator =(statement_handle const & copy) { stmt.assign(copy.stmt.native()); return *this; }
		statement_handle & operator =(statement & op)                { stmt.assign(op.native()); return *this; }

		statement & operator *()  { return stmt; }
		statement * operator ->() { return &stmt; }
	};
}