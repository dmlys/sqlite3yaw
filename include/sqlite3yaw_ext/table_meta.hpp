#pragma once
#include <string>
#include <vector>
#include <ext/strings/aci_string.hpp>
#include <sqlite3yaw/session.hpp>

namespace sqlite3yaw
{
	/// traits class describing meta strings(column name, table name, etc, case-insensitive)
	typedef ext::aci_char_traits  metastr_traits;

	struct field_meta
	{
		std::string name;
		std::string type;                      ///sqlite column type as in create table statement
		std::string def_val;
	};

	struct table_meta
	{
		std::string table_name;
		std::vector<field_meta> fields;       /// table columns in order of real placement
		std::string pk;                       /// primary key column name, if missing - empty
	};
	
	///loads column list from session.
	///table must exists, otherwise sqlite3yaw::sqlite_error will be thrown
	void load_table_fields(session & ses, table_meta & meta);
	///loads information about table by table_name
	///table must exists, otherwise sqlite3yaw::sqlite_error will be thrown
	table_meta load_table_meta(session & ses, const std::string & table_name);
	///loads list of table metadata from session
	std::vector<table_meta> load_session_meta(session & ses);
}
