#include <iostream>
#include "ntp_client.h" 

//#pragma warning(disable:4996)                       // disable CRT secure warnings

//#define IP          "195.113.144.201"               // NTP server ip <tik.cesnet.cz>
#define INTERVAL    1000                            // interval of NTP server query

using namespace NTP_client;

static constexpr char CLOUDFLARE_TIME_IP[] = "162.159.200.123";

int main()
{	
	char ntp_server_ip[20]{};                                
	strcpy_s( ntp_server_ip, CLOUDFLARE_TIME_IP ); 

	Client client;

	bool is_running = true;

	while (is_running)
	{
		try
		{
			uint32_t epoch_time = client.GetEpochTime( ntp_server_ip );

			if (epoch_time > 0)
			{
				std::cout << epoch_time << '\n';
			}

			Sleep( INTERVAL );
		}
		catch (const std::exception& e)
		{
			std::cout << e.what() << '\n';
			is_running = false;
		}
	}

	system( "pause" );
}

