/*
 * Program related to caches and memory. It is used to determine 
 * the total cache size, specifically for L3.
 * 
 * Copyright 2021 Richard L. Sites
 * Copyright (c) 2024 Christ Lin
 *
 * This code is adapted from the mystery2.cc example in the book
 * "Understanding Software Dynamics" by Richard L. Sites.
 *
 * Modifications were made by the Laboratory for Physical Science (LPS) team
 * from the Applied Methods and Research Experience (AMRE) program, 2024.
 * 
 * In 2024, we only changed the FindCacheSizes() function for detecting non-power-of-2 caches.
 * 
 * Note that due to the sensitivity of how caches work, it's better to close all other background programs to run the test.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> // for time()
#include <iostream>
#include <cstring>
#include <cstdlib>

#include "basetypes.h"
#include "polynomial.h"
#include "timecounters.h"

// We use a couple of fast pseudo-random generators that are based on standard 
// cyclic reduncy check arithmetic. Look in Wikipedia for more information on 
// CRC calculations.
//
// The particular CRC style we use is a left shift by one bit, followed by an 
// XOR of a specific CRC bit pattern if the bit shifted out is 1. This mimics
// long-used hardware that feeds back the top bit to XOR taps at specific 
// places in a shift register. Our use here is just to get a bunch of non-zero 
// bit patterns that have poor correlation from one to the next.
//
// Rather than the simple 
//  if (highbit is 1)
//    x = (x << 1) ^ bit_pattern
//  else
//    x = (x << 1)
//
// we make this calculation branch-free and therefore fast by ANDing the bit
// pattern against either 00000... or 11111..., depending on the value of the
// high bit. This is accomplished by an arithmetic right shift that sign-
// extends the input value. The entire expansion is four inline instrucitons:
// shift, shift, and, xor.
// 

// x must be of type uint8
// #define POLY8 (0x1d)   // From CRC-8-SAE J1850
// #define POLYSHIFT8(x) ( ((x) << 1) ^ ((static_cast<int8>((x)) >> 7) & POLY8) )
// #define POLYINIT8 (0xffu)

// POLY8 is a crc-based calculation that cycles through 255 byte values,
// excluding zero so long as the initial value is non-zero. If the initial
// value is zero, it cycles at zero every time.
//
// Here are the values if started at POLYINIT8 = 0xff
//   ff e3 db ab 4b 96 31 62 c4 95 37 6e dc a5 57 ae 
//   41 82 19 32 64 c8 8d 07 0e 1c 38 70 e0 dd a7 53 
//   a6 51 a2 59 b2 79 f2 f9 ef c3 9b 2b 56 ac 45 8a 
//   09 12 24 48 90 3d 7a f4 f5 f7 f3 fb eb cb 8b 0b 
//   16 2c 58 b0 7d fa e9 cf 83 1b 36 6c d8 ad 47 8e 
//   01 02 04 08 10 20 40 80 1d 3a 74 e8 cd 87 13 26 
//   4c 98 2d 5a b4 75 ea c9 8f 03 06 0c 18 30 60 c0 
//   9d 27 4e 9c 25 4a 94 35 6a d4 b5 77 ee c1 9f 23 
//   46 8c 05 0a 14 28 50 a0 5d ba 69 d2 b9 6f de a1 
//   5f be 61 c2 99 2f 5e bc 65 ca 89 0f 1e 3c 78 f0 
//   fd e7 d3 bb 6b d6 b1 7f fe e1 df a3 5b b6 71 e2 
//   d9 af 43 86 11 22 44 88 0d 1a 34 68 d0 bd 67 ce 
//   81 1f 3e 7c f8 ed c7 93 3b 76 ec c5 97 33 66 cc 
//   85 17 2e 5c b8 6d da a9 4f 9e 21 42 84 15 2a 54 
//   a8 4d 9a 29 52 a4 55 aa 49 92 39 72 e4 d5 b7 73 
//   e6 d1 bf 63 c6 91 3f 7e fc e5 d7 b3 7b f6 f1 ff 

static const int kPageSize = 4096;
static const int kPageSizeMask = kPageSize - 1;

// Make an array bigger than any expected cache size
static const int kMaxArraySize = 1152 * 1024 * 1024;

// We will read and write these pairs, allocated at different strides
struct Pair {
  Pair* next;
  int64 data;
};
// We use this to make variables live by never printing them, but make sure
// the compiler doesn't know that.
static time_t gNeverZero = 1;

// Allocate a byte array of given size, aligned on a page boundary
// Caller will call free(rawptr)
uint8* AllocPageAligned(int bytesize, uint8** rawptr) {
  int newsize = bytesize + kPageSizeMask;
  *rawptr = reinterpret_cast<uint8*>(malloc(newsize));
  uintptr_t temp = reinterpret_cast<uintptr_t>(*rawptr);
  temp = (temp + kPageSizeMask) & ~kPageSizeMask;
  return  reinterpret_cast<uint8*>(temp);
}

// Zero a byte array
void ZeroAll(uint8* ptr, int bytesize) {
  memset(ptr, 0, bytesize);
}

// Fill byte array with non-zero pseudo-random bits
void PseudoAll(uint8* ptr, int bytesize) {
  uint32* wordptr = reinterpret_cast<uint32*>(ptr);
  int wordcount = bytesize >> 2;
  uint32 x = POLYINIT32;
  for (int i = 0; i < wordcount; ++i) {
    *wordptr++ = x;
    x = POLYSHIFT32(x);
  }
}

// In a byte array, create a linked list of Pairs, spaced by the given stride. 
// Pairs are generally allocated near the front of the array first and near 
// the end of the array last. The list will have floor(bytesize / bytestride) 
// elements. The last element's next field is NULL and all the data fields are 
// zero. 
//
// ptr must be aligned on a multiple of sizeof(void*), i.e. 8 for a 64-bit CPU
// bytestride must be a multiple of sizeof(void*), and  must be at least 16
//
// If makelinear is true, the list elements are at offsets 0, 1, 2, ... times
// stride. If makelinear is false, the list elements are in a scrambled order 
// that is intended to defeat any cache prefetching hardware. See the POLY8 
// discussion above.
//
// This routine is not intended to be particularly fast; it is called just once
//
// Returns a pointer to the first element of the list
Pair* MakeLongList(uint8* ptr, int bytesize, int bytestride, bool makelinear) {
  // Make an array of 256 mixed-up offsets
  // 0, ff, e3, db, ... 7b, f6, f1
  int mixedup[256];
  // First element
  mixedup[0] = 0;
  // 255 more elements
  uint8 x = POLYINIT8;
  for (int i = 1; i < 256; ++i) {
    mixedup[i] = x;
    x = POLYSHIFT8(x);
  }

  Pair* pairptr = reinterpret_cast<Pair*>(ptr);
  int element_count = bytesize / bytestride;
  // Make sure next element is in different DRAM row than current element
  int extrabit = makelinear ? 0 : (1 << 14);
  // Fill in N-1 elements, each pointing to the next one
  for (int i = 1; i < element_count; ++i) {
    // If not linear, there are mixed-up groups of 256 elements chained together
    int nextelement = makelinear ? i : (i & ~0xff) | mixedup[i & 0xff];
    Pair* nextptr = reinterpret_cast<Pair*>(ptr + ((nextelement * bytestride) ^ extrabit));
    pairptr->next = nextptr;
    pairptr->data = 0;
    pairptr = nextptr;
  }
  // Fill in Nth element
  pairptr->next = NULL;
  pairptr->data = 0;

  return reinterpret_cast<Pair*>(ptr);
}

// Read all the bytes
void TrashTheCaches(const uint8* ptr, int bytesize) {
  // Fill up array with pseudo-random nonzero values
  const uint64* uint64ptr = reinterpret_cast<const uint64*>(ptr);
  int wordcount = bytesize >> 3;
  uint64 sum = 0;
  for (int i = 0; i < wordcount; ++i) {
    sum += uint64ptr[i];
  }

  // Make sum live so compiler doesn't delete the loop
  if (gNeverZero == 0) {fprintf(stdout, "sum = %lld\n", sum);}
} __attribute__((optimize(0)))


int64 ScrambledLoads(const Pair* pairptr, int count) {
  // Unroll four times to attempt to reduce loop overhead in timing
  int64 startcy = GetCycles();
  for (int i = 0; i < (count >> 2); ++i) {
    if (pairptr != NULL) {
      pairptr = pairptr->next;
    }
    if (pairptr != NULL) {
      pairptr = pairptr->next;
    }
    if (pairptr != NULL) {
      pairptr = pairptr->next;
    }
    if (pairptr != NULL) {
      pairptr = pairptr->next;
    }
  }
  int64 stopcy = GetCycles();
  int64 elapsed = stopcy - startcy;	// cycles
  // Make final pairptr live so compiler doesn't delete the loop
  if (gNeverZero == 0) {fprintf(stdout, "pairptr->data = %lld\n", pairptr->data);}
  return elapsed / count;
} __attribute__((optimize(0)))

// 2024, we only change the FindCacheSizes() for detecting non-power-of-2 cache.
void  FindCacheSizes(uint8* ptr, int kMaxArraySize, int linesize) {
  bool makelinear = false;
  const Pair* pairptr = MakeLongList(ptr, kMaxArraySize, linesize, makelinear);
// Here, this data is extracted from the website https://www.techpowerup.com/cpu-specs/?mfgr=Intel&sort=name, we get all the possible L3 cache size.
  double Possible_Cache_Size[] = {
    1.0, 1.5, 2.0, 3.0, 4.0, 5.0, 6.0, 8.0, 8.25, 9.0, 10.0, 11.0, 12.0, 13.75, 14.0, 15.0, 16.0, 16.5, 18.0, 19.25, 20.0, 22.0, 22.5, 24.0, 24.75, 25.0, 26.25, 27.5, 30.0, 30.25, 32.0, 
  33.0, 33.75, 35.0, 35.75, 36.0, 37.5, 38.5, 39.0, 40.0, 42.0, 45.0, 48.0, 52.5, 54.0, 55.0, 57.0, 60.0, 64.0, 
  67.5, 71.5, 75.0, 77.0, 82.5, 96.0, 
  97.5, 105.0, 112.5, 128.0, 160.0, 180.0, 192.0, 250.0, 256.0, 260.0, 300.0, 320.0, 384.0, 768.0, 1152.0
  };   //In MB, add any if there exist other new L3 cache size
  int size = sizeof(Possible_Cache_Size) / sizeof(Possible_Cache_Size[0]);
  int Converted_Cache_Size[size]; // Create an array of the same size
  for(int j = 0; j < size; j++) {
      Converted_Cache_Size[j] = (int)(Possible_Cache_Size[j]*1048576/linesize); // Convert MB into B, 1024*1024=1048576, then convert into count.
  }

  // Now loading the datasize into the cache based on the database.
  // Iterates 30 times to denoise.
  for(int i=0; i<20; ++i){
    for (int j = 0; j <= size-1; j ++) {
    // Try to force the data we will access out of the caches
      std::cout << Possible_Cache_Size[j] << ", ";
      TrashTheCaches(ptr, kMaxArraySize);
      for (int t = 0; t < 3; ++t) {
        int64 cyclesperload = ScrambledLoads(pairptr, Converted_Cache_Size[j]);
        std::cout << cyclesperload << ", ";
      }
      int64 cyclesperload = ScrambledLoads(pairptr, Converted_Cache_Size[j]);
      std::cout << cyclesperload << std::endl;
    }
  }
} __attribute__((optimize(0)))

int main (int argc, char* argv[]) {
  int default_cache_line_size = 64;
  int linesize = default_cache_line_size;

  for (int i = 1; i < argc; ++i) {
    if (std::strncmp(argv[i], "--cache_line_size=", 18) == 0) {
      std::string arg_value = argv[i] + 18; // Extract the value part
      if (!arg_value.empty()) {
        linesize = std::atoi(arg_value.c_str());
      }
    }
  }

  gNeverZero = time(NULL);
  uint8* rawptr;
  uint8* ptr = AllocPageAligned(kMaxArraySize, &rawptr);

  FindCacheSizes(ptr, kMaxArraySize, linesize);

  free(rawptr);
  return 0;
}

