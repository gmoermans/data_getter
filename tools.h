#ifndef __TOOLS_H__
#define __TOOLS_H__

#include <string>
#include <vector>
#include <mosquittopp.h>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include <memory>
#include "influxdb.hpp"
/**
 * {Holds every third part methods}
 */
namespace tools
{
	/**
	 * @brief      { function_description }
	 *
	 * @param[in]  strs       The strs
	 * @param[in]  separator  The separator
	 *
	 * @return     { description_of_the_return_value }
	 */
	std::vector<std::string> split(std::string strs, int separator );

	/**
	 * @brief      Splits a by string.
	 *
	 * @param[in]  strs       The strs
	 * @param[in]  separator  The separator
	 *
	 * @return     { description_of_the_return_value }
	 */
	std::vector<std::string> split_by_string(std::string strs, std::string separator );


	/**
	 * @brief      Prints a message.
	 *
	 * @param[in]  message  The message
	 */
	void print_message( const struct mosquitto_message *message );

	/**
	 * @brief      { function_description }
	 *
	 * @param[in]  message  The message
	 *
	 * @return     { description_of_the_return_value }
	 */
	Json::Value json_parser( std::string );

	/**
	 * @brief      record event into log file as logs/logs.txt
	 *
	 * @param[in]  event  { the event to write }
	 * @param[in]  type       'e' in case of error 'm' in case of messge 'w' in case of warning unknow type is stored as warnings.
	 */
	void logs( std::string event, char type );
	void logs( const char * msg, char type );

	long long int time_to_timestamp( std::string emitter_time );
}

#endif
