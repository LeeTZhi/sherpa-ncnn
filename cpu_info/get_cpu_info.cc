#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if.h>
#include <mbedtls/sha256.h>
#include <mbedtls/pk.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>


int get_mac_address(uint8_t address[6]) {
    struct ifreq ifr;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ - 1);

    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("ioctl");
        close(fd);
        return -1;
    }

    memcpy(address, ifr.ifr_hwaddr.sa_data, 6);
    close(fd);
    return 0;
}

int get_cpu_serial(uint8_t serial[8]) {
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp) {
        perror("fopen");
        return -1;
    }
    char line[256];
    char *serial_str = NULL;

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "Serial", 6) == 0) {
            serial_str = strchr(line, ':');
            serial_str += 2; // Skip over ": "
            break;
        }
    }

    if (serial_str) {
        for (int i = 0; i < 8; i++) {
            sscanf(serial_str + 2 * i, "%2hhx", &serial[i]);
        }
    } else {
        fprintf(stderr, "Serial number not found\n");
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}



int calculate_sha256(uint8_t address[6], uint8_t serial[8], uint8_t hash[32]) {
    mbedtls_sha256_context ctx;
    int ret = 0;
    mbedtls_sha256_init(&ctx);
    ret = mbedtls_sha256_starts(&ctx, 0); // 0 for SHA256, 1 for SHA224
    if (ret != 0) {
        goto EXIT;
    }
    ret = mbedtls_sha256_update(&ctx, address, 6);
    if (ret != 0) {
        goto EXIT;
    }
    ret = mbedtls_sha256_update(&ctx, serial, 8);
    if (ret != 0) {
        goto EXIT;
    }

    mbedtls_sha256_finish(&ctx, hash);

EXIT:
    mbedtls_sha256_free(&ctx);
    return ret;
}


int sign_hash(uint8_t *hash, size_t hash_len, const char *private_key, uint8_t *signature, size_t *sig_len) {
    int ret;
    mbedtls_pk_context pk;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = "mbedtls_pk_sign";

    mbedtls_pk_init(&pk);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    // Load private key
    if ((ret = mbedtls_pk_parse_keyfile(&pk, private_key, NULL)) != 0) {
        goto EXIT;
    }

    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) pers, strlen(pers))) != 0) {
        goto EXIT;
    }

    if ((ret = mbedtls_pk_sign(&pk, MBEDTLS_MD_SHA256, hash, hash_len, signature, sig_len, mbedtls_ctr_drbg_random, &ctr_drbg)) != 0) {
        goto EXIT;
    }

EXIT:
    mbedtls_pk_free(&pk);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    return ret;
}

int verify_signature(uint8_t *hash, size_t hash_len, const char *public_key, uint8_t *signature, size_t sig_len) {
    int ret;
    mbedtls_pk_context pk;

    mbedtls_pk_init(&pk);

    // Load public key
    if ((ret = mbedtls_pk_parse_public_keyfile(&pk, public_key)) != 0) {
        goto EXIT;
    }

    if ((ret = mbedtls_pk_verify(&pk, MBEDTLS_MD_SHA256, hash, hash_len, signature, sig_len)) != 0) {
        goto EXIT;
    }

EXIT:
    mbedtls_pk_free(&pk);

    return ret;
}
