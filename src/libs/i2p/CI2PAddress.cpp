//
// C++ Implementation: CI2PAddress
//
// Description:
//
//
// Author: MKVore <mkvore@mail.i2p>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "CI2PAddress.h"

const char CI2PAddress::INVALID_TAG 		= 0;
const char CI2PAddress::VALID_TAG 		= 1<<0;
const char CI2PAddress::ALIAS_TAG 		= 1<<1;
const char CI2PAddress::COMPLETE_FILE_TAG 	= 1<<2;
const char CI2PAddress::INCOMPLETE_FILE_TAG 	= 1<<3;

const CI2PAddress CI2PAddress::null(INVALID_TAG, true);
const CI2PAddress CI2PAddress::complete_file	(COMPLETE_FILE_TAG, true);
const CI2PAddress CI2PAddress::incomplete_file	(INCOMPLETE_FILE_TAG, true);

/* destinations is made of a PUBLIC_KEY + SIGNING_PUBLIC_KEY + CERTIFICATE */
#define PUBLIC_KEY_KEYSIZE_BYTES                256
#define SIGNING_PUBLIC_KEY_KEYSIZE_BYTES        128
// from i2p.i2p/core/java/src/net/i2p/data/Certificate.java
// /** Specifies a null certificate type with no payload */
// static int CERTIFICATE_TYPE_NULL = 0;
// /** specifies a Hashcash style certificate */
// static int CERTIFICATE_TYPE_HASHCASH = 1;
// /** we should not be used for anything (don't use us in the netDb, in tunnels, or tell others about us) */
// static int CERTIFICATE_TYPE_HIDDEN = 2;
// /** Signed with 40-byte Signature and (optional) 32-byte hash */
// static int CERTIFICATE_TYPE_SIGNED = 3;
// static int CERTIFICATE_LENGTH_SIGNED_WITH_HASH = Signature.SIGNATURE_BYTES + Hash.HASH_LENGTH;
// /** Contains multiple certs */
// static int CERTIFICATE_TYPE_MULTIPLE = 4;

/* => The last 3 bytes of a destination have to be 0 in iMule : (#384 = certificate type, #385,#386 = certificate length)
 * Otherwise, the key cannot be accepted because I2P expects more than 387 bytes in the destination key */
