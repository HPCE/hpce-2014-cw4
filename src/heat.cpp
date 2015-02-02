#include "heat.hpp"

#include <stdexcept>
#include <cmath>
#include <cstdint>
#include <memory>
#include <cstdio>
#include <string>

namespace hpce{
	
//! Create a square world with a standardised "slalom track"
world_t MakeTestWorld(unsigned n, float alpha)
{	
	std::vector<cell_flags_t> properties(n*n, (cell_flags_t)0);
	
	// Top, bottom, left, right boundary
	for(unsigned i=0;i<n;i++){
		properties[0*n+i]=Cell_Insulator;
		properties[(n-1)*n+i]=Cell_Insulator;
		properties[i*n+0]=Cell_Insulator;
		properties[i*n+(n-1)]=Cell_Insulator;
	}
	
	// Setup the corridors
	unsigned low=(unsigned)std::floor(n*1.0/3);
	unsigned mid=(unsigned)std::floor(n/2.0);
	unsigned high=(unsigned)std::floor(n*2.0/3);
	
	// Horizontal branches
	for(unsigned i=0;i<high;i++){
		properties[low*n+i]=Cell_Insulator;
	}
	for(unsigned i=low;i<n;i++){
		properties[high*n+i]=Cell_Insulator;
	}
	// Vertical spurs
	for(unsigned i=low;i<mid;i++){
		properties[i*n+high]=Cell_Insulator;
	}
	for(unsigned i=mid;i<high;i++){
		properties[i*n+low]=Cell_Insulator;
	}
	
	// Create state, all intially at ambient temperature
	std::vector<float> state(n*n, 0.0f);
	
	// Create a band of constant heart source along the top
	for(unsigned x=1;x<n-1;x++){
		// Heat source
		state[1*n+x]=1.0f;
		properties[1*n+x]=Cell_Fixed;
	}
	// And a point heat-sink in the mid bottom
	state[(n-3)*n+mid]=0.0f;
	properties[(n-3)*n+mid]=Cell_Fixed;
	
	// Now populate the actual world description
	world_t world;
	world.w=n;
	world.h=n;
	world.alpha=alpha;
	world.properties=properties;
	world.t=0.0f;	// Haven't started yet
	world.state=state;
	
	return world;
}

//! Save the give world to a file
void SaveWorld(std::ostream &dst, const world_t &world, bool binary)
{	
	if(binary){
		dst<<"HPCEHeatWorldV0Binary"<<std::endl;
	}else{
		dst<<"HPCEHeatWorldV0"<<std::endl;
	}
	dst<<world.w<<" "<<world.h<<" "<<world.alpha<<std::endl;
	
	dst<<"-";
	if(!binary){
		dst<<std::endl;
	}
	
	for(unsigned y=0;y<world.h;y++){
		if(binary){
			dst.write((char*)&world.properties[y*world.w], world.w*4);
		}else{
			for(unsigned x=0;x<world.w;x++){
				dst<<" "<<world.properties[y*world.w+x];
			}
			dst<<std::endl;
		}
	}
	
	dst<<"-";
	if(!binary){
		dst<<std::endl;
	}
	
	// Save the output modifiers
	std::ios fmt(NULL);
	fmt.copyfmt(dst);
	
	dst<<std::fixed;	// Record with absolute precision
	dst.precision(8);	// Want the state recorded with similar accuracy to a float
	// Note that by recording in text rather than binary, we'll see an expansion in data
	// size of around 3 times, and reading/writing will be much slower than for binary.
	
	for(unsigned y=0;y<world.h;y++){
		if(binary){
			dst.write((char*)&world.state[y*world.w], world.w*4);
		}else{
			for(unsigned x=0;x<world.w;x++){
				dst<<" "<<world.state[y*world.w+x];
			}
			dst<<std::endl;
		}
	}
	
	dst.copyfmt(fmt);
	
	dst<<"End"<<std::endl;
}

//! Read a world from a file
world_t LoadWorld(std::istream &src)
{
	bool binary=false;
	
	std::string header;
	src>>header;
	if(header=="HPCEHeatWorldV0"){
		binary=false;
	}else if(header=="HPCEHeatWorldV0Binary"){
		binary=true;
	}else{
		throw std::invalid_argument("LoadWorld : File does not start with HPCEHeatWorldV0.");
	}
	
	world_t world;
	
	src>>world.w>>world.h>>world.alpha;
	if(!src.good())
		throw std::invalid_argument("LoadWorld : Corrupt input file, couldn't write initial world state (width, height, alpha).");
	
	world.properties.resize(world.w*world.h);
	world.state.resize(world.w*world.h);
	
	char delim;
	src>>delim;
	if(delim!='-'){
		throw std::invalid_argument("LoadWorld : Corrupt input file, missing hyphen before properties array.");
	}
	
	for(unsigned y=0;y<world.h;y++){
		if(binary){
			src.read((char*)&world.properties[y*world.w], world.w*4);
			for(unsigned x=0;x<world.w;x++){
				unsigned flags=world.properties[y*world.w+x];
				if((flags!=0) && (flags!=Cell_Insulator) && (flags!=Cell_Fixed)){
					std::cerr<<"y="<<y<<", x="<<x<<", flags="<<flags<<"\n";
					throw std::invalid_argument("LoadWorld : Unknown flags for cell.");
				}
			}
		}else{
			for(unsigned x=0;x<world.w;x++){
				unsigned flags;
				src>>flags;
				if((flags!=0) && (flags!=Cell_Insulator) && (flags!=Cell_Fixed))
					throw std::invalid_argument("LoadWorld : Unknown flags for cell.");
				world.properties[y*world.w+x]=(cell_flags_t)flags;
			}
		}
	}
	if(!src.good())
		throw std::invalid_argument("LoadWorld : Corrupt input file, one or more elements of properties could not be read.");
	
	src>>delim;
	if(delim!='-'){
		throw std::invalid_argument("LoadWorld : Corrupt input file, missing hyphen before state array.");
	}
	
	for(unsigned y=0;y<world.h;y++){
		if(binary){
			src.read((char*)&world.state[y*world.w], world.w*4);
			for(unsigned x=0;x<world.w;x++){
				float temp=world.state[y*world.w+x];
				if(temp<0 || temp>1)
					throw std::invalid_argument("LoadWorld : Corrupt input file, temperature out of range.");
			}
		}else{
			for(unsigned x=0;x<world.w;x++){
				float temp;
				src>>temp;
				if(temp<0 || temp>1)
					throw std::invalid_argument("LoadWorld : Corrupt input file, temperature out of range.");
				world.state[y*world.w+x]=temp;
			}
		}
	}
	if(!src.good())
		throw std::invalid_argument("LoadWorld : Corrupt input file, one or more elements of state could not be read.");
	
	src>>header;
	if(header!="End"){
		throw std::invalid_argument("LoadWorld : Corrupt input file, missing 'End' to terminate world description.");
	}
	
	return world;
}

void RenderWorld(const std::string &fileName, const world_t &world)
{
	// The solution to doing BITMAPINFOHEADER etc. without being platform-specific
	// comes from:
	//   http://stackoverflow.com/a/18675807
	// In practise you would never do this, but it's the easiest way to stay platform independent fo
	// coursework purposes.

	uint8_t file[14] = {
		'B','M', // magic
		0,0,0,0, // size in bytes
		0,0, // app data
		0,0, // app data
		40+14,0,0,0 // start of data offset
	};
	uint8_t info[40] = {
		40,0,0,0, // info hd size
		0,0,0,0, // width
		0,0,0,0, // heigth
		1,0, // number color planes
		24,0, // bits per pixel
		0,0,0,0, // compression is none
		0,0,0,0, // image bits size
		0x13,0x0B,0,0, // horz resoluition in pixel / m
		0x13,0x0B,0,0, // vert resolutions (0x03C3 = 96 dpi, 0x0B13 = 72 dpi)
		0,0,0,0, // #colors in pallete
		0,0,0,0, // #important colors
		};

	unsigned w=world.w;
	unsigned h=world.h;

	unsigned padSize  = (4-w%4)%4;
	unsigned sizeData = w*h*3 + h*padSize;
	unsigned sizeAll  = sizeData + sizeof(file) + sizeof(info);

	file[ 2] = (uint8_t)( sizeAll    );
	file[ 3] = (uint8_t)( sizeAll>> 8);
	file[ 4] = (uint8_t)( sizeAll>>16);
	file[ 5] = (uint8_t	)( sizeAll>>24);
		
	info[ 4] = (uint8_t)( w   );
	info[ 5] = (uint8_t)( w>> 8);
	info[ 6] = (uint8_t)( w>>16);
	info[ 7] = (uint8_t)( w>>24);

	info[ 8] = (uint8_t)( h    );
	info[ 9] = (uint8_t)( h>> 8);
	info[10] = (uint8_t)( h>>16);
	info[11] = (uint8_t)( h>>24);

	info[24] = (uint8_t)( sizeData    );
	info[25] = (uint8_t)( sizeData>> 8);
	info[26] = (uint8_t)( sizeData>>16);
	info[27] = (uint8_t)( sizeData>>24);
		
	// End stackoverflow excerpt

	FILE *dst=stdout;
	if(fileName!="-"){
		dst=fopen(fileName.c_str(), "wb");
		if(dst==0)
			throw std::runtime_error("RenderWorld : Couldn't open destination file.");
	}
	try{
		if(sizeof(file)!=fwrite(file, 1, sizeof(file), dst))
			throw std::runtime_error("RenderWorld : Couldn't write bitmap header.");
		
		if(sizeof(info)!=fwrite(info, 1, sizeof(info), dst))
			throw std::runtime_error("RenderWorld : Couldn't write bitmap info.");
		
		std::vector<uint8_t> scanline(w*3+padSize, 0);
		
		for(unsigned y=0;y<h;y++){
			uint8_t *pDst=&scanline[0];
			for(unsigned x=0;x<w;x++){
				unsigned index=y*w+x;
				if(world.properties[index]&Cell_Insulator){
					*pDst++ = 0;
					*pDst++ = 255;
					*pDst++ = 0;
				}else{
					uint8_t heat=(uint8_t)(world.state[index]*255);
					*pDst++ = (255-heat);	// Blue
					*pDst++ = 0; // Green
					*pDst++ = heat; // Red
				}
			}
			if(scanline.size()!=fwrite(&scanline[0], 1, scanline.size(), dst))
				throw std::runtime_error("RenderWorld : Couldn't write scanline.");
		}
		
		if(dst!=stdout)
			fclose(dst);
	}catch(...){
		if(dst!=stdout)
			fclose(dst);
	}
}

//! Reference world stepping program
/*! \param dt Amount to step the world by.  Note that large steps will be unstable.
	\param n Number of times to step the world
	\note Overall time increment will be n*dt
*/
void StepWorld(world_t &world, float dt, unsigned n)
{
	unsigned w=world.w, h=world.h;
	
	float outer=world.alpha*dt;		// We spread alpha to other cells per time
	float inner=1-outer/4;				// Anything that doesn't spread stays
	
	// This is our temporary working space
	std::vector<float> buffer(w*h);
	
	for(unsigned t=0;t<n;t++){
		for(unsigned y=0;y<h;y++){
			for(unsigned x=0;x<w;x++){
				unsigned index=y*w + x;
				
				if((world.properties[index] & Cell_Fixed) || (world.properties[index] & Cell_Insulator)){
					// Do nothing, this cell never changes (e.g. a boundary, or an interior fixed-value heat-source)
					buffer[index]=world.state[index];
				}else{
					float contrib=inner;
					float acc=inner*world.state[index];
					
					// Cell above
					if(! (world.properties[index-w] & Cell_Insulator)) {
						contrib += outer;
						acc += outer * world.state[index-w];
					}
					
					// Cell below
					if(! (world.properties[index+w] & Cell_Insulator)) {
						contrib += outer;
						acc += outer * world.state[index+w];
					}
					
					// Cell left
					if(! (world.properties[index-1] & Cell_Insulator)) {
						contrib += outer;
						acc += outer * world.state[index-1];
					}
					
					// Cell right
					if(! (world.properties[index+1] & Cell_Insulator)) {
						contrib += outer;
						acc += outer * world.state[index+1];
					}
					
					// Scale the accumulate value by the number of places contributing to it
					float res=acc/contrib;
					// Then clamp to the range [0,1]
					res=std::min(1.0f, std::max(0.0f, res));
					buffer[index] = res;
					
				} // end of if(insulator){ ... } else {
			}  // end of for(x...
		} // end of for(y...
		
		// All cells have now been calculated and placed in buffer, so we replace
		// the old state with the new state
		std::swap(world.state, buffer);
		// Swapping rather than assigning is cheaper: just a pointer swap
		// rather than a memcpy, so O(1) rather than O(w*h)
	
		world.t += dt; // We have moved the world forwards in time
		
	} // end of for(t...
}

	
}; // namepspace hpce
