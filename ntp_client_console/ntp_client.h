/*
Example:
	Client client;
	if (uint32_t unix_time = client.GetUnixTime( CLOUDFLARE_TIME_IP );
		unix_time > 0)
	{
		std::cout << unix_time << '\n';
	}
*/

#pragma once

#ifndef NTP_CLIENT_H
#define NTP_CLIENT_H

#include <chrono>
#include <string>
#include <iostream>
#include <string>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsdkver.h>
#include <assert.h>
#include <WS2tcpip.h>
#include <WinSock2.h>

#pragma comment( lib, "Ws2_32.lib" )
//Mswsock.lib
//AdvApi32.lib

#define NTP_PACKET_LEN		48
#define UDP_PORT			123
#define RX_TIMEOUT			5000
#define JITTER_WINDOW		100

#define STR_MAX_SIZE		100
#define NTP_TS_DELTA		2208988800ull

/*
Below is a description of the NTP / SNTP Version 4 message format,
which follows the IPand UDP headers.This format is identical to
that described in RFC - 1305, with the exception of the contents of the
reference identifier field.The header fields are defined as follows :

1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| LI | VN | Mode | Stratum | Poll | Precision |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| Root Delay |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| Root Dispersion |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| Reference Identifier |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|                   Reference Timestamp(64)                     |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|                   Originate Timestamp(64)                     |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|                    Receive Timestamp(64)                      |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|                    Transmit Timestamp(64)                     |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| Key Identifier(optional) (32) |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|                                                               |
|                 Message Digest(optional) (128)                |
|                                                               |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Reference Timestamp : This is the time at which the local clock was
last set or corrected, in 64 - bit timestamp format.

Originate Timestamp : This is the time at which the request departed
the client for the server, in 64 - bit timestamp format.

Receive Timestamp : This is the time at which the request arrived at
the server, in 64 - bit timestamp format.

Transmit Timestamp : This is the time at which the reply departed the
server for the client, in 64 - bit timestamp format.


 * NTP uses two fixed point formats.  The first (l_fp) is the "long"
 * format and is 64 bits long with the decimal between bits 31 and 32.
 * This is used for time stamps in the NTP packet header (in network
 * byte order) and for internal computations of offsets (in local host
 * byte order). We use the same structure for both signed and unsigned
 * values, which is a big hack but saves rewriting all the operators
 * twice. Just to confuse this, we also sometimes just carry the
 * fractional part in calculations, in both signed and unsigned forms.
 * Anyway, an l_fp looks like:
 *
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |                         Integral Part                         |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |                         Fractional Part                       |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * REF http://www.eecis.udel.edu/~mills/database/rfc/rfc2030.txt

	offset = [(T2 - T1) + (T3 - T4)] / 2
	delay  =  (T4 - T1) - (T3 - T2)

 */


namespace NTPClient
{
	struct ResultEx
	{
		uint32_t UnixTime;		
	};

	enum class QueryStatus : int16_t
	{
		OK = 0,
		UNKNOWN_ERR = 1,
		INIT_WINSOCK_ERR = 2,
		CREATE_SOCKET_ERR = 3,
		SEND_MSG_ERR = 4,
		RECEIVE_MSG_ERR = 5,
		RECEIVE_MSG_TIMEOUT = 6,
		SET_WIN_TIME_ERR = 7,
		ADMIN_RIGHTS_NEEDED = 8
	};

	struct Packet				 // Total: 384 bits or 48 bytes.
	{
		uint8_t li_vn_mode;      //  8 bits. li, vn, and mode.
		//    -li   2 bits. Leap indicator.
		//    -vn   3 bits. Version number of the protocol. 3,4
		//    -mode 3 bits. Client will pick mode 3 for client.

		uint8_t stratum;         //  8 bits. Stratum level of the local clock.
		uint8_t poll;            //  8 bits. Maximum interval between successive messages.
		uint8_t precision;       //  8 bits. Precision of the local clock.
		uint32_t rootDelay;      // 32 bits. Total round trip delay time.
		uint32_t rootDispersion; // 32 bits. Max error aloud from primary clock source.

		uint32_t refId;          // 32 bits. Reference clock identifier.
		uint32_t refTm_s;        // 32 bits. Reference time-stamp seconds.
		uint32_t refTm_f;        // 32 bits. Reference time-stamp fraction of a second.

		uint32_t origTm_s;       // 32 bits. Originate time-stamp seconds. = t1
		uint32_t origTm_f;       // 32 bits. Originate time-stamp fraction of a second.

		uint32_t rxTm_s;         // 32 bits. Received time-stamp seconds. = t2
		uint32_t rxTm_f;         // 32 bits. Received time-stamp fraction of a second.

		uint32_t txTm_s;         // 32 bits. Transmit time-stamp seconds. = t3
		uint32_t txTm_f;         // 32 bits. Transmit time-stamp fraction of a second.
	};

	class Client
	{
	public:
		~Client();

		static std::optional<std::chrono::year_month_day> ExtractYMD( uint32_t unix_time );

		QueryStatus QueryNTPServer( const char* hostname, ResultEx* result_out );
		uint32_t GetUnixTime( const char* ntp_server_ip );

	private:

		QueryStatus Initialize( const char* hostname );
		QueryStatus Query();
		const char* GetQueryStatusString( QueryStatus status ) const;
		
		template<typename ...Args>
		void Log( const char* method, Args&&... args ) const
		{
			std::cout << "[NTPClient] [" << method << "] ";
			((std::cout << args << ' '), ...) << '\n';
		}

		WSADATA WSData;
		sockaddr_in SocketAddress;
		SOCKET Socket;
		int slen;

		Packet QueryPacket;
		char SendingPacket[NTP_PACKET_LEN];
		char ReceivedPacket[NTP_PACKET_LEN];

		char NTPServerIP[STR_MAX_SIZE];

		uint32_t UnixTime;

		const char status_s[9][50] = { "OK", "Unknown Error", "Init Winsock Err", "Create Socket Err", "Tx Message Err", "Rx Msg Err", "Rx Msg Timeout", "Set Win Time Err", "Admin Rights Needed" };
	};
}

#endif