#pragma once
#include <cstdint>

namespace cafe::sndcore2
{

static constexpr auto AXNumTvDevices = 1u;
static constexpr auto AXNumTvChannels = 6u;
static constexpr auto AXNumTvBus = 4u;

static constexpr auto AXNumDrcDevices = 2u;
static constexpr auto AXNumDrcChannels = 4u;
static constexpr auto AXNumDrcBus = 4u;

static constexpr auto AXNumRmtDevices = 4u;
static constexpr auto AXNumRmtChannels = 1u;
static constexpr auto AXNumRmtBus = 1u;

static constexpr auto AXMaxNumVoices = 96u;

} // namespace cafe::sndcore2
