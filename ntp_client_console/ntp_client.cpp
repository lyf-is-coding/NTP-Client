#define _CRT_SECURE_NO_WARNINGS

#include "ntp_client.h"

namespace NTP_client
{
	/* Ctor Dtor */

	Client::Client()
	{
		std::cout << "Construct client\n";
	}
	Client::~Client()
	{
		std::cout << "Destruct client\n";
	}

	/*  Public Instance Methods  */

	QueryStatus Client::QueryNTPServer( const char* hostname, ResultEx* result_out )
	{
		if (hostname == nullptr || strlen( hostname ) <= 0)
		{
			return QueryStatus::UNKNOWN_ERR;
		}

		try
		{
			// If hostname changed, reinit
			if (strcmp( this->NTPServerIP, hostname ) != 0)
			{
				QueryStatus init_ret = this->Initialize( hostname );
				strncpy_s( this->NTPServerIP, hostname, strlen( hostname ) );
				std::cout << "Server: " << this->NTPServerIP << '\n';

				if (init_ret != QueryStatus::OK) { return init_ret; }
			}

			QueryStatus ret2 = this->Query();
			if (ret2 != QueryStatus::OK)
			{
				return ret2;
			}

			(*result_out).EpochTime = this->EpochTime;

			return ret2;
		}
		catch (const std::exception& exc)
		{
			std::cout << exc.what();
			return QueryStatus::UNKNOWN_ERR;
		}
	}

	uint32_t Client::GetEpochTime( const char* ntp_server_ip )
	{
		if (ntp_server_ip == nullptr || strlen( ntp_server_ip ) == 0)
		{
			return 0;
		}

		try
		{
			// If hostname changed, reinit
			if (strcmp( this->NTPServerIP, ntp_server_ip ) != 0)
			{
				if (this->Initialize( ntp_server_ip ) != QueryStatus::OK) { return 0; }
			}

			if (this->Query() != QueryStatus::OK)
			{
				return 0;
			}

			return this->EpochTime;
		}
		catch (const std::exception& exc)
		{
			std::cout << exc.what();
			return 0;
		}
	}

	/*  Public Static Methods  */

	void Client::get_status_str( QueryStatus status, char* str )
	{
		auto socket = (unsigned int)status;
		if (socket >= 0 && socket <= 8)
			strcpy( str, status_s[socket] );
	}

	/*  Private Methods  */

	QueryStatus Client::Initialize( const char* hostname )
	{
		strncpy_s( this->NTPServerIP, hostname, strlen( hostname ) );
		std::cout << "Server: " << this->NTPServerIP << '\n';

		this->slen = sizeof( this->SocketAddress );

		std::cout << "Initialising Winsock... \n";
		if (WSAStartup( MAKEWORD( 2, 2 ), &this->wsa ) != 0)
		{
			printf( "Failed.Error Code : %d", WSAGetLastError() );
			return QueryStatus::INIT_WINSOCK_ERR;
		}
		printf( "Initialized.\n" );

		if (this->Socket = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
			 this->Socket == INVALID_SOCKET)
		{
			printf( "socket() failed with error code : %d", WSAGetLastError() );
			return QueryStatus::CREATE_SOCKET_ERR;
		}

		// Setup address structure
		memset( (char*)&this->SocketAddress, 0, sizeof( SocketAddress ) );
		SocketAddress.sin_family = AF_INET;
		SocketAddress.sin_port = htons( UDP_PORT );
		InetPtonA( AF_INET, hostname, &(SocketAddress.sin_addr) );

		int rx_timeout = RX_TIMEOUT;
		::setsockopt( this->Socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&rx_timeout, sizeof( rx_timeout ) );

		return QueryStatus::OK;
	}

	QueryStatus Client::Query()
	{
		// Init Sending packet
		memset( &this->QueryPacket, '\0', NTP_PACKET_LEN );
		this->QueryPacket.li_vn_mode = 227;			// 11100011 (client mode + NTPv4)
		this->QueryPacket.stratum = 0;				// Stratum level of the local clock
		this->QueryPacket.poll = 4;					// Maximum interval between successive messages
		this->QueryPacket.precision = (uint8_t)-10;	// Precision of the local clock (expressed in power of 2, -10 means 2-10, that is to say 1/1024=0.97ms)
		this->QueryPacket.rootDelay = 256;			// Total round trip delay time
		this->QueryPacket.rootDispersion = 256;		// Max error aloud from primary clock source
		memcpy( this->SendingPacket, &this->QueryPacket, NTP_PACKET_LEN );

		// Init Received packet
		memset( this->ReceivedPacket, '\0', NTP_PACKET_LEN );

		if (::sendto( this->Socket, this->SendingPacket, NTP_PACKET_LEN, 0, (sockaddr*)&this->SocketAddress, this->slen ) == SOCKET_ERROR)
		{
			printf( "sendto() failed with error code : %d", WSAGetLastError() );
			return QueryStatus::SEND_MSG_ERR;
		}

		if (::recvfrom( this->Socket, this->ReceivedPacket, NTP_PACKET_LEN, 0, (sockaddr*)&this->SocketAddress, &this->slen ) == SOCKET_ERROR)
		{
			printf( "recvfrom() failed with error code : %d", WSAGetLastError() );
			if (WSAGetLastError() == WSAETIMEDOUT)
				return QueryStatus::RECEIVE_MSG_TIMEOUT;
			return QueryStatus::RECEIVE_MSG_ERR;
		}

		// Copy result to Query packet
		memcpy( &this->QueryPacket, this->ReceivedPacket, NTP_PACKET_LEN );

		// Converts u_long result from TCP/IP network order to host byte order
		this->QueryPacket.txTm_s = ntohl( this->QueryPacket.txTm_s );

		this->EpochTime = this->QueryPacket.txTm_s - NTP_TS_DELTA; // Calculate Epoch time in second

		return QueryStatus::OK;
	}

	//QueryStatus Client::set_win_clock( time_point_t tm ) // sysinfoapi.h
	//{
	//	SYSTEMTIME st;
	//	//GetSystemTime(&gt);

	//	int y, m, d, h, mm, sec, ms, us, ns;
	//	Client::extract_time_point( tm, y, m, d, h, mm, sec, ms, us, ns );

	//	st.wYear = (WORD)y;
	//	st.wMonth = (WORD)m;
	//	st.wDay = (WORD)d;
	//	st.wHour = (WORD)h;
	//	st.wMinute = (WORD)mm;
	//	st.wSecond = (WORD)sec;
	//	st.wMilliseconds = (WORD)ms;

	//	auto ret = SetLocalTime( &st );

	//	if (ret)
	//	{
	//		return QueryStatus::OK;
	//	}
	//	else
	//	{
	//		auto err = GetLastError();
	//		if (err == 1314)
	//			return QueryStatus::ADMIN_RIGHTS_NEEDED;
	//		else
	//			return QueryStatus::SET_WIN_TIME_ERR;
	//	}
	//}
}
