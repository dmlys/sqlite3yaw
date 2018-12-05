#pragma once
#include <utility>
#include <sqlite3yaw/session.hpp>
#include <sqlite3yaw/exceptions.hpp>

namespace sqlite3yaw
{
	enum transaction_type
	{
	    def/*default*/, deferred, immediate, exclusive
	};

	template <transaction_type type>
	class base_transaction
	{
		sqlite3yaw::session * ses;
		bool commited;

	private:
		void begin();

	public:
		      auto & session()       noexcept { return *ses; }
		const auto & session() const noexcept { return *ses; }

	public:
		void rollback()                  { ses->exec("rollback"); }
		void nothrow_rollback() noexcept { ses->exec_ex("rollback"); }
		void commit();

	public:
		base_transaction(sqlite3yaw::session & ses_);
		~base_transaction() noexcept;

		base_transaction(base_transaction && tr) noexcept;
		base_transaction & operator =(base_transaction && tr) noexcept;

		base_transaction(base_transaction const &) = delete;
		base_transaction & operator =(base_transaction const &) = delete;
	};

	template <transaction_type type>
	inline base_transaction<type>::base_transaction(base_transaction && tr) noexcept
		: ses(std::exchange(tr.ses, nullptr)), commited(tr.commited) {}

	template <transaction_type type>
	inline base_transaction<type>::base_transaction(sqlite3yaw::session & ses_)
		: ses(&ses_), commited(false)
	{
		begin();
	}

	template <transaction_type type>
	inline base_transaction<type>::~base_transaction() noexcept
	{
		if (ses && !commited)
			nothrow_rollback();
	}

	template <transaction_type type>
	inline void base_transaction<type>::commit()
	{
		ses->exec("COMMIT");
		commited = true;
	}

	template <>
	inline
	void base_transaction<def>::begin()
	{
		ses->exec("begin");
	}

	template <>
	inline
	void base_transaction<deferred>::begin()
	{
		ses->exec("begin defrred");
	}

	template <>
	inline
	void base_transaction<immediate>::begin()
	{
		ses->exec("begin immediate");
	}

	template <>
	inline
	void base_transaction<exclusive>::begin()
	{
		ses->exec("begin exclusive");
	}

	typedef base_transaction<def> transaction;
	typedef base_transaction<deferred> deferred_transaction;
	typedef base_transaction<immediate> immediate_transaction;
	typedef base_transaction<exclusive> exclusive_transaction;
}
