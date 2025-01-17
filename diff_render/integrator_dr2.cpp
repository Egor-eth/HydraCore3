#include "integrator_dr.h"
#include "utils.h"

#include "include/cmaterial.h"
#include "include/cmat_gltf.h"
#include "include/cmat_conductor.h"
#include "include/cmat_glass.h"
#include "include/cmat_diffuse.h"
#include "include/cmat_plastic.h"

#include <chrono>
#include <string>

#include <omp.h>

#include "Image2d.h"
using LiteImage::Image2D;
using LiteImage::Sampler;
using LiteImage::ICombinedImageSampler;
using namespace LiteMath;

void IntegratorDR::RecordPixelRndIfNeeded(float2 offsets, float u)
{
  auto cpuThreadId = omp_get_thread_num();
  int size = int(m_recorded[cpuThreadId].perBounceRands.size());
  m_recorded[cpuThreadId].perBounceRands[size - LENS_RANDS + 0] = offsets.x;
  m_recorded[cpuThreadId].perBounceRands[size - LENS_RANDS + 1] = offsets.y;
  m_recorded[cpuThreadId].perBounceRands[size - LENS_RANDS + 2] = u;
}

void IntegratorDR::RecordRayHitIfNeeded(uint32_t bounceId, CRT_Hit hit)
{
  auto cpuThreadId = omp_get_thread_num();
  m_recorded[cpuThreadId].perBounce[bounceId].hit = hit;
}

void IntegratorDR::RecordShadowHitIfNeeded(uint32_t bounceId, bool inShadow)
{
  auto cpuThreadId = omp_get_thread_num();
  m_recorded[cpuThreadId].perBounce[bounceId].inShadow = inShadow ? 1 : 0;
}

void IntegratorDR::RecordLightRndIfNeeded(uint32_t bounceId, int lightId, float2 rands)
{
  auto cpuThreadId = omp_get_thread_num();
  m_recorded[cpuThreadId].perBounceLightId[bounceId] = lightId;
  m_recorded[cpuThreadId].perBounceRands[bounceId*RND_PER_BOUNCE + RND_LTG_ID + 0] = rands.x;
  m_recorded[cpuThreadId].perBounceRands[bounceId*RND_PER_BOUNCE + RND_LTG_ID + 1] = rands.y;
}

void IntegratorDR::RecordMatRndNeeded(uint32_t bounceId, float4 rands)
{
  auto cpuThreadId = omp_get_thread_num();
  m_recorded[cpuThreadId].perBounceRands[bounceId*RND_PER_BOUNCE + RND_MTL_ID + 0] = rands.x;
  m_recorded[cpuThreadId].perBounceRands[bounceId*RND_PER_BOUNCE + RND_MTL_ID + 1] = rands.y;
  m_recorded[cpuThreadId].perBounceRands[bounceId*RND_PER_BOUNCE + RND_MTL_ID + 2] = rands.z;
  m_recorded[cpuThreadId].perBounceRands[bounceId*RND_PER_BOUNCE + RND_MTL_ID + 3] = rands.w;
}

void IntegratorDR::RecordBlendRndNeeded(uint32_t bounceId, uint layer, float rand)
{
  auto cpuThreadId = omp_get_thread_num();
  m_recorded[cpuThreadId].perBounceRands[bounceId*RND_PER_BOUNCE + RND_BLD_ID + layer] = rand;

}

void IntegratorDR::GetExecutionTime(const char* a_funcName, float a_out[4])
{
  if(std::string(a_funcName) == "PathTraceDR" || std::string(a_funcName) == "PathTraceDRBlock")
    a_out[0] = diffPtTime;
  else 
    Integrator::GetExecutionTime(a_funcName, a_out);
}

