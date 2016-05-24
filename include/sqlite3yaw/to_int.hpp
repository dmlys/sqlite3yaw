#pragma once
#include <sqlite3yaw/config.hpp>

#ifdef SQLITE3YAW_TO_INT_USE_NOTHING
	#ifdef SQLITE3YAW_TO_INT
		#error "SQLITE3YAW_TO_INT_USE_NOTHING and SQLITE3YAW_TO_INT defined both"
	#else
		#define SQLITE3YAW_TO_INT(val) val
	#endif
#endif


#ifdef SQLITE3YAW_TO_INT_USE_BOOST
	#ifdef SQLITE3YAW_TO_INT
		#error "SQLITE3YAW_TO_INT_USE_BOOST and SQLITE3YAW_TO_INT defined both"
	#else
		#include <boost/numeric/conversion/cast.hpp>
		#define SQLITE3YAW_TO_INT(val) boost::numeric_cast<int>(val)
	#endif
#endif



#ifdef SQLITE3YAW_TO_INT_USE_STATIC_CAST
	#ifdef SQLITE3YAW_TO_INT
		#error "SQLITE3YAW_TO_INT_USE_STATIC_CAST and SQLITE3YAW_TO_INT defined both"
	#else
		#define SQLITE3YAW_TO_INT(val) static_cast<int>(val)
	#endif
#endif

namespace sqlite3yaw
{
	inline int ToInt(std::size_t val)
	{
		return SQLITE3YAW_TO_INT(val);
	}
}