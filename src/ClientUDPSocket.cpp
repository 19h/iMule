//
// This file is part of the imule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "ClientUDPSocket.h"	// Interface declarations

#include <protocol/Protocols.h>
#include <protocol/ed2k/Client2Client/TCP.h> // Sometimes we reply with TCP packets.
#include <protocol/ed2k/Client2Client/UDP.h>
#include <protocol/kad2/Client2Client/UDP.h>
#include <common/EventIDs.h>
#include <common/Format.h>	// Needed for CFormat

#include "Preferences.h"		// Needed for CPreferences
#include "PartFile.h"			// Needed for CPartFile
#include "updownclient.h"		// Needed for CUpDownClient
#include "UploadQueue.h"		// Needed for CUploadQueue
#include "Packet.h"				// Needed for CPacket
#include "SharedFileList.h"		// Needed for CSharedFileList
#include "DownloadQueue.h"		// Needed for CDownloadQueue
#include "Statistics.h"			// Needed for theStats
#include "amule.h"				// Needed for theApp
#include "ClientList.h"			// Needed for clientlist (buddy support)
#include "ClientTCPSocket.h"	// Needed for CClientTCPSocket
#include "MemFile.h"			// Needed for CMemFile
#include "Logger.h"
#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/utils/KadUDPKey.h"
#include "zlib.h"
#include "EncryptedDatagramSocket.h"

#include <i2p/CI2PAddress.h>

//
// CClientUDPSocket -- Extended eMule UDP socket
//

//#define __PACKET_RECV_DUMP__




CClientUDPSocket::CClientUDPSocket(wxString destkey, const CProxyData* ProxyData)
        : CMuleUDPSocket(wxT("Client UDP-Socket"), /*ID_CLIENTUDPSOCKET_EVENT, */destkey, ProxyData)

{
}


void CClientUDPSocket::OnReceive(int errorCode)
{
        CMuleUDPSocket::OnReceive(errorCode);

        // TODO: A better solution is needed.
        if (thePrefs::IsUDPDisabled()) {
                Close();
        }
}


void CClientUDPSocket::OnPacketReceived(const CI2PAddress& ip, byte* buffer, size_t length)
{
        wxCHECK_RET(length >= 2, wxT("Invalid packet."));

        uint8_t *decryptedBuffer;
        uint32_t receiverVerifyKey;
        uint32_t senderVerifyKey;
        int packetLen = CEncryptedDatagramSocket::DecryptReceivedClient(buffer, length, &decryptedBuffer, ip, &receiverVerifyKey, &senderVerifyKey);

        uint8_t protocol = decryptedBuffer[0];
        uint8_t opcode	 = decryptedBuffer[1];

        if (packetLen >= 1) {
                try {
                        switch (protocol) {
                        case OP_EMULEPROT:
                                ProcessPacket(decryptedBuffer + 2, packetLen - 2, opcode, ip);
                                break;

                        case OP_KADEMLIAHEADER:
                                theStats::AddDownOverheadKad(length);
                                if (packetLen >= 2) {
                                        Kademlia::CKademlia::ProcessPacket(decryptedBuffer, packetLen, ip, (Kademlia::CPrefs::GetUDPVerifyKey(ip) == receiverVerifyKey), Kademlia::CKadUDPKey(senderVerifyKey, theApp->GetPublicDest(false)));
                                } else {
                                        throw wxString(wxT("Kad packet too short"));
                                }
                                break;

                        case OP_KADEMLIAPACKEDPROT:
#ifdef __PACKET_RECV_DUMP__
                                printf("Paquet compressé reçu : opcode = %x taille = %d\n", opcode, length - 2);
                                DumpMem(buffer + 2, length - 2);
#endif
                                theStats::AddDownOverheadKad(length);
                                if (packetLen >= 2) {
                                        uint32_t newSize = packetLen * 10 + 300; // Should be enough...
                                        std::vector<uint8_t> unpack(newSize);
                                        uLongf unpackedsize = newSize - 2;
                                        uint16_t result = uncompress(&(unpack[2]), &unpackedsize, decryptedBuffer + 2, packetLen - 2);
                                        if (result == Z_OK) {
                                                AddDebugLogLineN(logClientKadUDP, wxT("Correctly uncompressed Kademlia packet"));
                                                unpack[0] = OP_KADEMLIAHEADER;
                                                unpack[1] = opcode;
                                                Kademlia::CKademlia::ProcessPacket(&(unpack[0]), unpackedsize + 2, ip, (Kademlia::CPrefs::GetUDPVerifyKey(ip) == receiverVerifyKey), Kademlia::CKadUDPKey(senderVerifyKey, theApp->GetPublicDest(false)));
                                        } else {
#ifdef __PACKET_RECV_DUMP__
                                                printf("Failed to uncompress Kademlia packet\n");
                                                wxASSERT( false ) ;
#endif
                                                AddDebugLogLineN(logClientKadUDP, wxT("Failed to uncompress Kademlia packet"));
                                        }
                                } else {
                                        throw wxString(wxT("Kad packet (compressed) too short"));
                                }
                                break;

                        default:
                                AddDebugLogLineN(logClientUDP, CFormat(wxT("Unknown opcode on received packet: 0x%x")) % protocol);
                        }
                } catch (const wxString& e) {
                        AddDebugLogLineN(logClientUDP, CFormat(wxT("Error while parsing UDP packet from %s: %s")) % ip.humanReadable() % e);
                } catch (const CInvalidPacket& e) {
                        AddDebugLogLineN(logClientUDP, CFormat(wxT("Invalid UDP packet encountered from %s: %s")) %  ip.humanReadable() % e.what());
                } catch (const CEOFException& e) {
                        AddDebugLogLineN(logClientUDP, CFormat(wxT("Malformed packet encountered while parsing UDP packet: ")) %  ip.humanReadable() % e.what());
                }
        }
}


void CClientUDPSocket::ProcessPacket(byte* packet, uint16 size, int8 opcode, const CI2PAddress & dest)
{
        switch (opcode) {
        case OP_REASKFILEPING: {
                AddDebugLogLineN( logClientUDP, wxT("Client UDP socket: OP_REASKFILEPING") );
                theStats::AddDownOverheadFileRequest(size);

                CMemFile data_in(packet, size);
                CMD4Hash reqfilehash = data_in.ReadHash();
                CKnownFile* reqfile = theApp->sharedfiles->GetFileByID(reqfilehash);
                bool bSenderMultipleIpUnknown = false;
                CUpDownClient* sender = theApp->uploadqueue->GetWaitingClientByIP_UDP(dest/*, true, &bSenderMultipleIpUnknown*/);

                if (!reqfile) {
                        std::unique_ptr<CPacket> response(new CPacket(OP_FILENOTFOUND,0,OP_EMULEPROT));
                        theStats::AddUpOverheadFileRequest(response->GetPacketSize());
                        if (sender) {
                                SendPacket(std::move(response), dest, sender->ShouldReceiveCryptUDPPackets(), sender->GetUserHash().GetHash(), false, 0);
                        } else {
                                SendPacket(std::move(response), dest, false, NULL, false, 0);
                        }

                        break;
                }

                if (sender) {
                        sender->CheckForAggressive();

                        //Make sure we are still thinking about the same file
                        if (reqfilehash == sender->GetUploadFileID()) {
                                sender->AddAskedCount();
                                sender->SetUDPDest(dest);
                                sender->SetLastUpRequest();
                                // iMule used this in its first version-> no GetUDPVersion check here
                                sender->ProcessExtendedInfo(&data_in, reqfile);
                                CMemFile data_out(128);
                                if (reqfile->IsPartFile()) {
                                        ((CPartFile*)reqfile)->WritePartStatus(&data_out);
                                } else {
                                        data_out.WriteUInt64(0); //! write parts count
                                }

                                data_out.WriteUInt16(sender->GetUploadQueueWaitingPosition());
                                std::unique_ptr<CPacket> response(new CPacket(data_out, OP_EMULEPROT, OP_REASKACK));
                                theStats::AddUpOverheadFileRequest(response->GetPacketSize());
                                AddDebugLogLineN( logClientUDP, wxT("Client UDP socket: OP_REASKACK to ") + sender->GetFullIP());
                                SendPacket(std::move(response), dest, sender->ShouldReceiveCryptUDPPackets(), sender->GetUserHash().GetHash(), false, 0);
                        } else {
                                AddDebugLogLineN( logClientUDP, wxT("Client UDP socket; ReaskFilePing; reqfile does not match") );
                        }
                } else {
                        if (!bSenderMultipleIpUnknown) {
                                if ((theStats::GetWaitingUserCount() + 50) > thePrefs::GetQueueSize()) {
                                        std::unique_ptr<CPacket> response(new CPacket(OP_QUEUEFULL,0,OP_EMULEPROT));
                                        theStats::AddUpOverheadFileRequest(response->GetPacketSize());
                                        SendPacket(std::move(response),dest, false, NULL, false, 0); // we cannot answer this one encrypted since we dont know this client
                                }
                        } else {
                                AddDebugLogLineN(logClientUDP, CFormat(wxT("UDP Packet received - multiple clients with the same IP but different UDP port found. Possible UDP Portmapping problem, enforcing TCP connection. Dest: %s")) % dest.humanReadable());
                        }
                }
                break;
        }
        case OP_QUEUEFULL: {
                AddDebugLogLineN( logClientUDP, wxT("Client UDP socket: OP_QUEUEFULL") );
                theStats::AddDownOverheadOther(size);
                CUpDownClient* sender = theApp->downloadqueue->GetDownloadClientByDest_UDP(dest);
                if (sender) {
                        sender->SetRemoteQueueFull(true);
                        sender->UDPReaskACK(0);
                }
                break;
        }
        case OP_REASKACK: {
                theStats::AddDownOverheadFileRequest(size);
                CUpDownClient* sender = theApp->downloadqueue->GetDownloadClientByDest_UDP(dest);
                if (sender) {
                        CMemFile data_in(packet,size);
                        sender->ProcessFileStatus(true, &data_in, sender->GetRequestFile());

                        uint16 nRank = data_in.ReadUInt16();
                        sender->SetRemoteQueueFull(false);
                        sender->UDPReaskACK(nRank);
                }
                break;
        }
        case OP_FILENOTFOUND: {
                AddDebugLogLineN( logClientUDP, wxT("Client UDP socket: OP_FILENOTFOUND") );
                theStats::AddDownOverheadFileRequest(size);
                CUpDownClient* sender = theApp->downloadqueue->GetDownloadClientByDest_UDP(dest);
                if (sender) {
                        sender->UDPReaskFNF(); // may delete 'sender'!
                        sender = NULL;
                }
                break;
        }
        default:
                theStats::AddDownOverheadOther(size);
        }
}
// File_checked_for_headers