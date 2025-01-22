#include "encryption.h"


/**
 * @brief Encrypts a block of data using AES-256 ECB mode.
 * 
 * This function takes an input block of plaintext and encrypts it using the AES-256 ECB mode of encryption.
 * It assumes that the input data length is a multiple of the AES block size (16 bytes). It uses provided key
 * to perform the encryption and places the result in the output buffer.
 *
 * @param output Pointer to the buffer where encrypted data should be stored.
 * @param input Pointer to the data to be encrypted.
 * @param len Length of the input data in bytes. Must be a multiple of CIPHER_BLOCK_SIZE.
 * @param key Pointer to the encryption key.
 * @return false if encryption was successful, true if the key was invalid.
 */
bool encrypt(uint8_t *output, const uint8_t *input, uint8_t len, const uint8_t *key) {
#ifdef DEBUG
  // Print the size of the input data and the number of AES blocks that will be processed.
  Serial1.print(F("Taille entree chiffrement: ")); Serial1.print(len); Serial1.println(F(" bytes"));
  Serial1.print(F("Nombre de boucle: ")); Serial1.print(len / CIPHER_BLOCK_SIZE); Serial1.println(F(""));
#endif

  // Set the encryption key. If the key is invalid, return false.
  if(!aes256ECB.setKey(key, aes256ECB.keySize())){
    Serial1.println(F("Key non valide for encryption!"));
    return false;
  }

  // Encrypt each block of input and write it to the output.
  for(uint8_t i = 0; i < len / CIPHER_BLOCK_SIZE; i++) {
    uint8_t tmpBuffer[CIPHER_BLOCK_SIZE]; // Temporary buffer for encryption.
    // Copy a block of plaintext into the temporary buffer.
    memcpy(tmpBuffer, input + (i * CIPHER_BLOCK_SIZE), CIPHER_BLOCK_SIZE);
    // Encrypt the block.
    aes256ECB.encryptBlock(tmpBuffer, tmpBuffer);
    // Copy the encrypted block back to the output buffer.
    memcpy(output + (i * CIPHER_BLOCK_SIZE), tmpBuffer, CIPHER_BLOCK_SIZE);
  }

  // Clear AES object's internal state to prevent leakage of sensitive information.
  aes256ECB.clear();

  return true;  // Indicate successful encryption.
}


/**
 * @brief Decrypts a block of data using AES-256 ECB mode.
 * 
 * This function takes an input block of ciphertext and decrypts it using the AES-256 ECB mode of decryption.
 * It assumes that the input data length is a multiple of the AES block size (16 bytes). It uses the provided key
 * to perform the decryption and places the result in the output buffer.
 *
 * @param output Pointer to the buffer where decrypted data should be stored.
 * @param input Pointer to the data to be decrypted.
 * @param len Length of the input data in bytes. Must be a multiple of CIPHER_BLOCK_SIZE.
 * @param key Pointer to the decryption key.
 * @return false if decryption was successful, true if the key was invalid.
 */
bool decrypt(uint8_t* output, const uint8_t* input, uint8_t len, const uint8_t* key) {
#ifdef DEBUG
  // Debug output for the size of the input data and the number of AES blocks to be processed.
  Serial1.print(F("Taille entree dechiffrement: "));
  Serial1.print(len);
  Serial1.println(F(" bytes"));
  Serial1.print(F("Nombre de boucle: "));
  Serial1.print(len / CIPHER_BLOCK_SIZE);
  Serial1.println(F(""));
#endif

  // Set the decryption key. Return true if the key is invalid, indicating an error.
  if (!aes256ECB.setKey(key, aes256ECB.keySize())) {
    Serial1.println(F("Key non valide for decryption!"));
    return true;  // Indicate an error in key setup.
  }

  // Decrypt each block of input and write it to the output.
  for (uint8_t i = 0; i < len / CIPHER_BLOCK_SIZE; i++) {
#ifdef DEBUG
    // Debug output for the byte offset of the current block.
    Serial1.print(F("Decalage addresse: "));
    Serial1.print((i * CIPHER_BLOCK_SIZE));
    Serial1.println(F(" bytes"));
#endif
    uint8_t tmpBuffer[CIPHER_BLOCK_SIZE]; // Temporary buffer for decryption.
    // Copy a block of ciphertext into the temporary buffer.
    memcpy(tmpBuffer, input + (i * CIPHER_BLOCK_SIZE), CIPHER_BLOCK_SIZE);
    // Decrypt the block.
    aes256ECB.decryptBlock(tmpBuffer, tmpBuffer);
    // Copy the decrypted block back to the output buffer.
    memcpy(output + (i * CIPHER_BLOCK_SIZE), tmpBuffer, CIPHER_BLOCK_SIZE);
  }

  aes256ECB.clear();  // Clear AES object's internal state to prevent leakage of sensitive information.
  return false;  // Indicate successful decryption.
}