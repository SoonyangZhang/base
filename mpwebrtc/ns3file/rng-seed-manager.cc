#include "rng-seed-manager.h"
namespace ns3{
static uint32_t rng_seed=1;
static uint64_t rng_run=1;
static uint64_t g_nextStreamIndex = 0;
void RngSeedManager::SetRun(uint64_t run){
    rng_run=run;
}
void RngSeedManager::SetSeed(uint32_t seed){
    rng_seed=seed;
}
uint64_t RngSeedManager::GetRun (void){
    return rng_run;
}
uint32_t RngSeedManager::GetSeed(void){
    return rng_seed;
}
uint64_t RngSeedManager::GetNextStreamIndex (void)
{
  uint64_t next = g_nextStreamIndex;
  g_nextStreamIndex++;
  return next;
}
}

