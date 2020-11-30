#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <string.h>
#include "data.h"

#define IV "00000000"

int encrypt_file(char *in, char *out, unsigned char *key) {
    FILE *infile = fopen(in, "rb"); // File to be encrypted
    if (infile == NULL) {
        Print("Error: could not open file.\n");
        return 0;
    }
    FILE *outfile = fopen(out, "wb"); // File to be written to with encrypted contents

    // get file size
    fseek(infile, 0L, SEEK_END);
    int fsize = ftell(infile);
    fseek(infile, 0L, SEEK_SET);

    EVP_CIPHER_CTX *ctx;

    int out_len_1 = 0; int out_len_2 = 0; int sig_len = 0;
    unsigned char *indata = malloc(fsize);
    unsigned char *indata_copy = malloc(fsize);
    unsigned char *outdata = malloc(fsize*2);
    unsigned char *signature = malloc(32);

    // Read Entire File
    fread(indata, sizeof(char), fsize, infile);

    // Encrypt file data
    if(!(ctx = EVP_CIPHER_CTX_new())) {
        Print("Failed to initialize context.\n");
        ERR_print_errors_fp(stderr);
        return 0;
    }

    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, IV)) {
        Print("Failed to initialize cipher.\n");
        ERR_print_errors_fp(stderr);
        return 0;
    }

    if(1 != EVP_EncryptUpdate(ctx, outdata, &out_len_1, indata, fsize)) {
        Print("Failed to update encryption.\n");
        ERR_print_errors_fp(stderr);
        return 0;
    }

    if(1 != EVP_EncryptFinal_ex(ctx, outdata + out_len_1, &out_len_2)) {
        Print("Failed to finalize encryption.\n");
        ERR_print_errors_fp(stderr);
        return 0;
    }

    // Sign encrypted data to protect integrity.
    if(!HMAC(EVP_sha256(), key, 16, outdata, out_len_1 + out_len_2, signature, &sig_len)) {
        Print("Signature generation failed.\n");
        return 0;
    }

    // Store signature in a file
    FILE *sig = fopen("signature.pem", "wb");
    fwrite(signature, sizeof(char), sig_len, sig);

    // Write encrypted data to outfile
    fwrite(outdata, sizeof(char), out_len_1 + out_len_2, outfile);

    EVP_CIPHER_CTX_free(ctx);
    free(indata);
    free(outdata);
    fclose(infile);
    fclose(outfile);
    return 1;
}

int decrypt_file(char *in, char *out, unsigned char *key) {
    FILE *infile = fopen(in, "rb"); // File to be decrypted
    FILE *outfile = fopen(out, "wb"); // File to be written to with decrypted contents
    FILE *sig = fopen("signature.pem", "rb");
    if (infile == NULL) {
        Print("Error: could not open file.\n");
        return 0;
    }
    
    // Get in file size
    fseek(infile, 0L, SEEK_END);
    int fsize = ftell(infile);
    fseek(infile, 0L, SEEK_SET);

    // Get signature file size
    fseek(sig, 0L, SEEK_END);
    int sigsize = ftell(sig);
    fseek(sig, 0L, SEEK_SET);

    if (sigsize == 0 || fsize == 0) {
        fclose(infile);
        fclose(outfile);
        fclose(sig);
        Print("Error getting size of files.\n");
        return 0;
    }

    // Initialize some stuff
    int out_len_1 = 0; int out_len_2 = 0;
    unsigned char *indata = malloc(fsize);
    unsigned char *outdata = malloc(fsize*2);
    unsigned char *stored_signature = malloc(sigsize);
    unsigned char *signature = malloc(sigsize);
    int sig_len;

    // Read in signature
    if (sigsize != 32) {
        Print("Signature has been compromised.\n");
        return 0;
    }
    fread(stored_signature, sizeof(char), sigsize, sig);

    // Read Entire File
    if (fsize > 10000000) {
        Print("Error: File is bigger than expected.\n");
        return 0;
    }
    fread(indata, sizeof(char), fsize, infile);

    // Generate Signature for read in data
    if(!HMAC(EVP_sha256(), key, 16, indata, fsize, signature, &sig_len)) {
        Print("Signature generation failed.\n");
        return 0;
    }
    
    // Compare the signatures to verify the integrity of the file:
    if (sig_len != sigsize || 0 != CRYPTO_memcmp(signature, stored_signature, sig_len)) {
        Print("File integrity has been compromised.\n");
        return 0;
    }

    // Decrypt file
    EVP_CIPHER_CTX *ctx;

    if(!(ctx = EVP_CIPHER_CTX_new()))
        return 0;

    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, IV))
        return 0;

    if(1 != EVP_DecryptUpdate(ctx, outdata, &out_len_1, indata, fsize))
        return 0;

    if(1 != EVP_DecryptFinal_ex(ctx, outdata + out_len_1, &out_len_2))
        return 0;

    fwrite(outdata, sizeof(char), out_len_1 + out_len_2, outfile);

    EVP_CIPHER_CTX_free(ctx);
    free(indata);
    free(outdata);
    fclose(infile);
    fclose(outfile);
    return 1;
}