#include "pow.h"

#define PRIME POW_LIMIT

long int pow_hash(long int x) {
  long int result = (x * BIG_X + BIG_Y) % PRIME;
  return result;
}