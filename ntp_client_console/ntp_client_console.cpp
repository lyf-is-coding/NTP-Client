#include <iostream>
#include "ntp_client.h" 

//#pragma warning(disable:4996)                       // disable CRT secure warnings

//#define IP          "195.113.144.201"               // NTP server ip <tik.cesnet.cz>
#define INTERVAL    1000                            // interval of NTP server query

static constexpr char CLOUDFLARE_TIME_IP[] = "162.159.200.123";

int main()
{	
	NTPClient::Client client;

	bool is_running = true;
	while (is_running)
	{
		try
		{
			if (uint32_t unix_time = client.GetUnixTime( CLOUDFLARE_TIME_IP );
				 unix_time > 0)
			{
				auto opt_ymd = client.ExtractYearMonthDay( unix_time );
				if (opt_ymd.has_value())
				{
					std::chrono::year_month_day ymd = std::move( *opt_ymd );

					std::cout << "Unix time " << unix_time << '\n';
					std::cout << "Year " << (int)ymd.year() << '\n';
					std::cout << "Month " << (unsigned)ymd.month() << '\n';
					std::cout << "Day " << (unsigned)ymd.day() << '\n';
				}
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

