#pragma once

#define EAC_XTEA_ROUNDS 32
#define EAC_XTEA_DELTA  0x9E3779B9u

static __inline void eac_xtea_decrypt(eac_u32* v, const eac_u32* k) {
    eac_u32 v0 = v[0], v1 = v[1];
    eac_u32 sum = EAC_XTEA_DELTA * EAC_XTEA_ROUNDS;
    int i;
    for (i = 0; i < EAC_XTEA_ROUNDS; ++i) {
        v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + k[(sum >> 11) & 3]);
        sum -= EAC_XTEA_DELTA;
        v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
    }
    v[0] = v0; v[1] = v1;
}

static __inline void eac_xtea_encrypt(eac_u32* v, const eac_u32* k) {
    eac_u32 v0 = v[0], v1 = v[1];
    eac_u32 sum = 0;
    int i;
    for (i = 0; i < EAC_XTEA_ROUNDS; ++i) {
        v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
        sum += EAC_XTEA_DELTA;
        v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + k[(sum >> 11) & 3]);
    }
    v[0] = v0; v[1] = v1;
}

/* Anahtar: [0]=arabellek adresi, [1]=0, [2]=PID, [3]=0 */
static __inline void eac_make_key(eac_u32* k, void* buffer, eac_u32 pid) {
    k[0] = (eac_u32)(eac_u64)(eac_u64)buffer;
    k[1] = 0;
    k[2] = pid;
    k[3] = 0;
}