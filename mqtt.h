#ifndef SIMPLECLIENT_MQTT_H
#define SIMPLECLIENT_MQTT_H
#include "tools.h"
#include "base64.h"
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <mutex>

#define MAX_PAYLOAD 50
#define DEFAULT_KEEP_ALIVE 60
#define MQTT_TOPIC "#"
#define LOOP_TIMEOUT 180
#define SLEEPER 1
#define REL 150

/**
 * @brief      { hold connection datas to influx db in order to have shorter parameters list }
 */
struct influxdbconnect_t{
	std::string addr;
	int port;
	std::string db_name;
	std::string usr;
	std::string psw;
	influxdbconnect_t(std::string addr, int port, std::string db_name, std::string usr, std::string psw) :
	addr(addr), port(port), db_name(db_name), usr(usr), psw(psw){}
};

/**
 * @brief      { hold data informations in order to have shorter parameters list}
 */
struct datainfo_t{
	std::string vname;
	std::string vtype;
	std::string deveui;
	long long int rt;
	datainfo_t(std::string vname, std::string vtype, std::string deveui, long long int rt) :
	vname(vname), vtype(vtype), deveui(deveui), rt(rt){}
};

/**
 * @brief      Mqtt_listner is an extension of mosquitto broker, used in order to sort payloads from chirpstack server
 * 			   and send them to influxdb and local database.
 */
class mqtt_listner : public mosqpp::mosquittopp
{
    /**
     * { pgsql connection string}
     */
    std::string db_connection_data;
    /**
     * { the port to send data to influxdb is defined into .hpp and will be not modified in this version of program }
     */
    std::mutex influxdb_mutex;
   
public:

    /**
     * @brief      Constructs a new instance.
     *
     * @param[in]  id    The identifier
     * @param[in]  host  The host
     * @param[in]  port  The port
     */
    mqtt_listner (const char *id, const char *host, int port );
    /**
     * @brief      Destroys the object.
     */
    ~mqtt_listner();


    /**
     * @brief      Called on connect.
     *
     * @param[in]  rc    The rectangle
     */
    void on_connect(int rc);

    /**
     * @brief      on_message is the main method of the program, once a paquet is emitted on MQTT, it will be processed
		   in this method.
     *
     * @param[in]  message  The message of suscripted topic ( "#" in this case )
     */
    void on_message(const struct mosquitto_message *message);

    /**
     * @brief      Called on subscribe.
     *
     * @param[in]  mid          The middle
     * @param[in]  qos_count    The qos count
     * @param[in]  granted_qos  The granted qos
     */
    void on_subscribe(int mid, int qos_count, const int *granted_qos);
    
    /**
     * @brief      check if the device is knew by the serveur ( this must be useless, this is just another layer of security )
     *
     * @param[in]  dev_eui  The dev eui
     *
     * @return     { true if the device is knew }
     */
    bool exists( std::string dev_eui );
    /**
     * @brief      Send data to influx db with given parameters
     *
     * @param[in]  conninfo  informations about destination server
     * @param[in]  datainfo  informations about the data to send
     * @param[in]  data      data to send
     *
     * @tparam     T         { the data can actually be of any type, pratically, the accepted types are integer, float or string if the type is diffrent and have no std::to_string method result is unknow (probably crash) }
     *
     * @return     { return non-zero value if send works }
     */
    template <typename T>
    int send_to_influx( influxdbconnect_t conninfo, datainfo_t datainfo, T data);

    /**
     * @brief      verify if the given parameter can be parsed into json object ( no 100% operational )
     *
     * @param[in]  str     The string
     * @param      object  the string object will be recorded into object parameter if return is false object returned is {"error" : "true"}
     *
     * @return     { true if std can be casted  }
     */
    Json::Value if_json( std::string str );

    template<typename T>
    std::string influx_string( datainfo_t datainfo, T data );

    int check_format( std::string frame );

    int send_to_backup_db( std::string value );
    int send_to_backup_db( Json::Value value );

    std::string server_to_influx( std::string meas, std::string deveui, std::string value, std::string time );

    int send_to_influx_from_json( influxdbconnect_t conninfo, Json::Value object, std::string deveui );
};
#endif //SIMPLECLIENT_MQTT_H
