#ifndef hpce_heat_hpp
#define hpce_heat_hpp

// Stop windows redifining min and max
#define NOMINMAX

#include <vector>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstdint>

namespace hpce{
	
	//! Each cell has specific fixed properties, as well as its current temperature
	/*! The uint32_t part is C++11 syntax to force the underlying type to be of a known size */
	typedef enum : uint32_t{
		Cell_Fixed			=0x1,	//! Indicates cell maintains fixed temperature
		Cell_Insulator	=0x2	//! Indicates heat does not flow across this cell (it is non conductive
	}cell_flags_t;
	
	//! Captures the description of a world, and it's current state
	struct world_t
	{
		// Fixed properties of the world
		unsigned w;	//! Number of cells across
		unsigned h;	//! Number of cells down
		float alpha;	//! Amount of heat that leaks to/from adjacent conductive cells
		std::vector<cell_flags_t> properties;	//! Fixed properties of each cell
		
		// Dynamic properties of the world
		float t;	//! Current world time
		std::vector<float> state;		//! Dynamic state of the world
	};
	
	//! Create a square world with a standardised "slalom track"
	world_t MakeTestWorld(unsigned n, float alpha);
	
	//! Save the give world to a file
	/*! \param binary If true, save in a faster but less readable format */
	void SaveWorld(std::ostream &dst, const world_t &world, bool binary=false);
	
	//! Read a world from a file
	world_t LoadWorld(std::istream &src);
	
	//! Render the world as a bitmap to the specified file
	/*! \param fileName Either the name of the file, or "-" for stdout
	*/
	void RenderWorld(const std::string &fileName, const world_t &world);
	
	//! Reference world stepping program
	/*! \param dt Amount to step the world by.  Note that large steps will be unstable.
		\param n Number of times to step
		\note Total change in world time will be dt*n
	*/
	void StepWorld(world_t &world, float dt, unsigned n);
};

#endif
