#include <Crypto.h> // Include the Crypto library for cryptographic operations
#include <AES.h>    // Include the AES library specific for AES encryption methods
#include "hardwareSerial.h"

#define CIPHER_BLOCK_SIZE 16  // Define the block size for AES encryption, which is 16 bytes for AES-256

AES256 aes256ECB;  // Create an instance of AES256 to use for ECB encryption