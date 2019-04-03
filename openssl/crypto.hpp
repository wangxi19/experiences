#ifndef _CRYPTO_HPP
#define _CRYPTO_HPP

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

void openSSLInit() {
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();
    OpenSSL_add_all_ciphers();
}

void PrintError() {
    char err[130]{0, };
    ERR_load_crypto_strings();
    ERR_error_string(ERR_get_error(), err);
    fprintf(stderr, "Error message: %s\n", err);
}

int RSASetKey(RSA** rsa, void* keyData, size_t keyLen, bool pubKey = true) {
    BIO* bio = BIO_new(BIO_s_mem());
    if (nullptr == bio) {
        return -1;
    }

    if (0 >= BIO_write(bio, keyData, (int)keyLen)) {
        BIO_free(bio);
        return -1;
    }

    if (pubKey) {
        EVP_PKEY* evpKey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
        if (nullptr == evpKey) {
            BIO_free(bio);
            PrintError();
            return -1;
        }
        *rsa = EVP_PKEY_get1_RSA(evpKey);
    } else {
        if (nullptr == PEM_read_bio_RSAPrivateKey(bio, rsa, nullptr, nullptr)) {
            BIO_free(bio);
            PrintError();
            return -1;
        }
    }

    BIO_free(bio);

    return 0;
}

int RSAEncrypt(RSA* rsa,  const void* fromData, size_t fLen, void* toData, int padding, bool pubKey = true) {
    int ret {-1};
    if (pubKey) {
        ret = RSA_public_encrypt(fLen, (const unsigned char *)fromData, (unsigned char *)toData, rsa, padding);
        if (-1 == ret) {
            PrintError();
            return -1;
        }
    } else {
        ret = RSA_private_encrypt(fLen, (const unsigned char *)fromData, (unsigned char *)toData, rsa, padding);
        if (-1 == ret) {
            PrintError();
            return -1;
        }
    }

    return ret;
}

int RSADecrypt(RSA* rsa, const void* fromData, size_t fLen, void* toData, int padding, bool pubKey = true) {
    int ret {-1};
    if (pubKey) {
        ret = RSA_public_decrypt(fLen, (const unsigned char *)fromData, (unsigned char *)toData, rsa, padding);
        if (ret <= 0) {
            PrintError();
            return ret;
        }
    } else {
        ret = RSA_private_decrypt(fLen, (const unsigned char *)fromData, (unsigned char *)toData, rsa, padding);
        if (ret <= 0) {
            PrintError();
            return ret;
        }
    }

    return ret;
}

int readFileContent(char* buf, const char* fileName) {
    std::FILE* fp = std::fopen(fileName, "r+");
    if (fp == nullptr) return -1;

    std::fseek(fp, 0, SEEK_END);
    size_t len = std::ftell(fp);
    std::rewind(fp);
    std::fread(buf, 1, len, fp);
    std::fclose(fp);
    return len;
}

void writeToFile(const char* buf, size_t len, const char* fileName) {
    std::FILE* fp = std::fopen(fileName, "w+");
    if (fp == nullptr) return;

    std::fwrite(buf, 1, len, fp);
    std::fclose(fp);
    return;
}

#endif //_CRYPTO_HPP
