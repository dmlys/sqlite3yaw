#include <sqlite3yaw.hpp>
#include <sqlite3yaw_ext/table_meta.hpp>
#include <sqlite3yaw_ext/util.hpp>
#include <functional>

namespace sqlite3yaw
{
	void load_table_fields(session & ses, table_meta & meta)
	{
		if (meta.table_name.empty())
			throw std::invalid_argument("meta.table_name is empty");
		
		meta.pk.clear();

		std::string cmd = "PRAGMA table_info(";
		sqlite3yaw::escape_sql_name(meta.table_name, std::back_inserter(cmd));
		cmd += ")";
		
		auto stmt = ses.prepare(cmd);
		while (stmt.step()) {
			field_meta fm;
			sqlite3yaw::get(stmt, "name", fm.name);
			sqlite3yaw::get(stmt, "type", fm.type);
			sqlite3yaw::get(stmt, "dflt_value", fm.def_val);
			
			std::string pk;
			sqlite3yaw::get(stmt, "pk", pk);
			
			meta.fields.push_back(std::move(fm));
			if (pk == "1")
				meta.pk = meta.fields.back().name;
		}
		stmt.finalize();
	}

	table_meta load_table_meta(session & ses, std::string const & table_name)
	{
		table_meta meta;
		meta.table_name = table_name;
		load_table_fields(ses, meta);
		return meta;
	}

	std::vector<table_meta> load_session_meta(session & ses)
	{
		std::vector<table_meta> meta;
		auto stmt = ses.prepare("select name from sqlite_master where type = 'table'");

		while (stmt.step()) {
			table_meta tableInfo;
			sqlite3yaw::get(stmt, 0, tableInfo.table_name);
			meta.push_back(tableInfo);
		}
		stmt.finalize();
		
		using namespace std::placeholders;
		std::for_each(meta.begin(), meta.end(),
			std::bind(load_table_fields, std::ref(ses), _1));

		return meta;
	}
}