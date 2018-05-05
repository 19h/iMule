//
// This file is part of the imule Project.
//
// Copyright (c) 2005-2011 aMule Team ( admin@amule.org / http://www.amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef MULEUDPSOCKET_H
#define MULEUDPSOCKET_H


#include "Types.h"				// Needed for uint16_t and uint32_t
#include "ThrottledSocket.h"	// Needed for ThrottledControlSocket
#include "i2p/CI2PAddress.h"

#include <wx/event.h>
#include <wx/socket.h>
#include "MuleThread.h"

#include <memory>

class CEncryptedDatagramSocket;
class CProxyData;
class CPacket;

/***
 * This class provides a UBT governed UDP-socket.
 *
 * The CMuleUDPSocket are created with the NOWAIT option and
 * handle both INPUT and OUTPUT events.
 *
 * The following additional features are provided compared to CDatagramSocketProxy:
 *  - Goverened by the UBT.
 *  - Automatic sending/receiving of packets.
 *  - Fallover recovery for when a socket becomes invalid (error 4).
 *
 * @see ThrottledControlSocket
 * @see CEncryptedDatagramSocket
 */
class CMuleUDPSocket : public ThrottledControlSocket, public wxObject
{

public:
        /**
         * Opens a UDP socket at the specified address.
         *
         * @param name Name used when logging events.
         * @param id The ID used for events.
         * @param address The address where the socket will listen.
         * @param ProxyData ProxyData assosiated with the socket.
         */
        CMuleUDPSocket(const wxString& name/*, int id*/, wxString destKeyName,
                       const CProxyData* ProxyData = NULL);

        bool GetLocal(CI2PAddress &);

        wxString GetPrivKey();

        /**
         * Destructor, safely closes the socket if opened.
         */
        virtual ~CMuleUDPSocket();

        virtual bool Destroy();

        /**
         * Opens the socket.
         *
         * The socket is bound to the address specified in
         * the constructor.
         */
        void Open();

        /**
         * Closes the socket.
         *
         * The socket can be reopened by calling Open.
         */
        void Close();


        /** This function is called by aMule when the socket may send. */
        virtual void OnSend(int errorCode);
        /** This function is called by aMule when there are data to be received. */
        virtual void OnReceive(int errorCode);
        /** This function is called by aMule when there is an error while receiving. */
        virtual void OnReceiveError(int errorCode, const CI2PAddress &);
        /** This function is called when the socket is lost (see comments in func.) */
        virtual void OnDisconnected(int errorCode);

        /**
         * Queues a packet for sending.
         *
         * @param packet The packet to send.
         * @param IP The target IP address.
         * @param port The target port.
         * @param bEncrypt If the packet must be encrypted
         * @param port The target port.
         * @param pachTargetClientHashORKadID The client hash or Kad ID
         * @param bKad
         * @param nReceiverVerifyKey
         *
         * Note that CMuleUDPSocket takes ownership of the packet.
         */
        void    SendPacket(std::unique_ptr<CPacket> packet, const CI2PAddress & dest, bool bEncrypt, const uint8* pachTargetClientHashORKadID, bool bKad, uint32_t nReceiverVerifyKey);

        /**
         * Returns true if the socket is Ok, false otherwise.
         *
         * @see wxSocketBase::Ok
         */
        bool	Ok();

        /// Socket data management
        void SetClientData( void *data );
        void SetEventHandler( wxEvtHandler& handler, int id = -1 );
        void SetNotify( wxSocketEventFlags flags );
        void Notify( bool notify );

        /// Returns the name of the socket
        wxString GetName() { return m_name; }

protected:
        /**
         * This function is called when a packet has been received.
         *
         * @param addr The address from where data was received.
         * @param buffer The data that has been received.
         * @param length The length of the data buffer.
         */
        virtual void OnPacketReceived(const CI2PAddress & addr, byte* buffer, size_t length) = 0;


        /** See ThrottledControlSocket::SendControlData */
        SocketSentBytes  SendControlData(uint32 maxNumberOfBytesToSend, uint32 minFragSize);

        static const uint32_t & maxPacketDataSize();
private:
        /**
         * Sends a packet to the specified address.
         *
         * @param buffer The data to be sent.
         * @param length the length of the data buffer.
         * @param ip The target ip address.
         * @param port The target port.
         */
        bool	SendTo(uint8_t *buffer, uint32_t length, const CI2PAddress & addr);


        /**
         * Creates a new socket.
         *
         * Calling this function when a socket already exists
         * is an illegal operation.
         */
        void	CreateSocket();

        /**
         * Destroys the current socket, if any.
         */
        void	DestroySocket();


        //! Specifies if the last write attempt would cause the socket to block.
        bool					m_busy;
        //! The name of the socket, used for debugging messages.
        wxString				m_name;
        //! The socket-ID, used for event-handling.
        //         int						m_id;
        //! The address at which the socket is currently bound.
        wxString				m_privKey;
        //! Proxy settings used by the socket ...
        const CProxyData*		m_proxy;
        //! Mutex needed due to the use of the UBT.
        wiMutex					m_mutex;
        //! The currently opened socket, if any.
        CEncryptedDatagramSocket*	m_socket;

        //! Storage struct used for queueing packets.
        struct UDPPack {
                //! The packet, which at this point is owned by CMuleUDPSocket.
                std::unique_ptr<CPacket>	packet;
                //! The timestamp of when the packet was queued.
                uint32		time;
                //! Target destination.
                CI2PAddress dest;
                //! If the packet is encrypted.
                bool	bEncrypt;
                //! Is it a kad packet?
                bool	bKad;
                // The verification key for RC4 encryption.
                uint32 nReceiverVerifyKey;
                // Client hash or kad ID.
                uint8 pachTargetClientHashORKadID[16];
        } ;

        //! The queue of packets waiting to be sent.
        std::list<UDPPack> m_queue;
        /// Socket data
        void *             m_socket_data ;
        wxEvtHandler *     m_socket_handler;
        int                m_socket_id;
        wxSocketEventFlags m_socket_flags;
        bool               m_socket_notify ;
};

#endif // CLIENTUDPSOCKET_H
// File_checked_for_headers
