#ifndef CADMIUM_CORE_PROFILE_PROFILE_HPP_
#define CADMIUM_CORE_PROFILE_PROFILE_HPP_
#if 1 // [cadmium_v2]

#if TRACY_ENABLE
#include <tracy/Tracy.hpp>
#define CADMIUM_PROFILE_TAG ZoneScoped
#define CADMIUM_PROFILE_TAG_BLOCK(name) ZoneScopedN(name)
#define CADMIUM_PROFILE_FRAME FrameMark
#define CADMIUM_PROFILE_PLOT(name, value) TracyPlot(name, value)
#else
#define CADMIUM_PROFILE_TAG
#define CADMIUM_PROFILE_TAG_BLOCK(name)
#define CADMIUM_PROFILE_FRAME
#define CADMIUM_PROFILE_PLOT(name, value)
#endif

#endif // [cadmium_v2]
#endif // CADMIUM_CORE_PROFILE_PROFILE_HPP_
