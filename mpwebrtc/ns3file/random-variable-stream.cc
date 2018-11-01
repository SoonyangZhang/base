#include "random-variable-stream.h"
#include "ns3-assert.h"
#include "rng-stream.h"
#include "rng-seed-manager.h"
namespace ns3{
RandomVariableStream::RandomVariableStream()
  : m_rng (0)
{
}
RandomVariableStream::~RandomVariableStream()
{
  delete m_rng;
}	
void
RandomVariableStream::SetAntithetic(bool isAntithetic)
{
  m_isAntithetic = isAntithetic;
}
bool
RandomVariableStream::IsAntithetic(void) const
{
  return m_isAntithetic;
}
void
RandomVariableStream::SetStream (int64_t stream)
{
  NS_ASSERT (stream >= -1);
  delete m_rng;
  if (stream == -1)
    {
      // The first 2^63 streams are reserved for automatic stream
      // number assignment.
      uint64_t nextStream = RngSeedManager::GetNextStreamIndex ();
      NS_ASSERT(nextStream <= ((1ULL)<<63));
      m_rng = new RngStream (RngSeedManager::GetSeed (),
                             nextStream,
                             RngSeedManager::GetRun ());
    }
  else
    {
      // The last 2^63 streams are reserved for deterministic stream
      // number assignment.
      uint64_t base = ((1ULL)<<63);
      uint64_t target = base + stream;
      m_rng = new RngStream (RngSeedManager::GetSeed (),
                             target,
                             RngSeedManager::GetRun ());
    }
  m_stream = stream;
}
int64_t
RandomVariableStream::GetStream(void) const
{
  return m_stream;
}

RngStream *
RandomVariableStream::Peek(void) const
{
  return m_rng;
}
UniformRandomVariable::UniformRandomVariable ()
{
}
double 
UniformRandomVariable::GetMin (void) const
{
  return m_min;
}
double 
UniformRandomVariable::GetMax (void) const
{
  return m_max;
}
double 
UniformRandomVariable::GetValue (double min, double max)
{
  double v = min + Peek ()->RandU01 () * (max - min);
  if (IsAntithetic ())
    {
      v = min + (max - v);
    }
  return v;
}
uint32_t 
UniformRandomVariable::GetInteger (uint32_t min, uint32_t max)
{
  NS_ASSERT (min <= max);
  return static_cast<uint32_t> ( GetValue ((double) (min), (double) (max) + 1.0) );
}

double 
UniformRandomVariable::GetValue (void)
{
  return GetValue (m_min, m_max);
}
uint32_t 
UniformRandomVariable::GetInteger (void)
{
  return (uint32_t)GetValue (m_min, m_max + 1);
}
}
