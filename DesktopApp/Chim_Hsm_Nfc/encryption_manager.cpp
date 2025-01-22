#include "encryption_manager.h" 
using namespace CryptoPP;

const size_t SALT_SIZE = 32; // Change this as per security requirements

void DeriveKeyAndIV(const std::string& passphrase, SecByteBlock& key, SecByteBlock& iv, const byte* salt, size_t saltSize) {
    int iterations = 10000;
    SecByteBlock derived(AES::MAX_KEYLENGTH + AES::BLOCKSIZE);
    PKCS5_PBKDF2_HMAC<SHA256> kdf;

    kdf.DeriveKey(
        derived.data(), derived.size(),
        0x00,
        reinterpret_cast<const byte*>(passphrase.data()), passphrase.size(),
        salt, saltSize,
        iterations
    );

    key.Assign(derived.data(), AES::MAX_KEYLENGTH);
    iv.Assign(derived.data() + AES::MAX_KEYLENGTH, AES::BLOCKSIZE);
}

std::string EncryptAES(const std::string& plainText, const SecByteBlock& key, const SecByteBlock& iv) {
    // std::cout << "plainText : " << plainText << std::endl;
    std::string cipherText;
    CBC_Mode<AES>::Encryption encryptor(key, key.size(), iv);
    StringSource ss(plainText, true,
        new StreamTransformationFilter(encryptor,
            new StringSink(cipherText)
        )
    );
    // std::cout << "cipherText : " << cipherText << std::endl;
    return cipherText;
}

std::string DecryptAES(const std::string& cipherText, const SecByteBlock& key, const SecByteBlock& iv) {
    std::string decryptedText;
    CBC_Mode<AES>::Decryption decryptor(key, key.size(), iv);
    StringSource ss(cipherText, true,
        new StreamTransformationFilter(decryptor,
            new StringSink(decryptedText)
        )
    );
    return decryptedText;
}

bool encryption(std::string passphrase, std::string keys, std::string &encodedSalt, std::string &encodedKeys) {

    // Generate a random salt
    byte salt[SALT_SIZE];
    AutoSeededRandomPool prng;
    prng.GenerateBlock(salt, sizeof(salt));
    // std::cout << "salt : " << salt << std::endl;
    // Derive key and IV from passphrase and salt
    SecByteBlock key(AES::MAX_KEYLENGTH), iv(AES::BLOCKSIZE);
    DeriveKeyAndIV(passphrase, key, iv, salt, SALT_SIZE);


    std::string cipherText = EncryptAES(keys, key, iv);

    // std::cout << "cipher Text : " << cipherText << std::endl;

    // Encode cipherText and salt into hexadecimal for storage or transmission
    StringSource ss1(salt, SALT_SIZE, true, new HexEncoder(new StringSink(encodedSalt)));
    StringSource ss2(cipherText, true, new HexEncoder(new StringSink(encodedKeys)));

    // std::cout << "encodedSalt : " << encodedSalt << std::endl;
    // std::cout << "encodedKeys : " << encodedKeys << std::endl;


    return 0;
}


bool decryption(std::string passphrase, std::string encodedSalt, std::string encodedKeys, std::string &decryptedKeys) {



    // To Decrypt, you would split the storedValue back into salt and cipherText,
    // decode them from hex, and then call DecryptAES()


    // Decode hex salt and ciphertext
    byte salt[SALT_SIZE];
    std::string decodedKeys;
    StringSource ss3(encodedSalt, true, new HexDecoder(new ArraySink(salt, SALT_SIZE)));
    StringSource ss4(encodedKeys, true, new HexDecoder(new StringSink(decodedKeys)));

    /*std::cout << "decodedSalt :" << salt << std::endl;
    std::cout << "decodedKeys :" << decodedKeys << std::endl;*/


    // Derive key and IV from passphrase and salt
    SecByteBlock key(AES::MAX_KEYLENGTH), iv(AES::BLOCKSIZE);
    DeriveKeyAndIV(passphrase, key, iv, salt, SALT_SIZE);

    decryptedKeys = DecryptAES(decodedKeys, key, iv);

    return 0;
}