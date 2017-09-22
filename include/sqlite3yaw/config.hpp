#pragma once

/**
	define one of follows, default SQLITE3YAW_TO_INT_USE_NOTHING
	
	* SQLITE3YAW_TO_INT_USE_NOTHING
	  size_t converted to int implicitly -> #define SQLITE3YAW_TO_INT(val) val
	
	* SQLITE3YAW_TO_INT_USE_BOOST
	  conversion performed by boost::numeric_cast<int>(val) -> #define SQLITE3YAW_TO_INT(val) boost::numeric_cast<int>(val)

	* SQLITE3YAW_TO_INT_USE_STATIC_CAST
	  conversion performed by static_cast<int>(val) -> #define SQLITE3YAW_TO_INT(val) static_cast<int>(val)
*/

/**
	SQLITE3YAW_TO_INT(val)
	is used for conversion from size_t to int.
	Implement it by your own, or use one of implementations by defining:
		* SQLITE3YAW_TO_INT_USE_NOTHING
		* SQLITE3YAW_TO_INT_USE_BOOST
		* SQLITE3YAW_TO_INT_USE_STATIC_CAST
	
	example: #define SQLITE3YAW_TO_INT(val) static_cast<int>(val)
*/

#if !(defined(SQLITE3YAW_TO_INT) || defined(SQLITE3YAW_TO_INT_USE_NOTHING) || \
	defined(SQLITE3YAW_TO_INT_USE_BOOST) || defined(SQLITE3YAW_TO_INT_USE_STATIC_CAST))
#define SQLITE3YAW_TO_INT_USE_STATIC_CAST
#endif