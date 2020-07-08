#include "mqtt.h"
#include "tools.h"
#include <iostream>
#include <pqxx/pqxx>
#include <sstream>
#include <algorithm>
#include <pthread.h>
#include <thread>
#include <unistd.h>
#include <semaphore.h>

mqtt_listner::mqtt_listner(const char *id, const char *host, int port ) : mosquittopp(id)
{
    int keepalive = DEFAULT_KEEP_ALIVE;
    connect_async(host, port, keepalive);
}

mqtt_listner::~mqtt_listner()
{
	
}

bool mqtt_listner::exists( std::string dev_eui )
{
	pqxx::connection dbconnection( "dbname = chirpstack_as user = postgres password = dbpassword hostaddr = 127.0.0.1 port = 5432" );
	try
	{
		if( !dbconnection.is_open() )
		{
			//error
			std::cout<<"database not opend"<<std::endl;
			return false;
		}
		std::vector<std::string> stock;
		//prepare nontransaction object
		pqxx::nontransaction n(dbconnection);
		//querry a list of every device eui
		std::string dev_querry = "select dev_eui from device where dev_eui = \'\\x"+dev_eui+"\';";
		pqxx::result dbdevices( n.exec( dev_querry ) );
		n.commit();

		std::string  ctrl;
		if(dbdevices.begin() == dbdevices.end())
		{
			//querry result is empty...
			return false;
		}
		for( auto row : dbdevices )
		{
			ctrl = row["dev_eui"].as<std::string>();
		}
		//just in case...
		if(ctrl.substr(2,16).compare(dev_eui)==0)
		{
			dbconnection.disconnect();
			return true;
		}
	}
	catch(const std::exception &e)
	{
		std::cout<<"Error on database opening"<<std::endl;
		dbconnection.disconnect();
		return false;
	}
	//??
	dbconnection.disconnect();
	return false;
}

void mqtt_listner::on_connect(int rc)
{
    if (!rc)
    {
		std::string c = "Connected - code "+std::to_string(rc);
		tools::logs(c, 'm');
		subscribe(NULL, MQTT_TOPIC);
    }
}

void mqtt_listner::on_subscribe(int mid, int qos_count, const int *granted_qos)
{
	//std::cout << "Subscription succeeded." << std::endl;
}

int mqtt_listner::send_to_influx_from_json( influxdbconnect_t conninfo, Json::Value object, std::string deveui)
{
	//sending control
	int ret;
	std::string resp;


	//data, time has to be reworked to match with size_t and influx format.
	std::string longtime = object["time"].asString();//time has just been extracted from json object
	long long rt = tools::time_to_timestamp( longtime );
	std::cout<<"time control into json send : " << rt << std::endl;
	if( rt < 0 )
	{
		//error on format ( must define error values later )
		return 13;
	}
	//if this object dont come from from a concentrator, values must be explicitly written in object,
	//then there is no fields.
	//prepare server info ( same for both concentrator or non concentrator )
	influxdb_cpp::server_info si(conninfo.addr, conninfo.port, conninfo.db_name, conninfo.usr, conninfo.psw);

	if( object["fields"].isNull() )
	{
		//server info
		ret = influxdb_cpp::builder()
			.meas(object["measurement"].asString())
			.tag("device",deveui)
			.field("value", object["value"].asFloat(), 4)
			.timestamp(rt)
			.post_http(si, &resp);
		std::cout<< "ret from json non-concentrator(json) "<< ret <<std::endl;
		std::cout<< "rest from json non-concentrator(json) "<< resp << std::endl;
		return ret;
	}

	//delete this, no longer usefull
	float val = object["fields"]["value"].asFloat();
	std::string device = object["tags"]["device"].asString();
	std::string vname = object["measurement"].asString();

	//add control for concentrator checking deveui, if
	std::cout<< "values from concentrator "<< std::endl;
	std::cout<< "server info : "<<std::endl;
	std::cout<< "addr        : "<<conninfo.addr<<std::endl;
	std::cout<< "port        : "<<conninfo.port<<std::endl;
	std::cout<< "dbname      : "<<conninfo.db_name<<std::endl;
	std::cout<< "parameters of data to send" << std::endl;
	std::cout<< "vtype : float" << std::endl;
	std::cout<< "vname : " << vname <<std::endl;
	std::cout<< "rt    : " << rt << std::endl;
	std::cout<< "central: " <<deveui<<std::endl;
	std::cout<< "device : " << device << std::endl;
	std::cout<< "Value : " << val <<std::endl;
	
	ret = influxdb_cpp::builder()
		.meas(object["measurement"].asString())
		.tag("central",deveui)
		.tag("device",object["tags"]["device"].asString())
		.field("value", object["fields"]["value"].asFloat(), 4)
		.timestamp(rt)
		.post_http(si, &resp);

	std::cout<< "ret from concentrator :" << ret <<std::endl;
	std::cout<< "resp from concentretor:"<<resp << std::endl;
	return ret;
}

template <typename T>
int mqtt_listner::send_to_influx( influxdbconnect_t conninfo, datainfo_t datainfo, T data )
{
	/*Load evry parameters data*/

	//sending control
	int ret;
	std::string resp;

	//server info
	influxdb_cpp::server_info si(conninfo.addr, conninfo.port, conninfo.db_name, conninfo.usr, conninfo.psw);

	//datas
	std::string vtype = datainfo.vtype;
	std::string vname = datainfo.vname;
	std::string deveui = datainfo.deveui;
	long long int rt = datainfo.rt;

	//add control for concentrator checking deveui, if

	std::cout<< "server info : "<<std::endl;
	std::cout<< "addr        : "<<conninfo.addr<<std::endl;
	std::cout<< "port        : "<<conninfo.port<<std::endl;
	std::cout<< "dbname      : "<<conninfo.db_name<<std::endl;
	std::cout<< "parameters of data to send" << std::endl;
	std::cout<< "vtype : " << vtype << std::endl;
	std::cout<< "vname : " << vname << std::endl;
	std::cout<< "rt    : " << rt << std::endl;
	std::cout<< "deveui: " << deveui<<std::endl;
	std::cout<< "Value : " << data <<std::endl;
	
	/*Check the type of value to send and send as influxdb request (doc into influxdb.hpp)
	  builder is the object that send request to influxdb*/
	if( vtype.compare("float") == 0 )
	{
		float val = static_cast<float>(data);
		ret = influxdb_cpp::builder()
			.meas(vname)
			.tag("device",deveui)
			.field("value", val, 4)
			.timestamp(rt)
			.post_http(si, &resp);
	}
	else if( vtype.compare("integer") == 0 )
	{
		int val = static_cast<int>(data);
		ret = influxdb_cpp::builder()
			.meas(vname)
			.tag("device",deveui)
			.field("value", val)
			.timestamp(rt)
			.post_http(si, &resp);
	}
	else
	{
		std::string val = std::to_string(data);
		ret = influxdb_cpp::builder()
			.meas(vname)
			.tag("device",deveui)
			.field("value", val)
			.timestamp(rt)
			.post_http(si, &resp);
	}
	//if the data has not been send correctly, send it to local database
	return ret;
}

template<typename T>
std::string mqtt_listner::influx_string( datainfo_t datainfo, T data )
{
	std::string db_save = "{\"measurement\": \""+datainfo.vname+"\", \"time\": "+std::to_string(datainfo.rt);
	db_save+="{\"deveui\": \""+datainfo.deveui+"\", \"value\": "+std::to_string(data)+"}";
	return db_save;
}

int mqtt_listner::send_to_backup_db( std::string value )
{
	pqxx::connection dbconnection( "dbname = chirpstack_as user = postgres password = dbpassword hostaddr = 127.0.0.1 port = 5432" );
	dbconnection.prepare("insert_object","INSERT INTO backup_db(content) VALUES($1)");
	pqxx::work w(dbconnection);
	w.prepared("insert_object")(value).exec();
	w.commit();
	return 0;
}

int mqtt_listner::send_to_backup_db( Json::Value value )
{
	Json::FastWriter fw;
	std::string backup = fw.write(value);
	pqxx::connection dbconnection( "dbname = chirpstack_as user = postgres password = dbpassword hostaddr = 127.0.0.1 port = 5432" );
	dbconnection.prepare("insert_object","INSERT INTO backup_db(content) VALUES($1)");
	pqxx::work w(dbconnection);
	w.prepared("insert_object")(backup).exec();
	w.commit();
	return 0;
}
/*Add more verifications here*/
//Using this method ONY when you are sure that basic is pareable.
Json::Value json_from_string( std::string basic )
{
	Json::Value object;
	JSONCPP_STRING err;
	Json::CharReaderBuilder builder;
	std::unique_ptr<Json::CharReader> reader( builder.newCharReader() );
	if( !reader->parse( basic.c_str(), basic.c_str() + basic.size(), &object, &err ) )
	{
		
	}
	return object;
}
Json::Value mqtt_listner::if_json( std::string basic )
{
	std::cout<<"Basic in if_json : "<< basic <<std::endl;
	Json::Value object;
	//prepare the error object value
	Json::Value err_json = json_from_string("{\"error\": true}");
	//error handling
	JSONCPP_STRING err;
	//jsoncpp object in order to build object from string
	Json::CharReaderBuilder builder;
	std::unique_ptr<Json::CharReader> reader( builder.newCharReader() );

	//if the object has fail in parse, reader will return false
	bool jp = false;
	try
	{
		jp = reader->parse( basic.c_str(), basic.c_str() + basic.size(), &object, &err );
	}
	catch(Json::LogicError& e)
	{
		std::cout<< e.what() << std::endl;
		return err_json;
	}
	if(!jp)
	{
		return err_json;
	}
	//object has been parsed
	//Now control the if the format match with server preconditions
	//if the object dont contains measurement
	try
	{
		if( object["time"].isNull() || object["measurement"].isNull() )
		{
			return err_json;
		}
		//if object is not a concentrator
		if( object["fields"].isNull() || object["tags"].isNull() )
		{
			//is size of non centrator dont match
			if( object.size() != 4 || object["value"].isNull())
			{
				return err_json;
			}
			return object;
		}
		//if concetrator dont have right values.
		else
		{
			if( object["fields"]["value"].isNull() || object["fields"].size() != 1 )
			{
				return err_json;
			}
			else if( object["tags"]["device"].isNull() || object["tags"].size() != 1 )
			{
				return err_json;
			}
		}
		return object;
	}
	catch( Json::Exception& e )
	{
		std::cout<<"Exception : " << e.what() << "parsing json"<< std::endl;
		return err_json;
	}
}

std::string mqtt_listner::server_to_influx( std::string meas, std::string deveui, std::string value, std::string time )
{
	std::string jstring = "{\"measurement\": \""+meas+"\", \"tags\": {\"device\": \""+deveui+"\"},\"time\": \""+time+"\", \"fields\": {\"value\": "+value+"}}";
	return jstring;
}

bool first_control( Json::Value object )
{
	return true;
}
void mqtt_listner::on_message(const struct mosquitto_message *message)
{
	//basic will hold the string inside message as his " basic form "
	std::string basic( static_cast<const char *>(message->payload),  message->payloadlen );
	std::string substring;
	bool done = false;
	bool utf_8 = true;

	//all the 20 next lines are functionnal but has to be changed for more correctness
	for( unsigned i = 0; i < basic.size() && !done && utf_8; ++i )
	{	
		if( basic.at(i) < 0 || basic.at(i) > 127 )
		{
			utf_8 = false;
		}
		else
		{
			//remove the head of message (unreadable)
			//this stuf is functionnal but has to be changed cause really ugly
			if( basic.at(i) == 0x7b ) //this is really weak
			{
				substring = basic.substr(i, basic.size()-1);
				done=true;
			}
		}
	}
	//If this object is supposed to be parsable to json without crash
	if(done)
	{		
		JSONCPP_STRING err;
		Json::Value object;
		Json::CharReaderBuilder builder;
		std::unique_ptr<Json::CharReader> reader( builder.newCharReader() );
		bool jp = false;
		//This message is parsable to json object due to server specifications
		try
		{
			 jp = reader->parse( basic.c_str(), basic.c_str() + basic.size(), &object, &err );
		}
		catch(Json::Exception& e)
		{
			std::cout<< e.what() << std::endl;
			return;
		}
		if( jp )
		{
			if( object["devEUI"].isNull() )
			{
				//this object dont come from server.
				return;
			}
			if( object["data"].isNull() )
			{
				//this object is probably a join request, this is no concern here
				return;
			}
			//look if the device is authorized to connect into server and if it contains data.
			if( exists( object["devEUI"].asString() ) )
			{
				//if the device exists, then, all meta data must be present.		
				//payload meta-data
				std::string deveui = object["devEUI"].asString();
				std::string devname = object["deviceName"].asString();
				std::string data = object["data"].asString();
				std::string gateui = object["rxInfo"][0]["gatewayID"].asString();
				std::string app_name = object["applicationName"].asString();
				
				std::cout<<"Having message from "<<deveui<<" : "<<devname<<std::endl;
				/*Here, we have the app name then, must load influxdb database connect here*/
				std::string addr;
				std::string dbname;
				int port;
				std::string usr;
				std::string psw;
			
				//connectio to chirpstack database, where every connection informations are stored ( local declaration for thread safety )
				pqxx::connection dbconnection( "dbname = chirpstack_as user = postgres password = dbpassword hostaddr = 127.0.0.1 port = 5432" );
				if(!dbconnection.is_open())
				{
					tools::logs("database not opennend", 'e');
					return;
				}

				//Querry to select connection data to the database where the device has to send his data.
				std::string querry = "select addr, port, usr, psw from db_connect where id = ( select db from application_br where label = \'"+app_name+"\' );";
				pqxx::nontransaction n(dbconnection);
				pqxx::result qr( n.exec( querry ) );
				n.commit();

				//sytaxic sugar ( has to be changed ) auto qr dont work
				for( auto row : qr )
				{
					addr = row["addr"].as<std::string>();
					port = row["port"].as< int >();
					usr = row["usr"].as<std::string>();
					psw = row["psw"].as<std::string>();
				}

				//declareation of data information that will be send to influxdb
				std::string vname;
				std::string vtype;
				long long int record_time;
				std::string deveui_end;
				float val;

				//convert data from message ( base 64 code )
				std::string np = base64_decode( data );

				std::vector<std::string> all_data = tools::split(np, 0x3b);
				//if all data is of size 1, the json formatting has been used
				//WHY ?
				//if the server protocal has been used, the payloads must containst at least timestamp and one data
				//if a payload only contains timestamp ( for some reasons ) it will be processed as an json object.
				//and this json object will be flagged as an error.
				if(all_data.size() < 2)
				{
					Json::Value testJson = if_json( np );
					if( !testJson["error"].isNull() )
					{
						//an error occurs while parsing message into json object
						std::cout<<"error on json parsing(main)"<<std::endl;
						dbconnection.disconnect();
						return;
					}
					//For concentrator device change the dbname and device eui for influx
					dbname = app_name;
					influxdbconnect_t ic(addr, port, dbname, usr, psw);

					influxdb_mutex.lock();
						int ret = send_to_influx_from_json( ic, testJson, devname);//watch for devname - > deveui
					influxdb_mutex.unlock();

					if( ret != 0 )
					{
						std::cout<<"object non send to influxdb "<< std::endl;
						send_to_backup_db( testJson );
						//store to local db cause the send to influxdb fails
					}
					dbconnection.disconnect();
					return;
				}

				//Must have "1554686443;1-123;2-0.1235;3-456[...]"
				std::string unix_epoch = all_data.at(0);//get timestamp as string
				record_time = tools::time_to_timestamp( unix_epoch );
				if( record_time < 0 )
				{
					std::cout<< "time format is wrong" << std::endl;
					dbconnection.disconnect();
					return;
				}
				/*Control time format here ?*/

				std::cout<<"Record time : "<< record_time << std::endl;

				//all data from 1 contain every datas send by end node using serveur format
				for( unsigned i = 1; i < all_data.size(); ++i )
				{
					std::vector< std::string > pair = tools::split(all_data.at(i), 0x3d);//[id, value]
					if( pair.size() == 2 )
					{
						//recover type and name with id and application.
						std::string querry = "SELECT value_label, value_type from values_ where appname = '"+app_name+"' AND value_id = "+pair.at(0)+";";

						//here, we must control the querry 
						pqxx::nontransaction n(dbconnection);
						pqxx::result qr( n.exec( querry ) );
						n.commit();
						//in case we use multiple result ( only one query for the moment )
						std::string vname;
						std::string vtype;

						for( auto row : qr )
						{
							vname  = row["value_label"].as<std::string>();
							vtype = row["value_type"].as<std::string>();
						}
						if( vname.size() < 1 )
						{
							std::cout << "data dont exist " << std::endl;
						}
						else
						{
							//at this point, we have recoverd the name and the type of data. Next step is to send these to influxdb
							/*If more types will be trheaded, insert them here*/
							/*protect stoi to make the server crash*/
							try
							{
								val = stof( pair.at(1) );//value float can handle other value ( used for later if more types are used )
								datainfo_t d(vname, vtype, devname, record_time);
								influxdbconnect_t ic(addr, port, app_name, usr, psw);
								//builder from influxdb is unsafe regards to trheads cause the port used is the same for each
								//instances.
								influxdb_mutex.lock();
									int ret = send_to_influx( ic, d, val);
								influxdb_mutex.unlock();

								std::cout<<"ret value : "<<ret<<std::endl;
								if( ret != 0 )
								{
									std::cout<<"object non send to influxdb "<< std::endl;
									std::string backup = server_to_influx( vname, devname, std::to_string(val), std::to_string(record_time) );
									send_to_backup_db( backup );
									//store to local db cause the send to influxdb fails
								}
							}
							catch( std::invalid_argument& e )
							{
								std::cout<<"error parsing vector element at : "<<i<<std::endl;
								std::cout<<e.what()<<std::endl;
							}
						}
					}
					if( pair.size() == 3 )
					{
						//concentrator form must be 1=1=1 device=data=value
					}
				}
				dbconnection.disconnect();
			}		
		}
		//data is not parsable to json due to server formatiing specification
	}
}
