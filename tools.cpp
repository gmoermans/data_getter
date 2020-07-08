

#include "tools.h"
#include <iostream>
#include <fstream>

namespace tools{

	std::vector<std::string> split(std::string strs, int separator )
	{
		std::vector< std::string > result;
		std::string v ="";
		for( unsigned i = 0; i < strs.size(); i++ )
		{
			if( strs.at(i)== separator && i != strs.size()-1 )
			{
				result.push_back( v );
				v="";
			}
			else
			{
				v = v + strs.at(i);
			}
		}
		result.push_back(v);
		return result;
	}
	std::vector<std::string> split_by_string(std::string strs, std::string separator )
	{
		std::vector<std::string> v;
		return v;
	}
	void logs( const char * msg, char type )
	{
		std::string message(msg);
		time_t rawtime = time(NULL);
		time (&rawtime);
		std::ofstream fs( "logs/logs.txt", std::ios::app );
		if( type == 'e' )
		{
			fs << "[error]="+message+"\t";
		}
		else if ( type == 'w' )
		{
			fs << "[warning]="+message+"\t";
		}
		else
		{
			fs << "[message]="+message+"\t";
		}
		fs << ctime( &rawtime );
		fs.close();
	}
	void logs( std::string message, char type )
	{
		time_t rawtime = time(NULL);
		time (&rawtime);
		std::ofstream fs( "logs/logs.txt", std::ios::app );
		if( type == 'e' )
		{
			fs << "[error]="+message+"\t";
		}
		else if ( type == 'w' )
		{
			fs << "[warning]="+message+"\t";
		}
		else
		{
			fs << "[message]="+message+"\t";
		}
		fs << ctime( &rawtime );
		fs.close();
	}

	long long int time_to_timestamp( std::string emitter_time )
	{
		std::cout<< "Emitter time : " <<emitter_time << std::endl;
		bool long_value = true;
		for( unsigned i = 0; i < emitter_time.size() &&long_value; ++i )
		{
			//if the value is not a digit then this is not a timestamp
			if( emitter_time[i] < 0x30 || emitter_time[i] > 0x39 )
			{
				long_value=false;
			}
		}
		if( long_value )
		{
			try
			{
				long long int ts = stoi( emitter_time );
				//format to milliseconds for influxdb
				if( emitter_time.size() == 10 )
				{
					ts*=1000;
				}
				std::cout << "Ts : " << ts << std::endl;
				return ts;	
			}
			catch( std::invalid_argument& e)
			{
				std::cout<< e.what() << std::endl;
				return -1;
			}
		}
		//cast string 
		struct tm t;
		
		//2020-05-15T30:20:00Z
		try{
			t.tm_year = atoi(emitter_time.substr(0,4).c_str())-1900;
			t.tm_mon = atoi(emitter_time.substr(5,2).c_str())-1;
			t.tm_mday = atoi(emitter_time.substr(8,2).c_str());
			t.tm_hour = atoi(emitter_time.substr(11,2).c_str())-1;
			t.tm_min = atoi(emitter_time.substr(14,2).c_str());
			t.tm_sec = atoi(emitter_time.substr(17,2).c_str());
		}
		catch( std::invalid_argument& e )
		{
			return -1;
		}
	
		time_t rt = mktime(&t);
		std::cout<<"ctime : "<<ctime(&rt) << std::endl;
		std::cout<<rt<<std::endl;
		long long int lt = ( long long int )rt;
		lt*=1000;
		std::cout<< "return value " << std::endl;
		return lt;
	}

}
