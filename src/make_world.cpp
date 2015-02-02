#include "heat.hpp"

#include <cstdlib>

int main(int argc, char *argv[])
{
	unsigned n=128;
	float alpha=0.1;
	bool binary=false;
	
	if(argc>1){
		n=atoi(argv[1]);
	}
	if(argc>2){
		alpha=(float)strtod(argv[2],NULL);
	}
	if(argc>3){
		if(atoi(argv[3]))
			binary=true;
	}
	
	try{
		hpce::world_t world=hpce::MakeTestWorld(n, alpha);
		
		hpce::SaveWorld(std::cout, world, binary);
	}catch(const std::exception &e){
		std::cerr<<"Exception : "<<e.what()<<std::endl;
		return 1;
	}
		
	return 0;
}
