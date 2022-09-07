#include <iostream>
#include "ntp_client.h" 

//#pragma warning(disable:4996)                       // disable CRT secure warnings

//#define IP          "195.113.144.201"               // NTP server ip <tik.cesnet.cz>
#define IP          "162.159.200.123"               // NTP server ip <tik.cesnet.cz>
#define INTERVAL    1000                            // interval of NTP server query

using namespace NTP_client;

static constexpr char CLOUDFLARE_TIME_IP[] = "162.159.200.123";

int main()
{
	ResultEx query_result{};
	char status_str[100];                           // status string from enum container 
	
	char ntp_server_ip[20]{};                                
	strcpy_s( ntp_server_ip, CLOUDFLARE_TIME_IP ); 
	QueryStatus query_status;

	auto client = std::make_unique<Client>();

	bool is_running = true;

	while (is_running)
	{
		try
		{
			query_status = client->QueryNTPServer( ntp_server_ip, &query_result );

			Client_get_status_str( query_status, status_str );              // convert status enum to str
			printf( "\nStatus: %s\n", status_str );               // print status string

			if (query_status == QueryStatus::OK)
			{                              // if status ok, print info
				//Client_format_info_str( &query_result, result_str );    // format info into string
				printf( "%d", query_result.EpochTime );                       // print all important info
				Sleep( INTERVAL );                                    // sleep for specified time
			}
			else
			{
				is_running = false;
			}
		}
		catch (const std::exception& e)
		{
			std::cout << e.what() << '\n';
			is_running = false;
		}
	}

	system( "pause" );
}

