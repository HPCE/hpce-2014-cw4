#include "heat.hpp"

#include <cstdlib>

int main(int argc, char *argv[])
{
	std::string dstFile="-"; // stdout
	
	if(argc>1){
		dstFile=argv[1];
	}
	
	try{
		hpce::world_t world=hpce::LoadWorld(std::cin);
		std::cerr<<"Loaded world with w="<<world.w<<", h="<<world.h<<std::endl;
		
		std::cerr<<"Rendering to "<<dstFile<<std::endl;
		hpce::RenderWorld(dstFile, world);
	}catch(const std::exception &e){
		std::cerr<<"Exception : "<<e.what()<<std::endl;
		return 1;
	}
		
	return 0;
}
