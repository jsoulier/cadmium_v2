#ifndef CADMIUM_WILDFIRE_SIMULATION_PROFILE_HPP_
#define CADMIUM_WILDFIRE_SIMULATION_PROFILE_HPP_
#if 1 // [wildfire_simulation]

#if USE_TRACY
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

#endif // [wildfire_simulation]
#endif // CADMIUM_WILDFIRE_SIMULATION_PROFILE_HPP_
