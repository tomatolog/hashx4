/* 
 * Copyright 2015 Kai Dietrich <mail@cleeus.de>
 *
 * This file is part of hashx4.
 *
 * Hashx4 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Hashx4 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with hashx4.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "hashx4_config.h"

#if HX4_HAS_MMX
# include <mmintrin.h>
#endif

#if HX4_HAS_SSE2
# include <emmintrin.h>
#endif

#if HX4_HAS_SSSE3
# include <tmmintrin.h>
#endif

#include "hashx4.h"
#include "hx4_util.h"
#include "../crc32c/include/crc32c/crc32c.h "

int hx4_djbx33a_32_ref(const void *buffer, size_t buffer_size, const void *cookie, size_t cookie_sz, void *out_hash, size_t out_hash_size) {
  const uint8_t *p;
  const uint8_t * const buffer_end = (uint8_t*)buffer + buffer_size;
  uint32_t state = 5381;
  int rc;

  rc = hx4_check_params(sizeof(state), buffer, buffer_size, cookie, cookie_sz, out_hash, out_hash_size);
  if(rc != HX4_ERR_SUCCESS) {
    return rc;
  }
  
  p = buffer;
  while(p<buffer_end) {
    state = state * 33  + *p;
    p++;
  }

  hx4_xor_cookie_32(&state, cookie);
  memcpy(out_hash, &state, sizeof(state));
  return HX4_ERR_SUCCESS;
}

int hx4_djbx33a_32_copt(const void *buffer, size_t buffer_size, const void *cookie, size_t cookie_sz, void *out_hash, size_t out_hash_size) {
  const uint8_t *p;
  const uint8_t * const buffer_end = (uint8_t*)buffer + buffer_size;
  const int num_bytes_to_seek = hx4_bytes_to_aligned(buffer, 16);
  uint32_t state = 5381;
  int rc;
  int i;

  rc = hx4_check_params(sizeof(state), buffer, buffer_size, cookie, cookie_sz, out_hash, out_hash_size);
  if(rc != HX4_ERR_SUCCESS) {
    return rc;
  }

  p = buffer;

  //hash input until p is aligned to alignment_target
  for(i=0; p<buffer_end && i<num_bytes_to_seek; i++) {
    state  = (state << 5) + state  + *p;
    p++;
  }

  HX4_ASSUME_ALIGNED(p, 16)

  //main processing loop
  while(p+15<buffer_end) {
    HX4_ASSUME_ALIGNED(p, 16)

#if 1
#define HX4_DJBX33A_ROUND(round) \
    /* state = state * 33 + p[round]; */ \
    state = (state << 5) + state + p[round];

    HX4_DJBX33A_ROUND(0)
    HX4_DJBX33A_ROUND(1)
    HX4_DJBX33A_ROUND(2)
    HX4_DJBX33A_ROUND(3)
    HX4_DJBX33A_ROUND(4)
    HX4_DJBX33A_ROUND(5)
    HX4_DJBX33A_ROUND(6)
    HX4_DJBX33A_ROUND(7)
    HX4_DJBX33A_ROUND(8)
    HX4_DJBX33A_ROUND(9)
    HX4_DJBX33A_ROUND(10)
    HX4_DJBX33A_ROUND(11)
    HX4_DJBX33A_ROUND(12)
    HX4_DJBX33A_ROUND(13)
    HX4_DJBX33A_ROUND(14)
    HX4_DJBX33A_ROUND(15)
#endif

#if 0
    for(i=0; i<16; i++) {
      //state = state*33 + p[i];
      state = (state << 5) + state + p[i];
    }
#endif

    p+=16;
  }
#undef HX4_DJBX33A_ROUND

  //hash remainder
  while(p<buffer_end) {
    //state = state * 33  + *p;
    state = (state << 5) + state + *p;
    p++;
  }

 
  hx4_xor_cookie_32(&state, cookie);
  memcpy(out_hash, &state, sizeof(state));
  return HX4_ERR_SUCCESS;
}


int hx4_x4djbx33a_128_ref(const void *buffer, size_t buffer_size, const void *cookie, size_t cookie_sz, void *out_hash, size_t out_hash_size) {
  const uint8_t *p;
  const uint8_t * const buffer_end = (uint8_t*)buffer + buffer_size;
  uint32_t state[] = { 5381, 5381, 5381, 5381 };
  int state_i=0;
  int rc;

  rc = hx4_check_params(sizeof(state), buffer, buffer_size, cookie, cookie_sz, out_hash, out_hash_size);
  if(rc != HX4_ERR_SUCCESS) {
    return rc;
  }
  
  p = buffer;
  while(p<buffer_end) {
    state[state_i] = state[state_i] * 33  + *p;
    p++;
    state_i = (state_i+1) % 4;
  }

  hx4_xor_cookie_128(state, cookie);
  memcpy(out_hash, state, sizeof(state));

  return HX4_ERR_SUCCESS;
}

int hx4_x4djbx33a_128_copt(const void *buffer, size_t buffer_size, const void *cookie, size_t cookie_sz, void *out_hash, size_t out_hash_size) {
  const uint8_t *p;
  const uint8_t * const buffer_end = (uint8_t*)buffer + buffer_size;
  const int num_bytes_to_seek = hx4_bytes_to_aligned(buffer, 16);
  uint32_t state[] = { 5381, 5381, 5381, 5381 };
  uint32_t state_tmp;
  int state_i=0;
  int rc;
  int i;

  rc = hx4_check_params(sizeof(state), buffer, buffer_size, cookie, cookie_sz, out_hash, out_hash_size);
  if(rc != HX4_ERR_SUCCESS) {
    return rc;
  }

  p = buffer;

  //hash input until p is aligned to alignment_target
  for(i=0; p<buffer_end && i<num_bytes_to_seek; i++) {
    //state[state_i] = state[state_i] * 33  + *p;
    state[state_i] = (state[state_i] << 5) + state[state_i]  + *p;
    p++;
    state_i = (state_i+1) & 0x03;
  }

  HX4_ASSUME_ALIGNED(p, 16)

  //rotate states to match position on the input stream
  //so that the main loop can be simple
  for(i=0; i<state_i; i++) {
    state_tmp = state[0];
    state[0] = state[1];
    state[1] = state[2];
    state[2] = state[3];
    state[3] = state_tmp;
  }

  //main processing loop
  while(p+15<buffer_end) {
    HX4_ASSUME_ALIGNED(p, 16)

#define HX4_DJB2X4_COPT_ROUND(state_i, round) \
    state[state_i] = (state[state_i] << 5) + state[state_i] + p[round];

    HX4_DJB2X4_COPT_ROUND(0,0)
    HX4_DJB2X4_COPT_ROUND(1,1)
    HX4_DJB2X4_COPT_ROUND(2,2)
    HX4_DJB2X4_COPT_ROUND(3,3)
    HX4_DJB2X4_COPT_ROUND(0,4)
    HX4_DJB2X4_COPT_ROUND(1,5)
    HX4_DJB2X4_COPT_ROUND(2,6)
    HX4_DJB2X4_COPT_ROUND(3,7)
    HX4_DJB2X4_COPT_ROUND(0,8)
    HX4_DJB2X4_COPT_ROUND(1,9)
    HX4_DJB2X4_COPT_ROUND(2,10)
    HX4_DJB2X4_COPT_ROUND(3,11)
    HX4_DJB2X4_COPT_ROUND(0,12)
    HX4_DJB2X4_COPT_ROUND(1,13)
    HX4_DJB2X4_COPT_ROUND(2,14)
    HX4_DJB2X4_COPT_ROUND(3,15)

    p+=16;
  }
  
#undef HX4_DJB2X4_COPT_ROUND

  //rotate back the states
  for(i=0; i<state_i; i++) {
    state_tmp = state[3];
    state[3] = state[2];
    state[2] = state[1];
    state[1] = state[0];
    state[0] = state_tmp;
  }

  //process remainder
  while(p<buffer_end) {
    //state[state_i] = state[state_i] * 33  + *p;
    state[state_i] = (state[state_i] << 5) + state[state_i]  + *p;
    p++;
    state_i = (state_i+1) & 0x03;
  }

  hx4_xor_cookie_128(state, cookie);
  memcpy(out_hash, state, sizeof(state));

  return HX4_ERR_SUCCESS;
}

#if HX4_HAS_MMX
int hx4_x4djbx33a_128_mmx(const void *buffer, size_t buffer_size, const void *cookie, size_t cookie_sz, void *out_hash, size_t out_hash_size) {
  const uint8_t *p;
  const uint8_t * const buffer_end = (uint8_t*)buffer + buffer_size;
  const int num_bytes_to_seek = hx4_bytes_to_aligned(buffer, 8);
  HX4_ALIGNED(uint32_t state[], 8) = { 5381, 5381, 5381, 5381 };
  uint32_t state_tmp;
  int state_i=0;
  int rc;
  int i;
  __m64 xstate0;
  __m64 xstate1;
  __m64 xpin;
  __m64 xp0;
  __m64 xp1;
  __m64 xdword;
 
  rc = hx4_check_params(sizeof(state), buffer, buffer_size, cookie, cookie_sz, out_hash, out_hash_size);
  if(rc != HX4_ERR_SUCCESS) {
    return rc;
  }

  p = buffer;

  //hash input until p is aligned to alignment_target
  for(i=0; p<buffer_end && i<num_bytes_to_seek; i++) {
    //state[state_i] = state[state_i] * 33  + *p;
    state[state_i] = (state[state_i] << 5) + state[state_i]  + *p;
    p++;
    state_i = (state_i+1) & 0x03;
  }

  HX4_ASSUME_ALIGNED(p, 8)

  //rotate states to match position on the input stream
  //so that the main loop can be simple
  for(i=0; i<state_i; i++) {
    state_tmp = state[0];
    state[0] = state[1];
    state[1] = state[2];
    state[2] = state[3];
    state[3] = state_tmp;
  }

  //load state into registers
  xstate0 = ((__m64*)state)[0];
  xstate1 = ((__m64*)state)[1];

  //main processing loop
  while(p+8<buffer_end) {
    HX4_ASSUME_ALIGNED(p, 8)

    /*load 8 bytes into xpin */
    xpin = *((__m64*)(p));

#define HX4_MMX_X4DJBX33A(xstate, xp) \
    xp = _mm_add_pi32(xstate, xp); \
    xstate = _mm_slli_pi32(xstate, 5); \
    xstate = _mm_add_pi32(xstate, xp); \

    //lower 4 byte
    xdword = _mm_setzero_si64();
    xdword = _mm_unpacklo_pi8(xpin, xdword);
    //byte[0,1]
    xp0 = _mm_setzero_si64();
    xp0 = _mm_unpacklo_pi8(xdword, xp0);
    HX4_MMX_X4DJBX33A(xstate0, xp0);
    //byte[2,3]
    xp1 = _mm_setzero_si64();
    xp1 = _mm_unpackhi_pi8(xdword, xp1);
    HX4_MMX_X4DJBX33A(xstate1, xp1);

    //upper 4 byte
    xdword = _mm_setzero_si64();
    xdword = _mm_unpackhi_pi8(xpin, xdword);
    //byte[0,1]
    xp0 = _mm_setzero_si64();
    xp0 = _mm_unpacklo_pi8(xdword, xp0);
    HX4_MMX_X4DJBX33A(xstate0, xp0);
    //byte[2,3]
    xp1 = _mm_setzero_si64();
    xp1 = _mm_unpackhi_pi8(xdword, xp1);
    HX4_MMX_X4DJBX33A(xstate1, xp1);
#undef HX4_MMX_X4DJBX33A

    p+=8;
  }

  //store registers back into state
  ((__m64*)state)[0] = xstate0;
  ((__m64*)state)[1] = xstate1;

  //reset MMX
  _mm_empty();

  //rotate back the states
  for(i=0; i<state_i; i++) {
    state_tmp = state[3];
    state[3] = state[2];
    state[2] = state[1];
    state[1] = state[0];
    state[0] = state_tmp;
  }

  //process remainder
  while(p<buffer_end) {
    //state[state_i] = state[state_i] * 33  + *p;
    state[state_i] = (state[state_i] << 5) + state[state_i]  + *p;
    p++;
    state_i = (state_i+1) & 0x03;
  }

  hx4_xor_cookie_128(state, cookie);
  memcpy(out_hash, state, sizeof(state));

  return HX4_ERR_SUCCESS;
}
#endif //HX4_HAS_MMX

#if HX4_HAS_SSE2
int hx4_x4djbx33a_128_sse2(const void *buffer, size_t buffer_size, const void *cookie, size_t cookie_sz, void *out_hash, size_t out_hash_size) {
  const uint8_t *p;
  const uint8_t * const buffer_end = (uint8_t*)buffer + buffer_size;
  const int num_bytes_to_seek = hx4_bytes_to_aligned(buffer, 16);
  HX4_ALIGNED(uint32_t state[], 16) = { 5381, 5381, 5381, 5381 };
  uint32_t state_tmp;  
  int state_i = 0;
  int rc;
  int i;
  __m128i xstate;
  __m128i xpin;
  __m128i xp;
  __m128i xqword;

  rc = hx4_check_params(sizeof(state), buffer, buffer_size, cookie, cookie_sz, out_hash, out_hash_size);
  if(rc != HX4_ERR_SUCCESS) {
    return rc;
  }

  p = buffer;

  //hash input until p is aligned to alignment_target
  for(i=0; p<buffer_end && i<num_bytes_to_seek; i++) {
    //state[state_i] = state[state_i] * 33  + *p;
    state[state_i] = (state[state_i] << 5) + state[state_i]  + *p;
    p++;
    state_i = (state_i+1) & 0x03;
  }

  HX4_ASSUME_ALIGNED(p, 16)

  //rotate states to match position on the input stream
  //so that the main loop can be simple
  for(i=0; i<state_i; i++) {
    state_tmp = state[0];
    state[0] = state[1];
    state[1] = state[2];
    state[2] = state[3];
    state[3] = state_tmp;
  }
  
  //transfer state into register
  xstate = _mm_load_si128((__m128i*)state);
  
 //main processing loop
  while(p+15<buffer_end) {
    HX4_ASSUME_ALIGNED(p, 16)

#define HX4_SSE2_X4DJBX33A(xstate, xp) \
    xp = _mm_add_epi32(xp, xstate); \
    xstate = _mm_slli_epi32(xstate, 5 ); \
    xstate = _mm_add_epi32(xstate, xp);

    //load 16 bytes aligned
    xpin = _mm_load_si128((__m128i*)p);
    
    //qword0, lower 8byte
    xqword = _mm_setzero_si128();
    xqword = _mm_unpacklo_epi8(xpin, xqword);   
    //dword0
    xp = _mm_setzero_si128();
    xp = _mm_unpacklo_epi8(xqword, xp);
    HX4_SSE2_X4DJBX33A(xstate, xp);
    //dword1
    xp = _mm_setzero_si128();
    xp = _mm_unpackhi_epi8(xqword, xp);
    HX4_SSE2_X4DJBX33A(xstate, xp);

    //qword1, upper 8byte
    xqword = _mm_setzero_si128();
    xqword = _mm_unpackhi_epi8(xpin, xqword);
    //dword2
    xp = _mm_setzero_si128();
    xp = _mm_unpacklo_epi8(xqword, xp);
    HX4_SSE2_X4DJBX33A(xstate, xp);
    //dword3
    xp = _mm_setzero_si128();
    xp = _mm_unpackhi_epi8(xqword, xp);
    HX4_SSE2_X4DJBX33A(xstate, xp);
#undef HX4_SSE2_X4DJBX33A
        
    p+=16;
  }

  //store back state from register into memory
  _mm_store_si128((__m128i*)state, xstate);
  
  //rotate back the states
  for(i=0; i<state_i; i++) {
    state_tmp = state[3];
    state[3] = state[2];
    state[2] = state[1];
    state[1] = state[0];
    state[0] = state_tmp;
  }

  //process any input that is left
  while(p<buffer_end) {
    //state[state_i] = state[state_i] * 33  + *p;
    state[state_i] = (state[state_i] << 5) + state[state_i]  + *p;
    p++;
    state_i = (state_i+1) & 0x03;
  }

  hx4_xor_cookie_128(state, cookie);
  memcpy(out_hash, state, sizeof(state));

  return HX4_ERR_SUCCESS;
}
#endif //HX4_HAS_SSE2

#if HX4_HAS_SSSE3
int hx4_x4djbx33a_128_ssse3(const void *buffer, size_t buffer_size, const void *cookie, size_t cookie_sz, void *out_hash, size_t out_hash_size) {
  const uint8_t *p;
  const uint8_t * const buffer_end = (uint8_t*)buffer + buffer_size;
  const int num_bytes_to_seek = hx4_bytes_to_aligned(buffer, 16);
  HX4_ALIGNED(uint32_t state[], 16) = { 5381, 5381, 5381, 5381 };
  uint32_t state_tmp;
  __m128i xstate;
  __m128i xp;
  __m128i xpin;
  __m128i xbmask;
  __m128i xshuffle;
  int state_i = 0;
  int rc;
  int i;

  rc = hx4_check_params(sizeof(state), buffer, buffer_size, cookie, cookie_sz, out_hash, out_hash_size);
  if (rc != HX4_ERR_SUCCESS) {
    return rc;
  }

  p = buffer;

  //hash input until p is aligned to alignment_target
  for (i = 0; p<buffer_end && i<num_bytes_to_seek; i++) {
    //state[state_i] = state[state_i] * 33 + *p;
    state[state_i] = (state[state_i] << 5) + state[state_i] + *p;
    p++;
    state_i = (state_i + 1) & 0x03;
  }

  HX4_ASSUME_ALIGNED(p, 16)

  //rotate states to match position on the input stream
  //so that the main loop can be simple
  for (i = 0; i<state_i; i++) {
    state_tmp = state[0];
    state[0] = state[1];
    state[1] = state[2];
    state[2] = state[3];
    state[3] = state_tmp;
  }

  //transfer state into register
  xstate = _mm_load_si128((__m128i*)state);
  //load AND mask
  xbmask = _mm_set1_epi32(0x000000ff);
  //load shuffle mask
  xshuffle = _mm_set_epi8(
    15, 11, 7, 3,
    14, 10, 6, 2,
    13, 9 , 5, 1,
    12, 8 , 4, 0
  );

  //main processing loop
  while (p+15<buffer_end) {
    HX4_ASSUME_ALIGNED(p, 16)

    //load 16 bytes aligned
    xpin = _mm_load_si128((__m128i*)p);
    xpin = _mm_shuffle_epi8(xpin, xshuffle);

#define HX4_SSSE3_DJB2ROUND(round) \
    xp = _mm_srli_epi32(xpin, 8*round); \
    xp = _mm_and_si128(xp, xbmask); \
    xp = _mm_add_epi32(xp, xstate); \
    xstate = _mm_slli_epi32(xstate, 5); \
    xstate = _mm_add_epi32(xstate, xp); \

    HX4_SSSE3_DJB2ROUND(0)
    HX4_SSSE3_DJB2ROUND(1)
    HX4_SSSE3_DJB2ROUND(2)
    HX4_SSSE3_DJB2ROUND(3)

    p += 16;
  }
#undef HX4_SSSE3_DJB2ROUND

  //store back state from register into memory
  _mm_store_si128((__m128i*)state, xstate);

  //rotate back the states
  for (i = 0; i<state_i; i++) {
    state_tmp = state[3];
    state[3] = state[2];
    state[2] = state[1];
    state[1] = state[0];
    state[0] = state_tmp;
  }

  //process any input that is left
  while (p<buffer_end) {
    //state[state_i] = state[state_i] * 33 + *p;
    state[state_i] = (state[state_i] << 5) + state[state_i] + *p;   
    p++;
    state_i = (state_i + 1) & 0x03;
  }
  
  hx4_xor_cookie_128(state, cookie);
  memcpy(out_hash, state, sizeof(state));

  return HX4_ERR_SUCCESS;
}
#endif //HX4_HAS_SSSE3

int mnt_fnv64 (const void *buffer, size_t buffer_size, const void *cookie, size_t cookie_sz, void *out_hash, size_t out_hash_size)
{
	const uint8_t * p = (const uint8_t *)buffer;
	uint64_t hval = 0xcbf29ce484222325ULL;
	for ( ; buffer_size>0; buffer_size-- )
	{
		// xor the bottom with the current octet
		hval ^= (uint64_t)*p++;

		// multiply by the 64 bit FNV magic prime mod 2^64
		hval += (hval << 1) + (hval << 4) + (hval << 5) + (hval << 7) + (hval << 8) + (hval << 40); // gcc optimization
	}

	memcpy ( out_hash, &hval, sizeof(hval) );

	return HX4_ERR_SUCCESS;

}

int hx4_crc32c (const void *buffer, size_t buffer_size, const void *cookie, size_t cookie_sz, void *out_hash, size_t out_hash_size)
{
	uint32_t hval = crc32c_value ( buffer, buffer_size );

	memcpy ( out_hash, &hval, sizeof(hval) );

	return HX4_ERR_SUCCESS;
}