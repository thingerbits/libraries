
/* Tim Steiner
 * Copyright (c) 2016 , CryptoTrust LLC.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *      
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OnlyKey Project
 *    (http://www.crp.to/ok)"
 *
 * 4. The names "OnlyKey" and "OnlyKey Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    admin@crp.to.
 *
 * 5. Products derived from this software may not be called "OnlyKey"
 *    nor may "OnlyKey" appear in their names without prior written
 *    permission of the OnlyKey Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OnlyKey Project
 *    (http://www.crp.to/ok)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OnlyKey PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OnlyKey PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#include "okcrypto.h"
#include <SoftTimer.h>
#include <cstring>
#include "Arduino.h"
#include "onlykey.h"
#include <SoftTimer.h>
#include <RNG.h>


#if !defined(MBEDTLS_CONFIG_FILE)
#include "config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include "memory_buffer_alloc.h"
#endif

#ifdef US_VERSION

/*************************************/
//RNG Assignments
/*************************************/
size_t length = 48; // First block should wait for the pool to fill up.
/*************************************/
//RSA assignments
/*************************************/
uint8_t rsa_publicN[MAX_RSA_KEY_SIZE];
uint8_t rsa_private_key[MAX_RSA_KEY_SIZE];
/*************************************/
//ECC Authentication assignments
/*************************************/
uint8_t ecc_public_key[(MAX_ECC_KEY_SIZE*2)+1];
uint8_t ecc_private_key[MAX_ECC_KEY_SIZE];
/*************************************/

extern uint8_t Challenge_button1;
extern uint8_t Challenge_button2;
extern uint8_t Challenge_button3;
extern uint8_t CRYPTO_AUTH;
extern int outputU2F;
uint8_t type;
extern int packet_buffer_offset;
extern uint8_t resp_buffer[64];
extern uint8_t large_buffer[1024];
extern uint8_t packet_buffer[768];
extern uint8_t recv_buffer[64];
extern int large_data_len;
extern int msgcount;

void SIGN (uint8_t *buffer) {
	uECC_set_rng(&RNG2); 
	#ifdef DEBUG
	Serial.println();
	Serial.println("OKSIGN MESSAGE RECEIVED"); 
	#endif
	if (buffer[5] < 101) { //Slot 101-132 are for ECC, 1-4 are for RSA
	uint8_t features = onlykey_flashget_RSA ((int)buffer[5]);
	if (type == 0) {
		if (outputU2F) {
			custom_error(3); //no key set in this slot
		}
		return;
	}
	#ifdef DEBUG
	Serial.print(features, BIN);
	#endif
	if (is_bit_set(features, 6)) {
		RSASIGN(buffer);
	} else {
		#ifdef DEBUG
		Serial.print("Error key not set as signature key");
		#endif
		if (!outputU2F) {
			hidprint("Error key not set as signature key");
		} else {
			custom_error(2); //key type not set as signature/decrypt
		}
		return;
	}
	} else {
	uint8_t features = onlykey_flashget_ECC ((int)buffer[5]);
	if (type == 0) {
		if (outputU2F) {
			custom_error(3); //no key set in this slot
		}
		return;
	}
	#ifdef DEBUG
	Serial.print(features, BIN);
	#endif
	if (is_bit_set(features, 6)) {
		ECDSA_EDDSA(buffer);
	} else {
		#ifdef DEBUG
		Serial.print("Error key not set as signature key");
		#endif
		if (!outputU2F) {
			hidprint("Error key not set as signature key");
		} else {
			custom_error(2); //key type not set as signature/decrypt
		}
		return;
	}
	}
}

void GETPUBKEY (uint8_t *buffer) {
	#ifdef DEBUG
	Serial.println();
	Serial.println("OKGETPUBKEY MESSAGE RECEIVED"); 
	#endif
	if (buffer[6] + buffer[7] + buffer[8] + buffer[9] != 0) {
		uint8_t slotnum[1];
		slotnum[0] = GETKEYLABELS(0);
		if (slotnum[0] >= 1) store_U2F_response(slotnum, 1, true);
	}
	else if (buffer[5] < 101 && !outputU2F) { //Slot 101-132 are for ECC, 1-4 are for RSA
		if (onlykey_flashget_RSA ((int)buffer[5])) GETRSAPUBKEY(buffer);
	} else if (!outputU2F) {
		if (onlykey_flashget_ECC ((int)buffer[5])) GETECCPUBKEY(buffer);	
	} 
}

void DECRYPT (uint8_t *buffer){
	uECC_set_rng(&RNG2); 
	#ifdef DEBUG
	Serial.println();
	Serial.println("OKDECRYPT MESSAGE RECEIVED"); 
	#endif
	if (buffer[5] < 101) { //Slot 101-132 are for ECC, 1-4 are for RSA
	uint8_t features = onlykey_flashget_RSA (buffer[5]);
	if (type == 0) {
		if (outputU2F) {
			custom_error(3); //no key set in this slot
		}
		return;
	}
	if (is_bit_set(features, 5)) {
		RSADECRYPT(buffer);
	} else {
		#ifdef DEBUG
		Serial.print("Error key not set as decryption key");
		#endif
		if (!outputU2F) {
			hidprint("Error key not set as decryption key");
		} else {
			custom_error(2); //key type not set as signature/decrypt
		}
		return;
	}
	} else {
	uint8_t features = onlykey_flashget_ECC (buffer[5]);
    if (type == 0) {
		if (outputU2F) {
			custom_error(3); //no key set in this slot
		}
		return;
	}	
	if (is_bit_set(features, 5)) {
		ECDH(buffer);
	} else {
		#ifdef DEBUG
		Serial.print("Error key not set as decryption key");
		#endif
		if (!outputU2F) {
			hidprint("Error key not set as decryption key");
		} else {
			custom_error(2); //key type not set as signature/decrypt
		}
		return;
	}
	}
}

void GENERATE_KEY (uint8_t *buffer) {
	uECC_set_rng(&RNG2); 
	uint8_t backupslot;
	uint8_t temp[64];
	#ifdef DEBUG
	Serial.println();
	Serial.println("GENERATE KEY MESSAGE RECEIVED"); 
	#endif
	onlykey_eeget_backupkey (&backupslot);
	if (buffer[5] > 100) { //Slot 101-132 are for ECC, 1-4 are for RSA
		if ((buffer[6] & 0x0F) == 1) {
			Ed25519::generatePrivateKey(buffer+7);
			if (backupslot == buffer[5]) { //Backup Key
				memcpy(temp, buffer+7, 32);
				RawHID.send(temp, 0);	
			} 
		} else if ((buffer[6] & 0x0F) == 2) {
			const struct uECC_Curve_t * curve = uECC_secp256r1(); //P-256
			uECC_make_key(ecc_public_key, buffer+7, curve); 
			if (backupslot == buffer[5]) { //Backup Key
				memcpy(temp, buffer+7, 32);
				RawHID.send(temp, 0);	
			} 
		} else if ((buffer[6] & 0x0F) == 3) {
			const struct uECC_Curve_t * curve = uECC_secp256k1();
			uECC_make_key(ecc_public_key, buffer+7, curve); 
			if (backupslot == buffer[5]) { //Backup Key
				memcpy(temp, buffer+7, 32);
				RawHID.send(temp, 0);	
			} 
		}
		memset(ecc_public_key, 0, sizeof(ecc_public_key));
	}
	return;
}

void GETRSAPUBKEY (uint8_t *buffer)
{
#ifdef DEBUG
	byteprint(rsa_publicN, (type*128));
#endif
    if (!outputU2F){
	memcpy(resp_buffer, rsa_publicN, 64);
    RawHID.send(resp_buffer, 0);
	delay(100);
	memcpy(resp_buffer, rsa_publicN+64, 64);
    RawHID.send(resp_buffer, 0);
	delay(100);
	}
	if (type>=2 && !outputU2F) {
	memcpy(resp_buffer, rsa_publicN+128, 64);
    RawHID.send(resp_buffer, 0);
	delay(100);
	memcpy(resp_buffer, rsa_publicN+192, 64);
    RawHID.send(resp_buffer, 0);
	delay(100);
	} if (type>=3 && !outputU2F) {
	memcpy(resp_buffer, rsa_publicN+256, 64);
    RawHID.send(resp_buffer, 0);
	delay(100);
	memcpy(resp_buffer, rsa_publicN+320, 64);
    RawHID.send(resp_buffer, 0);
	delay(100);
	} if (type==4 && !outputU2F) {
	memcpy(resp_buffer, rsa_publicN+384, 64);
    RawHID.send(resp_buffer, 0);
	delay(100);
	memcpy(resp_buffer, rsa_publicN+448, 64);
    RawHID.send(resp_buffer, 0);
	delay(100);
	} 
}

void RSASIGN (uint8_t *buffer)
{
	uint8_t rsa_signature[(type*128)];

    if(!CRYPTO_AUTH) process_packets (buffer);
	else if (CRYPTO_AUTH == 4) {
		if (packet_buffer_offset != 28 && packet_buffer_offset != 32 && packet_buffer_offset != 48 && packet_buffer_offset != 64) {
		if (!outputU2F) hidprint("Error with RSA data to sign invalid size");
#ifdef DEBUG
    Serial.println("Error with RSA data to sign invalid size");
	Serial.println(packet_buffer_offset);
#endif
		fadeoff(1);
		packet_buffer_offset = 0;
		memset(packet_buffer, 0, sizeof(packet_buffer)); //wipe buffer
		return;
	}
#ifdef DEBUG
    Serial.println();
    Serial.printf("RSA data to sign size=%d", packet_buffer_offset);
	Serial.println();
	byteprint(packet_buffer, packet_buffer_offset);
#endif
	// sign data in packet_buffer 
    if (rsa_sign (packet_buffer_offset, packet_buffer, rsa_signature) == 0)
	{
#ifdef DEBUG
		Serial.print("Signature = ");
	    byteprint(rsa_signature, sizeof(rsa_signature));
		Serial.println();
#endif
	if (outputU2F==0){
	memcpy(resp_buffer, rsa_signature, 64);
    RawHID.send(resp_buffer, 0);
	delay(100);
	memcpy(resp_buffer, rsa_signature+64, 64);
    RawHID.send(resp_buffer, 0);
	delay(100);
		if (type>=2) {
		memcpy(resp_buffer, rsa_signature+128, 64);
		RawHID.send(resp_buffer, 0);
		delay(100);
		memcpy(resp_buffer, rsa_signature+192, 64);
		RawHID.send(resp_buffer, 0);
		delay(100);
		} if (type>=3) {
		memcpy(resp_buffer, rsa_signature+256, 64);
		RawHID.send(resp_buffer, 0);
		delay(100);
		memcpy(resp_buffer, rsa_signature+320, 64);
		RawHID.send(resp_buffer, 0);
		delay(100);
		} if (type==4) {
		memcpy(resp_buffer, rsa_signature+384, 64);
		RawHID.send(resp_buffer, 0);
		delay(100);
		memcpy(resp_buffer, rsa_signature+448, 64);
		RawHID.send(resp_buffer, 0);
		delay(100);
		} 
	} else if (outputU2F) {
	store_U2F_response(rsa_signature, (type*128), true);
	}
	} else {
		if (!outputU2F) hidprint("Error with RSA signing");
	}
	fadeoff(85);
	memset(rsa_signature, 0, sizeof(rsa_signature));
    return;
	} else {
#ifdef DEBUG
    Serial.println("Waiting for challenge buttons to be pressed");
#endif
	}
}

void RSADECRYPT (uint8_t *buffer)
{
	unsigned int plaintext_len = 0;

    if(!CRYPTO_AUTH) process_packets (buffer);
	else if (CRYPTO_AUTH == 4) {
		if (packet_buffer_offset != (type*128)) {
		if (!outputU2F) hidprint("Error with RSA data to decrypt invalid size");
#ifdef DEBUG
    Serial.println("Error with RSA data to decrypt invalid size");
	Serial.println(packet_buffer_offset);
#endif
		fadeoff(1);
		packet_buffer_offset = 0;
		memset(packet_buffer, 0, sizeof(packet_buffer)); //wipe buffer
		return;
	}
#ifdef DEBUG
    Serial.println();
    Serial.printf("RSA ciphertext blob size=%d", packet_buffer_offset);
	Serial.println();
	byteprint(packet_buffer, packet_buffer_offset);
#endif
	// decrypt ciphertext in packet_buffer to large_buffer
    if (rsa_decrypt (&plaintext_len, packet_buffer, large_buffer) == 0)
	{
#ifdef DEBUG
		Serial.println();
		Serial.print("Plaintext len = ");
		Serial.println(plaintext_len);
		Serial.print("Plaintext = ");
		byteprint(large_buffer, plaintext_len);
		Serial.println();
#endif
    if (outputU2F==0) {
	memcpy(resp_buffer, large_buffer, 64);
    RawHID.send(resp_buffer, 0);
	delay(100);
		if (plaintext_len > 64) {
		memcpy(resp_buffer, large_buffer+64, 64);
		RawHID.send(resp_buffer, 0);
		delay(100);
		} if (plaintext_len > 128) {
		memcpy(resp_buffer, large_buffer+128, 64);
		RawHID.send(resp_buffer, 0);
		delay(100);
		} if (plaintext_len > 192) {
		memcpy(resp_buffer, large_buffer+192, 64);
		RawHID.send(resp_buffer, 0);
		delay(100);
		}  
	} else if (outputU2F) {
	store_U2F_response(large_buffer, plaintext_len, true);
	}
	} else {
		if (!outputU2F) hidprint("Error with RSA decryption");
	}
	fadeoff(85);
    // Reset the buffer offset
	memset(large_buffer, 0, sizeof(large_buffer));
    return;
	} else {
#ifdef DEBUG
    Serial.println("Waiting for challenge buttons to be pressed");
#endif
	}
}

void GETECCPUBKEY (uint8_t *buffer)
{
		onlykey_flashget_ECC (buffer[5]);        
			#ifdef DEBUG
    	    Serial.println("OKGETECCPUBKEY MESSAGE RECEIVED"); 
			byteprint(ecc_public_key, sizeof(ecc_public_key));
	    #endif
            if (!outputU2F) {
			RawHID.send(ecc_public_key, 0);
			}
			memset(ecc_public_key, 0, MAX_ECC_KEY_SIZE*2); //wipe buffer
			memset(ecc_private_key, 0, MAX_ECC_KEY_SIZE); //wipe buffer
         
}

void ECDSA_EDDSA(uint8_t *buffer)
{
	uint8_t ecc_signature[64];
#ifdef DEBUG
    Serial.println();
    Serial.println("OKECDSA_EDDSACHALLENGE MESSAGE RECEIVED"); 
#endif
    if(!CRYPTO_AUTH) process_packets (buffer);
	else if (CRYPTO_AUTH == 4) {

#ifdef DEBUG
    Serial.println();
    Serial.printf("ECC challenge blob size=%d", packet_buffer_offset);
#endif
	uint8_t tmp[32 + 32 + 64];
	SHA256_HashContext ectx = {{&init_SHA256, &update_SHA256, &finish_SHA256, 64, 32, tmp}};
	// Sign the blob stored in the buffer
	if (type==0x01) Ed25519::sign(ecc_signature, ecc_private_key, ecc_public_key, packet_buffer, packet_buffer_offset);
	else if (type==0x02) {
		    const struct uECC_Curve_t * curve = uECC_secp256r1(); //P-256
			uECC_sign_deterministic(ecc_private_key,
						packet_buffer,
						packet_buffer_offset,
						&ectx.uECC,
						ecc_signature,
						curve);
	}
	else if (type==0x03) {
			const struct uECC_Curve_t * curve = uECC_secp256k1(); 
			uECC_sign_deterministic(ecc_private_key,
						packet_buffer,
						packet_buffer_offset,
						&ectx.uECC,
						ecc_signature,
						curve);
	}
#ifdef DEBUG
	    for (uint8_t i = 0; i< sizeof(ecc_signature); i++) {
    	    Serial.print(ecc_signature[i],HEX);
     	    }
#endif
	if (outputU2F) {
	store_U2F_response(ecc_signature, MAX_ECC_KEY_SIZE*2, true); 
	} else{
	RawHID.send(ecc_signature, 0);
	}
    // Stop the fade in
    fadeoff(85);
	memset(ecc_public_key, 0, sizeof(ecc_public_key)); //wipe buffer
	memset(ecc_private_key, 0, sizeof(ecc_private_key)); //wipe buffer
    return;
	} else {
#ifdef DEBUG
    Serial.println("Waiting for challenge buttons to be pressed");
#endif
	}
}


void ECDH(uint8_t *buffer)
{
    uint8_t ephemeral_pub[MAX_ECC_KEY_SIZE*2];
	uint8_t secret[64] = {0};
#ifdef DEBUG
    Serial.println();
    Serial.println("OKECDH MESSAGE RECEIVED"); 
#endif
    if(!CRYPTO_AUTH) process_packets (buffer);
	else if (CRYPTO_AUTH == 4) {
	memcpy (ephemeral_pub, packet_buffer, MAX_ECC_KEY_SIZE*2);
    if (shared_secret(ephemeral_pub, secret)) {
		if (!outputU2F) hidprint("Error with ECC Shared Secret");
		return;
	}
#ifdef DEBUG
    Serial.println();
    Serial.print("Public key to generate shared secret for"); 
	for (int i = 0; i<= MAX_ECC_KEY_SIZE*2; i++) {
		Serial.print(ephemeral_pub[i],HEX);
		}
    Serial.println();
    Serial.print("ECDH Secret is "); 
	for (uint8_t i = 0; i< 32; i++) {
		Serial.print(secret[i],HEX);
		}
#endif
	if (outputU2F) {
	//store_U2F_response(secret, 32); 
	} else{
	//RawHID.send(secret, 0);
	}
	//delay(100);
    // Reference - https://www.ietf.org/mail-archive/web/openpgp/current/msg00637.html
	// https://fossies.org/linux/misc/gnupg-2.1.17.tar.gz/gnupg-2.1.17/g10/ecdh.c
	// gcry_md_write(h, "\x00\x00\x00\x01", 4);      /* counter = 1 */
    // gcry_md_write(h, secret_x, secret_x_size);    /* x of the point X */
    // gcry_md_write(h, message, message_size);      /* KDF parameters */
	// This is a limitation as we have to be able to fit the entire message to decrypt
	// In this way RSA seems to have an advantage?
	// /* Build kdf_params.  */
    //{
    //IOBUF obuf;
    //
    //obuf = iobuf_temp();
    ///* variable-length field 1, curve name OID */
    //err = gpg_mpi_write_nohdr (obuf, pkey[0]);
    ///* fixed-length field 2 */
    //iobuf_put (obuf, PUBKEY_ALGO_ECDH);
    ///* variable-length field 3, KDF params */
    //err = (err ? err : gpg_mpi_write_nohdr (obuf, pkey[2]));
    ///* fixed-length field 4 */
    //iobuf_write (obuf, "Anonymous Sender    ", 20);
    ///* fixed-length field 5, recipient fp */
    //iobuf_write (obuf, pk_fp, 20);
    //
    //message_size = iobuf_temp_to_buffer (obuf, message, sizeof message);
	/*

	uint8_t hash_alg = large_buffer[0];
	uint8_t *pub_key = large_buffer+1;
	uint8_t *msg = large_buffer+1+32;
	uint8_t counter[] = "\x00\x00\x00\x01";
	uint8_t hash[64];
    mbedtls_sha512_context sha512;
	switch (hash_alg) {
		case 2: //sha256
		SHA256_CTX context;
		sha256_init(&context);
		sha256_update(&context, counter, 4); //add counter
		sha256_update(&context, secret, sizeof(secret)); //add secret
		sha256_update(&context, msg, (packet_buffer_offset-1-type)); //add message
		sha256_final(&context, hash); 
		break;
		case 3: //sha384
		mbedtls_sha512_init (&sha512);
		mbedtls_sha512_starts (&sha512, 1); //is 384
		mbedtls_sha512_update (&sha512, counter, 4); //add counter
		mbedtls_sha512_update (&sha512, secret, sizeof(secret)); //add secret
		mbedtls_sha512_update (&sha512, msg, (packet_buffer_offset-1-type)); //add message
		mbedtls_sha512_finish (&sha512, hash); 
		mbedtls_sha512_free (&sha512);
		break;
		case 5: //sha512
		mbedtls_sha512_init (&sha512);
		mbedtls_sha512_starts (&sha512, 0); //is not 384
		mbedtls_sha512_update (&sha512, counter, 4); //add counter
		mbedtls_sha512_update (&sha512, secret, sizeof(secret)); //add secret
		mbedtls_sha512_update (&sha512, msg, (packet_buffer_offset-1-sizeof(secret))); //add message
		mbedtls_sha512_finish (&sha512, hash); 
		mbedtls_sha512_free (&sha512);
		break;
		default:
		hidprint("Error hash algorithm incorrect");
		return;
	}
	//Send the KEK, client app should know the symmetric encryption alg
	//Depending on the alg the client will drop the uneeded tail of the the key
    
#ifdef DEBUG
    Serial.println();
    Serial.print("ECDH KEK is "); 
	for (int i = 0; i< sizeof(hash); i++) {
		Serial.print(hash[i],HEX);
		}
#endif
    RawHID.send(hash, 0);
	*/
    fadeoff(85);
	memset(secret, 0, sizeof(secret)); //wipe buffer
	memset(ecc_public_key, 0, sizeof(ecc_public_key)); //wipe buffer
	memset(ecc_private_key, 0, sizeof(ecc_private_key)); //wipe buffer
    return;
	} else {
#ifdef DEBUG
    Serial.println("Waiting for challenge buttons to be pressed");
#endif
	}
}

int shared_secret (uint8_t *ephemeral_pub, uint8_t *secret) {
	const struct uECC_Curve_t * curve;
	#ifdef DEBUG 
	Serial.printf("Shared Secret for type %X ",type);
	#endif
	switch (type) {
	case 1:
		if (crypto_box_beforenm(secret, ephemeral_pub, ecc_private_key)) return 0;
		else return 1;			
	case 2:
		curve = uECC_secp256r1(); 
		if (uECC_shared_secret(ephemeral_pub, ecc_private_key, secret, curve)) return 0;
		else return 1;	
	case 3:
		curve = uECC_secp256k1(); 
		if (uECC_shared_secret(ephemeral_pub, ecc_private_key, secret, curve)) return 0;
		else return 1;	
	default:
		if (!outputU2F) hidprint("Error ECC type incorrect");
		return 1;
	}
}

void aes_crypto_box (uint8_t *buffer, int len, bool open) { 
	uint8_t iv[12];
	uint8_t temp[32];
	memset(iv, 0, 12);
	msgcount++;
	int ctr = ((msgcount>>24)&0xff) | // move byte 3 to byte 0
	  ((msgcount<<8)&0xff0000) | // move byte 1 to byte 2
	  ((msgcount>>8)&0xff00) | // move byte 2 to byte 1
	  ((msgcount<<24)&0xff000000); // byte 0 to byte 3
	memcpy(iv, &ctr, 4);
	#ifdef DEBUG
	Serial.print("IV");
	byteprint(iv, 12);
	#endif
	#ifdef DEBUG
	Serial.print("Key");
	byteprint(ecc_private_key, 32);
	#endif
	#ifdef DEBUG
	Serial.print("buffer");
	byteprint(buffer, len);
	#endif
	if (open) {
		aes_gcm_decrypt2 (buffer, iv, ecc_private_key, len); 
	}
	else {
		aes_gcm_encrypt2 (buffer, iv, ecc_private_key, len);
	}
}

int rsa_sign (int mlen, const uint8_t *msg, uint8_t *out)
{
	//mbedtls_rsa_self_test(1);
	mbedtls_md_type_t md_type = MBEDTLS_MD_NONE;
	int ret = 0;
	static mbedtls_rsa_context rsa;
    uint8_t rsa_ciphertext[(type*128)];
	mbedtls_mpi P1, Q1, H;
	mbedtls_rsa_init( &rsa, MBEDTLS_RSA_PKCS_V15, 0 );
	
	mbedtls_mpi_init (&P1);  mbedtls_mpi_init (&Q1);  mbedtls_mpi_init (&H);
	rsa.len = (type*128);
	MBEDTLS_MPI_CHK( mbedtls_mpi_lset (&rsa.E, 0x10001) );
	
	MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary (&rsa.P, &rsa_private_key[0], ((type*128) / 2) ));
	MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary (&rsa.Q, &rsa_private_key[((type*128) / 2)], ((type*128) / 2) ));
	MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi (&rsa.N, &rsa.P, &rsa.Q) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_sub_int (&P1, &rsa.P, 1) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_sub_int (&Q1, &rsa.Q, 1) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi (&H, &P1, &Q1) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_inv_mod (&rsa.D , &rsa.E, &H) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi (&rsa.DP, &rsa.D, &P1) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi (&rsa.DQ, &rsa.D, &Q1) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_inv_mod (&rsa.QP, &rsa.Q, &rsa.P) );
	
	#ifdef DEBUG
	Serial.printf( "\nRSA len = " );
	Serial.println(rsa.len);
	#endif
	ret = mbedtls_rsa_check_privkey( &rsa );
	cleanup:
	mbedtls_mpi_free (&P1);  mbedtls_mpi_free (&Q1);  mbedtls_mpi_free (&H);
	
   if( ret != 0 )
    {
		#ifdef DEBUG
	        Serial.printf("Error with key check =%d", ret);
	        #endif
			if (outputU2F) {
				custom_error(4); //invalid key, key check failed
			}
		return -1;
	}
  if (ret == 0)
    {
      #ifdef DEBUG
      Serial.print("RSA sign messege length = ");
	  Serial.println(mlen);
	  #endif
	  if (mlen > ((type*128)-11)) mlen = ((type*128)-11);
		
		switch (mlen) {
		case 64:
			md_type = MBEDTLS_MD_SHA512;
		break;
					
		case 48:
			md_type = MBEDTLS_MD_SHA384;
		break;

		case 32:
			md_type = MBEDTLS_MD_SHA256;
		break;
		
		case 28:
			md_type = MBEDTLS_MD_SHA224;
		break;
			
		//case 20:
		//	md_type = MBEDTLS_MD_RIPEMD160;
		//break;
	
		default:
		break;

		}
	  
      ret = mbedtls_rsa_rsassa_pkcs1_v15_sign (&rsa, mbedtls_rand, NULL, MBEDTLS_RSA_PRIVATE, md_type, mlen, msg, rsa_ciphertext);
      #ifdef DEBUG
      Serial.print("Hash Value = ");
	  byteprint((uint8_t *)msg, mlen);
	  #endif
	  memcpy (out, rsa_ciphertext, (type*128));
	  /*int ret2 = mbedtls_rsa_rsassa_pkcs1_v15_verify ( &rsa, NULL, NULL, MBEDTLS_RSA_PUBLIC, MBEDTLS_MD_NONE, mlen, msg, rsa_ciphertext );
	  #ifdef DEBUG
      Serial.print("Hash Value = ");
	  byteprint((uint8_t *)msg, mlen);
	  #endif
	  if( ret2 != 0 ) {
		  #ifdef DEBUG
		  Serial.print("Signature Verification Failed ");
		  Serial.println(ret2);
		  #endif
		  return -1;
	  }*/
    }
  mbedtls_rsa_free (&rsa);
  if (ret == 0)
    {
    #ifdef DEBUG
    Serial.println("completed successfully");
	#endif
    return 0;
    }
  else
    {
	#ifdef DEBUG
	Serial.print("MBEDTLS_ERR_RSA_XXX error code ");
    Serial.println(ret);
	#endif
	if (outputU2F) {
		custom_error(5); //invalid data, or data does not match  key
	}
    return -1; 
    }
}

int rsa_decrypt (unsigned int *olen, const uint8_t *in, uint8_t *out)
{
  mbedtls_mpi P1, Q1, H;
  int ret = 0;
  static mbedtls_rsa_context rsa;
  #ifdef DEBUG
  Serial.printf ("\nRSA decrypt:");
  Serial.println ((uint32_t)&ret);
  #endif

  mbedtls_rsa_init (&rsa, MBEDTLS_RSA_PKCS_V15, 0);
  mbedtls_mpi_init (&P1);  mbedtls_mpi_init (&Q1);  mbedtls_mpi_init (&H);
  rsa.len = (type*128);
  MBEDTLS_MPI_CHK( mbedtls_mpi_lset (&rsa.E, 0x10001) );
  MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary (&rsa.P, &rsa_private_key[0], ((type*128) / 2) ));
  MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary (&rsa.Q, &rsa_private_key[((type*128) / 2)], ((type*128) / 2) ));
  MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi (&rsa.N, &rsa.P, &rsa.Q) );
  MBEDTLS_MPI_CHK( mbedtls_mpi_sub_int (&P1, &rsa.P, 1) );
  MBEDTLS_MPI_CHK( mbedtls_mpi_sub_int (&Q1, &rsa.Q, 1) );
  MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi (&H, &P1, &Q1) );
  MBEDTLS_MPI_CHK( mbedtls_mpi_inv_mod (&rsa.D , &rsa.E, &H) );
  MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi (&rsa.DP, &rsa.D, &P1) );
  MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi (&rsa.DQ, &rsa.D, &Q1) );
  MBEDTLS_MPI_CHK( mbedtls_mpi_inv_mod (&rsa.QP, &rsa.Q, &rsa.P) );
  #ifdef DEBUG
  Serial.println (rsa.len);
  #endif
  cleanup:
  mbedtls_mpi_free (&P1);  mbedtls_mpi_free (&Q1);  mbedtls_mpi_free (&H);
  
	#ifdef DEBUG
	Serial.printf( "\nRSA len = " );
	Serial.println(rsa.len);
	#endif
  ret = mbedtls_rsa_check_privkey( &rsa ); 
	if (ret != 0) {
	  #ifdef DEBUG
      Serial.print ("MBEDTLS_ERR_RSA_XXX error code ");
	  Serial.println (ret);
	  #endif
	  if (outputU2F) {
		custom_error(4); //invalid key, key check failed
	  }
	}
  if (ret == 0)
    {
	  #ifdef DEBUG
      Serial.print ("RSA decrypt ");
	  #endif
      ret = mbedtls_rsa_rsaes_pkcs1_v15_decrypt (&rsa, NULL, NULL, MBEDTLS_RSA_PRIVATE, olen, in, out, 512);
    }
  mbedtls_rsa_free (&rsa);
  if (ret == 0)
    {
	  #ifdef DEBUG
      Serial.println ("completed successfully");
	  Serial.print (*olen);
	  #endif
      return 0;
    }
  else
    {
	  #ifdef DEBUG
      Serial.print ("MBEDTLS_ERR_RSA_XXX error code ");
	  Serial.println (ret);
	  #endif
	  if (outputU2F) {
		custom_error(5); //invalid data, or data does not match  key
	  }
      return -1;
    }
}


void rsa_getpub (uint8_t type)
{
  mbedtls_mpi P, Q, N;
  int ret = 0;
  #ifdef DEBUG
  Serial.print ("RSA generate public N:");
  #endif
  
  mbedtls_mpi_init (&P);  mbedtls_mpi_init (&Q);  mbedtls_mpi_init (&N);

  MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary (&P, &rsa_private_key[0], ((type*128) / 2) ));
  MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary (&Q, &rsa_private_key[((type*128) / 2)], ((type*128) / 2) ));
  MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi (&N, &P, &Q) );
  MBEDTLS_MPI_CHK( mbedtls_mpi_write_binary (&N, &rsa_publicN[0], (type*128) ));
  cleanup:
  mbedtls_mpi_free (&P);  mbedtls_mpi_free (&Q);  mbedtls_mpi_free (&N);
  
  if (ret == 0)
    {
	  #ifdef DEBUG
      Serial.print ("Storing RSA public ");
	  byteprint(rsa_publicN, (type*128));
	  #endif
    } else {
	  if (!outputU2F) hidprint("Error generating RSA public N");
	  #ifdef DEBUG
      Serial.print ("Error generating RSA public");
	  byteprint(rsa_publicN, (type*128));
	  #endif
	}
}

int rsa_encrypt (int len, const uint8_t *in, uint8_t *out)
{
	mbedtls_mpi P1, Q1, H;
	int ret = 0;
	static mbedtls_rsa_context rsa;
	#ifdef DEBUG
	Serial.printf ("\nRSA encrypt:");
	Serial.println ((uint32_t)&ret);
	#endif

	mbedtls_rsa_init (&rsa, MBEDTLS_RSA_PKCS_V15, 0);
	mbedtls_mpi_init (&P1);  mbedtls_mpi_init (&Q1);  mbedtls_mpi_init (&H);
	rsa.len = (type*128);
	MBEDTLS_MPI_CHK( mbedtls_mpi_lset (&rsa.E, 0x10001) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary (&rsa.P, &rsa_private_key[0], ((type*128) / 2) ));
	MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary (&rsa.Q, &rsa_private_key[((type*128) / 2)], ((type*128) / 2) ));
	MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi (&rsa.N, &rsa.P, &rsa.Q) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_sub_int (&P1, &rsa.P, 1) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_sub_int (&Q1, &rsa.Q, 1) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi (&H, &P1, &Q1) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_inv_mod (&rsa.D , &rsa.E, &H) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi (&rsa.DP, &rsa.D, &P1) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi (&rsa.DQ, &rsa.D, &Q1) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_inv_mod (&rsa.QP, &rsa.Q, &rsa.P) );
	#ifdef DEBUG
	Serial.println (rsa.len);
	#endif
	cleanup:
	mbedtls_mpi_free (&P1);  mbedtls_mpi_free (&Q1);  mbedtls_mpi_free (&H);
  
	#ifdef DEBUG
	Serial.printf( "\nRSA len = " );
	Serial.println(rsa.len);
	#endif
  ret = mbedtls_rsa_check_pubkey( &rsa ); 
	if (ret != 0) {
	  #ifdef DEBUG
      Serial.print ("MBEDTLS_ERR_RSA_XXX error code ");
	  Serial.println (ret);
	  #endif
	  return ret;
	}
  if (ret == 0)
    {

	  ret = mbedtls_rsa_rsaes_pkcs1_v15_encrypt	( &rsa, mbedtls_rand, NULL, MBEDTLS_RSA_PUBLIC, len,
                           in, out );
    }
  mbedtls_rsa_free (&rsa);
  if (ret == 0)
    {
	  #ifdef DEBUG
      Serial.print ("completed successfully");
	  #endif
      return 0;
    }
  else
    {
	  #ifdef DEBUG
      Serial.print ("MBEDTLS_ERR_RSA_XXX error code ");
	  Serial.println (ret);
	  #endif
      return ret;
    }
}


bool is_bit_set(unsigned char byte, int index) {
  return (byte >> index) & 1;
}

int mbedtls_rand( void *rng_state, unsigned char *output, size_t len )
{
	if( rng_state != NULL )
        rng_state = NULL;
    RNG2( output, len );
    return( 0 );
}

int RNG2(uint8_t *dest, unsigned size) {
	// Generate output whenever 32 bytes of entropy have been accumulated.
    // The first time through, we wait for 48 bytes for a full entropy pool.
    while (!RNG.available(length)) {
      //Serial.println("waiting for random number");
	  rngloop(); //Gather entropy
    }
    RNG.rand(dest, size);
    length = 32;
#ifdef DEBUG
	Serial.println();
	Serial.print("Generating random number of size = ");
	Serial.print(size);
	byteprint(dest, size);
#endif
    return 1;
}

void newhope_test ()
{
	int i;
	unsigned long ran;
	char rand[32];
	csprng SRNG,CRNG;
	RAND_clean(&SRNG); 
	RAND_clean(&CRNG);
	char s[1792],sb[1824],uc[2176],keyA[32],keyB[32];

	octet S= {0,sizeof(s),s};
	octet SB= {0,sizeof(sb),sb};
	octet UC= {0,sizeof(uc),uc};
	octet KEYA={0,sizeof(keyA),keyA};
	octet KEYB={0,sizeof(keyB),keyB};
	RNG2((uint8_t*)rand, 32);
	RAND_seed(&SRNG, rand);
	RNG2((uint8_t*)rand, 32);
	RAND_seed(&CRNG, rand);

	// NewHope Simple key exchange

	NHS_SERVER_1(&SRNG,&SB,&S); 
	NHS_CLIENT(&CRNG,&SB,&UC,&KEYB);
	NHS_SERVER_2(&S,&UC,&KEYA);

	Serial.println("NewHope Simple Implemetation from open source AMCL crypto library (https://github.com/MIRACL/amcl)");
	Serial.println("Ref. Alkim, Ducas, Popplemann and Schwabe (https://eprint.iacr.org/2016/1157)");
	Serial.printf("Alice shared secret= 0x");
	byteprint((uint8_t*)KEYA.val, KEYA.len);
	Serial.printf("Bob's shared secret= 0x");
	byteprint((uint8_t*)KEYB.val, KEYB.len);
	return;
}

#endif
