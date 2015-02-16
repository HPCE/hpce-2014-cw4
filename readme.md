Overal Goals
============

The overall goals of this coursework are to:

- Develop familiarity with building OpenCL programs

- Follow a simple methodology for isolating the  parts of an existing
	program which can move to a kernel

- Understand the OpenCL primitives needed to create and execute a kernel

- Examine and remove some of the communications bottlenecks which
	can reduce performance

- Look at some simple techniques for improving GPU performance

- Explore remote GPU instances via AWS

This coursework alone is not intended to make you a GPU
expert. You should know how to create an OpenCL program
from scratch by the end, but the performance you get
may not be be as high as a TBB version.

Checking your OpenCL environment
================================

Before writing any code, it is worth checking your OpenCL environment
and SDK, just to check that you have the right devices and things
installed. There is a file called `src/test_opencl.cpp`, which is 
a simple, but complete, OpenCL program. It doesn't do a lot, but
it does allocate GPU buffers and try to execute a kernel, which
is enough to tell whether things are up and running.

Compile the program, and make sure you can both build
and execute it. You may need to fiddle with your
include and link directories to make it build, and
find/download an SDK. While an OpenCL run-time is
installed in most systems, there is not often no SDK
installed.

SDKs and Run-times
------------------

As with TBB, you need two parts: the OpenCL SDK, which gives
the headers and libraries you need to compile your programs;
and the OpenCL run-time, which provides access to the
underlying compute resource.

### Cross-platform

@farrell236 was nice enough to contribute the [CMakeLists.txt](CMakeLists.txt),
which is a way of getting [cross-platform builds with CMake](https://github.com/HPCE/hpce-2014-cw4/issues/5).

### Windows

There are a number of SDKs available that can be downloaded,
from AMD, Intel, NVidia, and so on. These SDKs can be
pretty big, so you may want to use the extremely minimal
version included in this distribution. This just contains
the headers and libraries, and is appropriate for use in
machines you don't control, where OpenCL is installed, but
the SDK is not.

A subset of lab machines on level 5 have OpenCL capable
GPUs, plus the drivers installed. However, they do not
have any SDKs.

_Note: some [problems compiling for windows](https://github.com/HPCE/hpce-2014-cw4/issues/2). If
you're seeing something similar, it would be useful to know what the
common causes or problems are._

### Linux

You should be able to get a working OpenCL driver via your
GPU vendor - often that will come with an SDK as well. If
you don't have a GPU, or it isn't supported, you can try
using a software OpenCL implementation. Intel provide a
multi-core version, and the AMD OpenCL run-time also contains
a software version, though you'll need to be root to
install it.

Many DoC machines have NVidia cards, with the OpenCL
drivers installed.

### Mac users

As far as I know all Macs come with OpenCL installed these
days, but the SDK may not be installed. I would imagine you
can bring the SDK over with whatever package managed OSX
has, as Apple are very integrated into the whole OpenCL thing.

Apparently, adding the option:

    -framework OpenCL

to your compilation may help when trying to sort out header
and linker directories.

_Note: Various contributors have suggestions for getting
[compilation working](https://github.com/HPCE/hpce-2014-cw4/issues/4)
under OSX._

### AWS

I've created an image for AWS which has OpenCL set up,
both for software and the GPU. The title of the image
is `HPCE-2014-GPU-Image`, which can be selected when
you launch an AWS instance. The steps for reproducing
are [available](aws_setup.md), but it isn't much fun
recreating it. @schmethan suggests also installing
`eog` and `libcanberra-gtk3-module` for viewing images.

### Debugging

@kronocharia has [some suggestions](https://github.com/HPCE/hpce-2014-cw4/issues/6) on how
to debug programs which are part of a pipeline.

The Heat World code
===================

To make this somewhat realistic, you are working with an
existing proprietary application. This is a very simplified
version of a research problem I once personally had to work
with, where I was given a big lump of code and asked to
make it go faster. The overall structure is a classic
example of a finite-difference or stencil operation, where
you are applying some sort of local smoothing operation
over huge numbers of time-steps. Such stencil calculations are
very common, but different enough that one often can't use
an off-the-shelf piece of software. Application areas where
they arise include:

- Electric and magnetic field models

- Percolation and diffusion of dopants

- Temperature propagation within devices and heat-sinks

- Computational finance

The core of the code is in the two files `include/heat.hpp` and
`src/heat.cpp`, with the header giving the API for creating, storing,
loading, and stepping a model of a heat system.

As well as the core `src/heat.cpp`, there are also a number of
driver programs:

- `src/make_world.cpp` : Creates a new two-dimensional model, and
	gives it an initial state.

- `src/step_world.cpp` : Reads a world from a file, and advances
	it by a chosen number of time-steps.

- `src/render_world.cpp` : Takes the current state of the world,
	renders it to a bitmap.

Each of these source files must be compiled together with `heat.cpp`
to produce an executable. There are no other dependencies on external
libraries.

Use your chosen development environment to compile the three files,
resulting in `make_world`, `step_world` and `render_world` (you
can have them build in whatever directory you like). If you now
do:
	
	make_world 10 0.1

You'll see it create a text description of a heat world. At the
top is the header, describing the dimensions of the world, and
the propagation constant (0.1 in this case). The propagation
constant measures how fast heat migrates between conductive
cells in this world, so a constant of 0.01 would result in slower
movement of heat.

Below the header you'll see a grid of 0, 1, and 2, defining
the shape of the world. Any 2s represent insulator, where
heat cannot flow, while any 1s represents fixed temperature
cells, such as heat-sinks and heat-sources. All the 0s are
"normal" conductive cells.

Below the world grid is the current temperature or state of every
cell in the grid, with values between 0 and 1. After creating
the world it is cold, except for a row of 1s near the top
of the state which are all hot. If you compare these cells
with the world grid, you'll see these 1s are fixed, so they
will keep their value of 1 as time progresses. However,
surrounding cells will change temperature in response to
the cells around them.

Now advance the world by a single time-step of length 0.1 seconds:

	make_world 10 0.1 | step_world 0.1 1

You'll see that the the cells next to the heat source row has
warmed up. The warming is not uniform, as at the far left and
far right the cells are up against insulators, so according
to the simple model using here, they end up slightly warmer.

You can now advance the world by a few seconds:

	make_world 10 0.1 | step_world 0.1 100

You should see that some of the heat is making it's way
around the right hand side, but it can't come through
the insulator across the middle-left. Step forward
by a 1000 seconds:

	make_world 10 0.1 | step_world 0.1 10000

and you'll see the world has largely reached a steady-state,
with high temperatures around the top, then slowly
decreasing temperatures through to the single point heat-sink
at the bottom.

In such problems we are usually interested in looking
at very high spatial resolutions (i.e. lots of cells),
and very fine temporal resolutions. This sort of discretisation
is most accurate when both time and space are very
fine. To visualise much finer-grain scenarios, it is
convenient to view it as a bitmap:

	make_world 100 0.1 | step_world 0.1 100 | render_world dump.bmp

If you look at `dump.bmp`, you should see insulators rendered
in green, and the temperature shown from blue (0) to red (1). However,
over a total time of 0.1*100=10 seconds, very little will have happened.
To see anything happen, you'll need to up the time significantly:

	make_world 100 0.1 | step_world 0.1 100000 | render_world dump.bmp
	
It is getting fairly slow, even at a resolution of 100x100, but in
practise we'd like to increase resolution 10x in all three dimensions
(both spatial axes + time), which increases the compute time by
a factor of 1000.

This type of computation is ideal for a GPU, as it maps very
well to the grid-structure provided by both the hardware
and the programming model.

Preparing the code for OpenCL
=============================

Create a new folder within `src` called `src/your_login`. This is
where your additional code will live (as before, in principle I
should be able to compile everyone in the classes's code together
at the end).

We're going to convert the original C++ code to OpenCL quite
incrementally, keeping a trail of working versions as we go.
You should keep track of this as labels within git, so
that you can always get back to a working version (and
work out what you changed that broke it).
If all you have is the original sequential and the highly optimised
parallel GPU version, then it can be very difficult to see how the
two are actually related.

### Examining the source code

The first thing to do is to look at the source code,
and to identify opportunities for parallelism. If
you open `src/heat.cpp`, you'll see a number of
functions, but the computationally intensive part is
the function `hpce::StepWorld`. This is responsible
for moving the world forwards by `n` steps which each
increment time by `dt`.

Within `StepWorld` there is some initial setup
which allocates buffers and calculates constants,
followed three nested loops. These loops
represent the bulk of the time in the algorithm, and
it should be clear that the execution time complexity
is O(`n`*`w`*`h`). So broadly speaking if we double any _one_
of `n`, `w`, or `h` then the execution time will double, and
if we double _all_ of `n`, `w`, and `h` then the execution
time will be eight times longer.

Recalling your experience with vectorisation in
matlab, you should immediately see opportunities
for parallelism here: all the iterations of the
two inner loops are independent, and so can be
parallelised. However, there is a loop carried
dependency across the time loop, similar to the
erode filter we looked at in lectures. However,
unlike the erode example, where there were multiple
frames which could be pipelined, here we have
a single world to update, so pipelining will not
help.

### Extracting the inner loop to a lambda

First we want to isolate the part of the program
that will be moved to the GPU. Because the GPU works
in a different memory space, it is useful to emulate
that hard boundary in software first, before starting
to convert to kernels.

#### Writing the code

Take the file `src/heat.cpp`, and copy it to a new
file called `src/your_login/step_world_v1_lambda.cpp`.
The original file contains multiple support and IO functions
which we aren't interested in (we'll still be linking with
the original `src/heat.cpp`, so cut out all the functions
which aren't `StepWorld`. Rename the remaining `StepWorld`
to `StepWorldV1Lambda`, and put it in a namespace called
`your_login`. So you should be left with a file which looks
something like:

    #include "heat.hpp"
	
	... // More include files
	
	namespace hpce{
	namespace your_login{
	
	void StepWorldV1Lambda(world_t &world, float dt, unsigned n)
	{
		... // Existing implementation
	}
	
	}; // namespace your_login
	}; // namespace hpce

Our first step is to pull the two inner loops out into
a lambda, starting to isolate the core loops. Pull
the body of the inner-most loop into a seperate
lambda called `kernel_xy`, which should take an
x and y parameter, but capture everything else by
reference (i.e. use the `[&]` lambda form).

The resulting code should look something like:

    std::vector<float> buffer(w*h);
	
    auto kernel_xy = [&](unsigned x, unsigned y)
    {
    	... // Original loop body
    };
	
    for(unsigned t=0;t<n;t++){
        for(unsigned y=0;y<h;y++){
            for(unsigned x=0;x<w;x++){
                kernel_xy(x,y);
            }  // end of for(x...
        } // end of for(y...
		
		... // Rest of code

Note that the lambda has to be declared after
`buffer`, as if you try to declare it before, then
it is impossible to close over the variable - you
can't refer to something that hasn't been named
yet.

#### Building the test framework

To compile this, we need some sort of driver
program to load the world, call our function,
and save the world. Fortunately, we already have
such a program in `src/step_world.cpp`. Take the
`main` from that program and paste it in below
your existing code. Note that it should appear
outside the namespaces, as `main` goes
in the global namespace. Modify the call to `hpce::StepWorld`
to `hpce::your_login::StepWorldV1Lambda`, to ensure
it uses your new function.

Modify your build environment so that `src/heat.cpp` is
compiled together with `src/your_login/step_world_v1_lambda.cpp`
to produce a single executable called `step_world_v1_lambda`.
Again, it is up to you where you choose to have it built.

By making sure the executables have different names
and we can still run old versions, we retain the ability
to select between different implementations. Often
students - I'm sure not you - resort to #ifdef and
commented out code as they develop new versions of
existing code.

Test your program still does the same as the
original implementation in whatever way you
think best. Note that you have both
`step_world` and `step_world_v1_lambda`
available at the same time, and that there
are programs like `diff` (posix) and `FC` (windows)
which will tell you if two text files are
different. In bash this is particularly easy to do,
and it can be incorporated into your makefile if you
wish.

### Making the isolation clean

Your lambda version captures everything by
reference, so within `kernel_xy` there are
references to `world`, `buffer`, and so on.
For the GPU version the kernel cannot make
any references to things which are not explictly
passed as parameters, and the best way of
emulating this is to move the kernel outside
the stepping function into a pure function.
In this context, "pure function" means a function
that is only able to interact with the world
via its parameters, and accesses no global
state or captured variables.

OpenCL also doesn't allow complex C++ data-types
to cross the CPU-GPU boundary, so we'll also
need to convert those to primitive types or
pointers to primitive types. For example, the
kernel currently closes over (captures) `world` which is
of type `world_t`, and `buffer`, which has type
`std::vector<float>`. These need to be turned
into parameters with primitive types and known size, such as
`uint32_t`, `float`, and/or pointers to those
types.

Based on your `step_world_v1_lambda.cpp` code, create
a new file called `step_world_v2_function.cpp` in the
same directory, and rename the inner function to
`hpce::your_login::StepWorldV2Function`. Make sure you
can build and run the file before proceeding.

Currently it will have a lambda called `kernel_xy`; take
the lambda outside the `StepWorldV2Function`, and
make it a new function called `kernel_xy`:

    // Note the change from unsigned -> uint32_t to get a known size type
    void kernel_xy(uint32_t x, uint32_t y)
    {
	    ... // Original lambda body
	}
	
	void StepWorldV2Function(world_t &world, float dt, unsigned n)
	{
	    ... // Original code, now without lambda

If you try compiling this, you'll see that it doesn't
work - you'll get undefined errors about `w`, `world`,
and so on. This is telling you all the information
which will eventually have to be manually copied to
the GPU, and if necessary back again afterwards. When
accelerating for real, this
is a good point to think about communication costs - have
you really chosen the best point
to cut the program at? Are there any things being copied
across which could more cheaply be recalculated within
the kernel?

Resolve all these errors by adding parameters to the
definition of `kernel_xy`, and by adding arguments
to the point where `kernel_xy` is called. For example,
to resolve the error with `w`, you would modify
the declaration:

    void kernel_xy(uint32_t x, uint32_t y, uint32_t w);

and then use:

    for(unsigned x=0;x<w;x++){
		kernel_xy(x,y, w);
	}  // end of for(x...

Note that some things will require conversion from C++
types to primitive types. For example to convert `world.state`,
you'll need to pass a pointer to the data:

    void kernel_xy(uint32_t x, uint32_t y, uint32_t w, const float *world_state);

and:

    for(unsigned x=0;x<w;x++){
		kernel_xy(x,y, w, &world.state[0]);
	}  // end of for(x...

I used "const" on the world_state parameter to indicate that
it is never modified inside the kernel. However, the
converted `buffer` parameter cannot be const, as the kernel
must write to the array to produce output. This process
helps show which arrays must be copied both to and from
the GPU, and which only need to be copied one way.

When it comes to passing the cell properties, note that
cell_flags_t was originally defined to have the underlying
type uint32_t, so you can safely cast to a (const uint32_t *) when
converting parameters.

Keep adding parameters until you have removed all errors,
making sure that you convert all of them to primitive
numbers or pointers to arrays.

Test the function to make sure it still works. If so, we
are now in a position to map it into a CPU kernel.

Intial conversion to OpenCL
===========================

We've now got a version of the code which has
separated the computationally intensive part out
into a function. This function has:

- An iteration identifier (`x`,`y`), which identifies where
  the computation is occuring within the overall parallel
  iteration space. This will become our OpenCL global work id.
  
- A number of scalar (non-pointer) parameters, such
  as `w`, which will be passed by value at the kernel invocation.
  
- A number of non-scalar (pointer) parameters, which will
  need to be manually copied to the GPU before kernel execution,
  passed by reference when we invoke the kernel, and then
  manually copied back after execution finishes.

To convert this to OpenCL, first we'll handle the
host code (the long boring bit), then look at the
GPU kernel. We'll do all this in a file called
`src/your_login/step_world_v3_opencl.cpp`, with
a function called `src::your_login::StepWorldV3OpenCL`.

Create the new file based on the `src/your_login/step_world_v2_function.cpp` code,
and check you can still build and run it. While we're
setting up the OpenCL parts, don't worry too much about
the data going in and out. You can simply test it using
something equivalent to:

    bin/make_world | bin/step_world_v3_opencl > /dev/null

as for a while we'll just look at stderr.

### Opening the context, compiling kernels

Because OpenCL supports many devices, there are
a number of stages involved in starting things
up and connecting to various devices. Before
we even think about executing kernels, we need to:

1. Choose an OpenCL platform, as there might
   be multiple platforms available in one
   machine. For example, there may be both
   Intel and NVidia platforms installed.

2. Select one of the devices exposed by the
   platform. As an example, the AMD platform
   may expose both CPU and GPU OpenCL devices.

3. Create an OpenCL context for the platform,
   which describes the environment within which
   we can launch kernels and manage memory buffers.

4. Create and build our kernels. It is common
   to load and build the GPU kernel code at run-time,
   just before the kernel is first used.

These steps can often be simplified with library
code, but here we'll rely on just the standard
OpenCL C++ wrappers. These bindings come from
`CL/cl.hpp`, and are installed in most recent
OpenCL SDKs, but just in case, there is a version
in the `include` directory too. The functions provided
by this header are all documented at the [Khronos site](http://www.khronos.org/registry/cl/specs/opencl-cplusplus-1.2.pdf).

Add the include at the top of `src/your_login/step_world_v3_opencl.cpp`:

	#define __CL_ENABLE_EXCEPTIONS 
	#include "CL/cl.hpp"

The `__CL_ENABLE_EXCEPTIONS` definition tells the C++
wrapper that it should throw exceptions when things
go wrong, rather than just returning an error code.
This makes it much harder to accidentally ignore steps
which went wrong. 

At this point you may hit an error about `CL/opencl.h` not
being found. Make sure that OpenCL is available, and that
you have appropriately modified your include directories
to point towards the include files in the OpenCL SDK.
Assuming the initial `test_opencl.cpp` program compiled
this should work fine.

#### Choosing a platform

We'll add the generic OpenCL initialisation at the start of
`StepWorldV3OpenCL`, before doing anything with the world-specific
code. First get a list of platforms from OpenCL:

    std::vector<cl::Platform> platforms;
		
	cl::Platform::get(&platforms);
	if(platforms.size()==0)
		throw std::runtime_error("No OpenCL platforms found.");

After this code executes we'll have a list of installed
platforms, which we can print the names of (in production
code you would comment this out or make it optional):

    std::cerr<<"Found "<<platforms.size()<<" platforms\n";
	for(unsigned i=0;i<platforms.size();i++){
		std::string vendor=platforms[i].getInfo<CL_PLATFORM_VENDOR>();
		std::cerr<<"  Platform "<<i<<" : "<<vendor<<"\n";
	}

At this point we could select just the first platform,
but it is nice to be able to provide a way to select
a specific platform. Often this would be by a configuration
file, but we'll do it by an environment variable:

    int selectedPlatform=0;
	if(getenv("HPCE_SELECT_PLATFORM")){
		selectedPlatform=atoi(getenv("HPCE_SELECT_PLATFORM"));
	}
	std::cerr<<"Choosing platform "<<selectedPlatform<<"\n";
	cl::Platform platform=platforms.at(selectedPlatform);    

This means that by default it will select the first platform,
but if we change an environment variable (e.g. `export HPCE_SELECT_PLATFORM=1`
in bash), then the executable will do something different.
While not as good as a configuration file, this makes it
possible to select what happens _without_ recompiling. Again,
I'm sure you don't use #ifdef and commenting to change
functionality, but many less enlightened students do.

We now have a handle to our chosen platform in `platform`. This
handle will automatically be deleted when the object
goes out of scope, so there is no explicit handle management
needed.

#### Choose a device

Within the platform there might be multiple devices, which
we can enumerate:

    std::vector<cl::Device> devices;
	platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);	
	if(devices.size()==0){
		throw std::runtime_error("No opencl devices found.\n");
	}
		
	std::cerr<<"Found "<<devices.size()<<" devices\n";
	for(unsigned i=0;i<devices.size();i++){
		std::string name=devices[i].getInfo<CL_DEVICE_NAME>();
		std::cerr<<"  Device "<<i<<" : "<<name<<"\n";
	}

Depending on your system you may now see two devices. On my
desktop, I get:

	dt10@TOTO /cygdrive/e/_dt10_/documents/teaching/2014/hpce/cw/CW4
    $ bin/make_world.exe 64 | bin/step_world_v3_opencl > /dev/null
    Loaded world with w=64, h=64
    Stepping by dt=0.1 for n=1
    Found 1 platforms
      Platform 0 : Advanced Micro Devices, Inc.
    Choosing platform 0
    Found 2 devices
      Device 0 : Caicos
      Device 1 :         Intel(R) Core(TM) i7-2600 CPU @ 3.40GHz

This shows I have one GPU device (Caicos), and one CPU device.
Within each device there may be multiple CPUs, but here
OpenCL is just listing the physical clusters available.

Despite one of these devices being a CPU, remember that it
is just as valid OpenCL device as a GPU. It is often
not as fast or powerful, but it can still be programmed
in the same way. However, on a multi-core machine the
CPU OpenCL device may in fact be very powerful, with the
ability to exploit wide SIMD instructions. Do not
under-estimate the power of a [c4.8xlarge](http://aws.amazon.com/ec2/instance-types/)
running all 36 cores over vectorisable code, as it can
often beat a GPU.

As before we'll select device 0 by default, but give the
option of changing that via an environment variable:

	int selectedDevice=0;
	if(getenv("HPCE_SELECT_DEVICE")){
		selectedDevice=atoi(getenv("HPCE_SELECT_DEVICE"));
	}
	std::cerr<<"Choosing device "<<selectedDevice<<"\n";
	cl::Device device=devices.at(selectedDevice);

So for example I can switch between device 0 and 1 without
re-compiling:

	dt10@TOTO /cygdrive/e/_dt10_/documents/teaching/2014/hpce/cw/CW4
	$ bin/make_world.exe 64 | bin/step_world_v3_opencl > /dev/null
	Loaded world with w=64, h=64
	Stepping by dt=0.1 for n=1
	Found 1 platforms
	  Platform 0 : Advanced Micro Devices, Inc.
	Choosing platform 0
	Found 2 devices
	  Device 0 : Caicos
	  Device 1 :         Intel(R) Core(TM) i7-2600 CPU @ 3.40GHz
	Choosing device 0

	dt10@TOTO /cygdrive/e/_dt10_/documents/teaching/2014/hpce/cw/CW4
	$ export HPCE_SELECT_DEVICE=1

	dt10@TOTO /cygdrive/e/_dt10_/documents/teaching/2014/hpce/cw/CW4
	$ bin/make_world.exe 64 | bin/step_world_v3_opencl > /dev/null
	Loaded world with w=64, h=64
	Stepping by dt=0.1 for n=1
	Found 1 platforms
	  Platform 0 : Advanced Micro Devices, Inc.
	Choosing platform 0
	Found 2 devices
	  Device 0 : Caicos
	  Device 1 :         Intel(R) Core(TM) i7-2600 CPU @ 3.40GHz
	Choosing device 1

#### Creating a context

To actually make use of the device, we need to create an
OpenCL context, which represents a domain within which
we can create and use kernels and memory buffers. Each
object can only be used within the context it was created
for, and it is unusual to have more than one context
in existence at a time (though it is legal).

Contexts can actually cover multiple devices (though
we won't use that ability here), so to create a
context we pass a vector of devices. Conveniently,
we still have such a vector in `devices` from when
we enumerated the devices, which we can use to
construct the context:

	cl::Context context(devices);

That's about it for creating the context - it doesn't
do a lot, but we need it as a parameter to all the
kernel and memory buffer functions coming up.

#### Loading and building a kernel

The system is now pretty much initialised, and we can
start doing application specific stuff. The first thing
we want to do is create and compile our kernels. Each
kernel is a piece of code compiled for the GPU, which can
then be launched over a particular iteration space. But
before writing the kernel, we'll write the host code
to load and compile it.

Kernel code often exists as text files outside the program,
and it is largely up to the user to get the text of
the source code - OpenCL doesn't know or care where it
comes from. Finding and locating this code at run-time
can be a pain, and it is not uncommon to embed them
as resources within the executable. For the purposes
of this coursework we will assume that all cl code is
in the `src/your_login` directory. So for example, the opencl
code for this particular executable will eventually be
placed at `src/your_login/step_world_v3_kernel.cl`.
However, as a get-out, we will again define an
environment variable called `HPCE_CL_SRC_DIR` which
can be used to override that at run-time.

Having defined where the code will be found, the
first thing to do is load the file contents. This is
trivial, but worth knowing how to do simply, so briefly
read this, then integrate it into your file:

	// To go at the top of the file
	#include <fstream>
	#include <streambuf>

	// To go in hpce::your_login, just above StepWorldV3OpenCL
	std::string LoadSource(const char *fileName)
	{
		// Don't forget to change your_login here
		std::string baseDir="src/your_login";
		if(getenv("HPCE_CL_SRC_DIR")){
			baseDir=getenv("HPCE_CL_SRC_DIR");
		}
		
		std::string fullName=baseDir+"/"+fileName;
		
		// Open a read-only binary stream over the file
		std::ifstream src(fullName, std::ios::in | std::ios::binary);
		if(!src.is_open())
			throw std::runtime_error("LoadSource : Couldn't load cl file from '"+fullName+"'.");
		
		// Read all characters of the file into a string
		return std::string(
			(std::istreambuf_iterator<char>(src)), // Node the extra brackets.
            std::istreambuf_iterator<char>()
		);
	}

We need to collect together sources into a _program_, which
is the unit of compilation in OpenCL. In our case it
will only contain one kernel, but in principle there could
be many kernels, spread across multiple sources. So, returning to
where we left off, just after creating the context:

	std::string kernelSource=LoadSource("step_world_v3_kernel.cl");
	
	cl::Program::Sources sources;	// A vector of (data,length) pairs
	sources.push_back(std::make_pair(kernelSource.c_str(), kernelSource.size()+1));	// push on our single string
	
	cl::Program program(context, sources);
	program.build(devices);

At this point you should be able to compile and run the overall
program, but it should crash because there is no such kernel.

#### Writing a kernel

OpenCL host code (what we were just doing in C++) and
OpenCL kernel code is written in a different language, so
we need to create a new file to represent it. Create
a file called `src/your_login/step_world_v3_kernel.cl`, and
copy the entire `kernel_xy` function into it. You can
leave the original `kernel_xy` function where it is, so
that the software path still works.

Your file `src_cl/your_login/step_world_v3_kernel.cl` should
now look something like (your parameters may be in a different
order):

	void kernel_xy(unsigned x, unsigned y,
		unsigned w,
		float inner, float outer,
		const uint32_t *properties,
		const float *world_state,
		float *buffer
		)
	{
		unsigned index=y*w + x;
		
		... // Rest of code
	}
	
Kernel functions are prefixed with `__kernel`, so as a minimum
step towards creating a working kernel, change the function
type in the cl file to:

	__kernel void kernel_xy(unsigned x, unsigned y, // Rest of kernel...

Try running your C++ program again. It should now be able
to find the file, but then OpenCL will throw an exception
when it tries to compile it. The underlying cause is
syntax errors, just as with a malformed C program, but
here we can't see them.

To expose the specific error messages, go back to the
C++ file and modify the build statement to the following:

	cl::Program program(context, sources);
	try{
		program.build(devices);
	}catch(...){
		for(unsigned i=0;i<devices.size();i++){
			std::cerr<<"Log for device "<<devices[i].getInfo<CL_DEVICE_NAME>()<<":\n\n";
			std::cerr<<program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[i])<<"\n\n";
		}
		throw;
	}
	
This code is catching the error being thrown, and
then extracting the build log for each device. If
you run it again, you should see a whole bunch
of complaints.

You'll see a number of errors, most likely including:

- `uint32_t` is undefined : In the c/c++ world, unsigneds
	and integers have a platform specific width, so
	we chose `uint32_t` to give it a width. However, in
	the OpenCL kernel, the uint type is defined a 32-bit
	integer on all platforms, so we can substitute `uint`
	for `uint32_t`.

- Pointers must have an address space : the compiler
	needs to know where pointers can possibly point,
	and how they might be shared. We'll use global
	memory to start with, so add the `__global` anotation
	to the pointer parameters.

- `Cell_Fixed` is not defined. These definitions were
	not brought over from the C++, so simply add
	enum definitions to the .cl file:
	
		enum cell_flags_t{
			Cell_Fixed		=0x1,
			Cell_Insulator	=0x2
		};

- `std::min` is not defined. OpenCL contains a number of
	equivalent built-ins such as min, max, sin, and cos,
	for use in kernels. You can simply delete the `std::`
	part to get the builtin function.

After dealing with the errors, the code should now
happily build. However, we are not done yet, as unlike
`tbb::parallel_for`, the iteration space for
OpenCL is embedded directly in the language. So
rather than passing `x` and `y` as parameters, we
will need to get the indices out of special indexing
registers.

There are two types of builtin indices:

- `size_t get_local_id(uint dimindx)` : This returns the position within
	the local work-group. Threads within a local work-group can
	communicate via local memory, and the size of the local group is
	limited by hardware resources.

- `size_t get_global_id(uint dimindx)` : This returns the position within
	the entire work-group. Threads in different local work-groups
	cannot communicate via local memory, but can communicate via
	(slower) global memory, and the size of the work-group is almost
	unlimited.

The dimensionality of each id is determined by the dimension
of the iteration space used at launch-time (which is coming up
soon). In this specific case we are using a 2D iteration
space, and our memory pointers are global, so we can extract
the `x` and `y` co-ordinates from the global work-group. As well
as the co-ordinates, the run-time and hardware also know the
sizes of the groups via `get_local_size` and `get_global_size`,
so we can also remove the `w` parameter:

	__kernel void kernel_xy(
		float inner, float outer,
		__global const uint *properties,
		__global const float *world_state,
		__global float *buffer
		)
	{
		uint x=get_global_id(0);
		uint y=get_global_id(1);
		uint w=get_global_size(0);
		
		unsigned index=y*w + x;
		
		... // Original code

In principle, this should now be a fully working OpenCL kernel!

### Creating and executing kernels

We now have all the static infrastructure in place,
with our devices and context open, and our OpenCL
code compiled. At this point we have the choice
of starting up multiple tasks which make use of
the program we compiled, as the OpenCL interface
is inherently multi-threaded, and there is nothing
to stop multiple threads or tasks sharing a device.

However, within each thread we would have to make sure
that all OpenCL tasks are synchronised and
co-ordinated with each other (regardless of what
other threads are doing). The mechanism for
doing that is called a _command queue_, and as
it suggests, it is a queueing or scheduling
structure for issuing commands to the device. These
commands can relate both to data movement between
CPU and GPU, and to the actual execution of kernels
on the GPU. It is essentially a way of building
dependency graphs at run-time, then letting the
GPU execute those graphs in the most efficient
way possible.

To actually execute our kernel, we'll need to follow
these steps:

1. Allocate on-GPU memory buffers for the non-scalar parameters.

2. Bind any fixed input parameters (e.g. `inner`) to the kernel.

3. Create a command queue associated with the context.

4. Copy any fixed input data (e.g. `properties`) to the GPU memory buffers.

5. Bind any dynamic input parameters to the kernel.

6. Copy any input data to the GPU memory buffers.

7. Schedule the kernel onto the command queue. This (finally!) is
	where the actual execution happens.

8. Copy any results back from GPU memory buffers.

#### Allocating buffers

We have three arrays in use while stepping the world:

- `properties` : A read-only array describing the static
	structure of the world. This does not change across
	all iterations within the world (i.e. for multiple
	iterations of the outer t loop).

- `state` : A read-only array containing the current state
	of the world. However, this array does change between
	each iteration.

- `buffer` : A write-only array where we build up the output
	of each iteration.

Each of these arrays uses one `float` or `uint32_t` per cell,
so the total bytes for each buffer is `4*world.w*world.h`.
We can allocate space for those arrays on the GPU's local
memory using `cl::Buffer`. First we'll create the properties
array:

	size_t cbBuffer=4*world.w*world.h;
	cl::Buffer buffProperties(context, CL_MEM_READ_ONLY, cbBuffer);
	cl::Buffer buffState(context, CL_MEM_READ_ONLY, cbBuffer);
	cl::Buffer buffBuffer(context, CL_MEM_WRITE_ONLY, cbBuffer);

So we have now allocated the buffers on the GPU, but don't have
any idea about what data is on them. The flags mentioning READ and WRITE
are hints to the GPU about how the buffers will be used from
the kernel - you can always pass `CL_MEM_READ_WRITE` and not
think about it, but if you can determine how memory will be
accessed, it is possible the OpenCL runtime can optimise memory
transfers.

#### Setting kernel parameters

It is often the case that some kernel parameters are constant
through multiple uses of the kernel, in which case we
can create the kernel and bind those parameters while
setting things up. In our specific case, all our parameters are
actually fixed, so we can set them all up before entering
the loop (in a later optimisation we will have to modify some
of them as we execute).

To set the kernel parameters, we first have to create
an instance of our kernel which we can set the parameters
on and schedule for execution. Just after the buffers, add:

	cl::Kernel kernel(program, "kernel_xy");

This will find the compiled kernel called `kernel_xy` in
the program, and create a new instance of it. We may
wish to have multiple instances of the same kernel in
order to schedule parallel copies with different parameters,
but here we need just one.

Parameters are bound to the kernel using the `setArg` member
function, and are set by index, not name. So we can quickly
go and look in our kernel definition to count off the
indices:

	__kernel void kernel_xy(
		float inner,	// 0
		float outer,	// 1
		__global const uint *properties,	// 2
		__global const float *world_state,	// 3
		__global float *buffer	// 4
	);

Don't worry if your kernel parameters are in a different order,
just note which order they occur in.

Given the order, you can then bind the arguments in the C++ code:

	kernel.setArg(0, inner);
	kernel.setArg(1, outer);
	kernel.setArg(2, buffProperties);
	kernel.setArg(3, buffState);
	kernel.setArg(4, buffBuffer);
	
You'll need to make sure that this occurs _after_ the `inner`
and `outer` variables are declared, but before the loop is
entered.

The `setArg` function is overloaded, so it knows how to deal
with scalar parameters such as `inner` as well as non-scalar
`cl::Buffer`s like `buffState`. However, you cannot bind a raw
c++ pointer, such as a `float *`: at best you will get a compile-time
error, and at worst it will crash unpredictably at run-time.

#### Creating a command queue

This is our final part of our overall application setup
before moving inside the execution loop. As with the context,
this one is quite simple. After setting the kernel
arguments, simply create a command queue:

	cl::CommandQueue queue(context, device);

Note that the command queue is bound to a single specific
device, even though there may be multiple devices in
our context. That is fine for our purposes, and is
fine as long as you only want to work with one OpenCL
device, but if we wanted to use multiple devices,
we would need multiple command queues.

#### Copying over fixed data

The properties array is constant across all iterations, so
we may as well copy it over to the GPU just once, before
starting the iterations. Copies are scheduled to the
command queue using `enqueueWriteBuffer`, which copies a
specified area from host (CPU) memory to device (GPU)
memory:

	queue.enqueueWriteBuffer(buffProperties, CL_TRUE, 0, cbBuffer, &world.properties[0]);

The parameters to the function are:

- `buffProperties` : The GPU buffer we're writing too.

- `CL_TRUE` : A flag to indicate we want synchronous operation, so the
	function will not complete until the copy has finished.

- `0` : The starting offset within the GPU buffer.

- `cbBuffer` : The number of bytes to copy.

- `&world.properties[0]` : Pointer to the data in host memory we want to copy.

Check that your program still compiles and runs. At this point it should
still perform its normal function using software, but we are about to
rip that out.

So assuming everything compiles and runs, we will now move into
the loop over `t`. The part we are going to replace
is the double loop over `y` and `x`, so:

1. Check you know what the two loops are doing, and their
	loop bounds.

2. Delete both inner loops.

You should be left with just a loop over `t`, with a
call to `std::swap` and the time increment at the end. At
this point the program is now broken, until we get
the kernel working.

#### Copying memory buffers

The first thing to do within the loop is to copy the current
state over to the GPU:

	cl::Event evCopiedState;
	queue.enqueueWriteBuffer(buffState, CL_FALSE, 0, cbBuffer, &world.state[0], NULL, &evCopiedState);

Unlike the previous copy, we are doing this one asynchronously, shown
by the second parameter being CL_FALSE rather than CL_TRUE. That means
that the call to `enqueueWriteBuffer` can return even before the
copy has completed. We will have other operations which need the copy
to complete before they can start, so we pass a pointer to `evCopiedState` to
the function. This event can then be passed to any future function
which must wait for this copy, explicitly indicating the
dependency to the OpenCL run-time. Once this write has finished,
the `evCopiedState` even will be signalled, releasing any
waiting tasks.

#### Executing the kernel

Finally! A lot of GPU programming involves setting up a whole
load of infrastructure before actually executing a kernel.
Most of it is boiler-plate, and can be hidden by libraries,
but it is all leading up to this:

	cl::NDRange offset(0, 0);				// Always start iterations at x=0, y=0
	cl::NDRange globalSize(w, h);	// Global size must match the original loops
	cl::NDRange localSize=cl::NullRange;	// We don't care about local size
	
	std::vector<cl::Event> kernelDependencies(1, evCopiedState);
	cl::Event evExecutedKernel;
	queue.enqueueNDRangeKernel(kernel, offset, globalSize, localSize, &kernelDependencies, &evExecutedKernel);

The first part is setting up the iteration space, with the `offset`
specifying that both `x` and `y` start and zero, and `globalSize` saying
that we should loop up to `x`<`w` and `y`<`h`. The localSize parameter
determines clustering of threads into local work-groups, which
we don't yet care about here (as we're only using global work-groups).

The second part establishes the dependencies of the kernel,
by creating a vector of all the things that
must complete before the kernel can run. For this case, there
is only one dependency: the copy must have completed, so the
event `evCopiedState` must have completed. We also produce
an event called `evExecutedKernel`, so that we can tell when
this kernel finishes.

After enqueueing the kernel, we don't know whether the copy
has finished, or the kernel has finished. Maybe both have,
maybe neither has. We cannot assume anything until we
synchronise.

#### Copying the results back

The final job is to copy the results back after the kernel
finishes:

	std::vector<cl::Event> copyBackDependencies(1, evExecutedKernel);
	queue.enqueueReadBuffer(buffBuffer, CL_TRUE, 0, cbBuffer, &buffer[0], &copyBackDependencies);

The copy cannot start until the kernel has finished, so we have
to indicate the depdendency on `evExecutedKernel`. However, after
this completes we need to interact with `buffer`, so we make this
call synchronous using `CL_TRUE`.

And that's it. You should now have (laboriously) accelerated and
existing C++ program. If all has gone well, the program should
correctly spawn the calculation on the CPU, and perform all
steps correctly. But as always: check to be sure.

### Is it fast?

The whole point of this exercise was to make things
faster, and if you time things as they stand, you
may not be that impresssed:

    time (bin/make_world | bin/step_world 0.1 1 > /dev/null)
	time (bin/make_world | bin/$USER/step_world_v3_opencl 0.1 1 > /dev/null)
	
You will probably find that the opencl version is slower, probably
by a large amount. This can be attributed to the balance between
two factors:

1. There is a lot of overhead querying device drivers and setting
    up OS level information.
2. The kernel must be compiled from scratch before it can
    execute anything.
3. The amount of compute for the first world is very small, as
    the world is tiny and only iterated one step.

However, what we want to deal with in practise are:

1. Large worlds. The greater the resolution of the world, the
	more precise the model, and the less the effect of
	spatial discretisaion.

2. Small timesteps. Smaller values of `dt` will give less
    discretisation error, resulting in a more accurate model.

3. Large periods. We want to see the steady state of system,
    which means running it until it stops changing, which
	could be many tens of thousands of iterations.
	
There is also a large hidden cost at the moment due to
the formatting of the world data, as it is being laboriously
converted to and from text when it is generated in `make_world`,
and then when it is input and output in `step_world`. The
reason for the text format is to make it easy to debug, but
there is also a binary format, supported by both programs.
To compare the two, try generating a larger world in text
then binary:

    time (bin/make_world 1000 0.1   0  > /dev/null)   # text format
	time (bin/make_world 1000 0.1   1  > /dev/null)   # binary format
	
Depending on your platform, you'll see between one and
two orders of magnitude in difference between the two. I
would reccommend using the binary format when not
debugging, as otherwise your improvements in speed will
be swamped by conversions.

Let's try to eliminate the overheads, by reading from a
binary file and trying to increase the world size a bit:

    bin/make_world 1000 0.1 1 > /tmp/world.bin  # Save binary world to temp file
	
Now look at the raw cost of execution with no steps at all:

	time (cat /tmp/world.bin | bin/step_world 0.1 0  1 > /dev/null) 
	time (cat /tmp/world.bin | bin/$USER/step_world_v3_opencl 0.1 0  1 > /dev/null)
	
The first command measures the cost of getting the data
in and out, which should be down to tens of milliseconds,
while the second command measures the overhead of
getting the GPU up and running. This will vary depending
on the system, but tells us something useful: if the current
execution time of your program is less than the GPU startup
cost, you will not be able to make it faster using a GPU.

We can check this by measuring the cost of one step:

	time (cat /tmp/world.bin | bin/step_world 0.1 1  1 > /dev/null) 
	time (cat /tmp/world.bin | bin/$USER/step_world_v3_opencl 0.1 1  1 > /dev/null)
	
The difference between this pair of times and the previous
pair of times tells us about the marginal cost of each frame,
after performing setup. Depending on your platform, the GPU
time per frame will be similar to or, more likely, quite a bit slower
than the original CPU. 

So what is wrong? We have a number of problems here:

1. Unecessary copying : we are copying the new state
	from GPU buffer back to CPU buffer, doing nothing
	with it, then copying the same data back again.

2. Too little work per thread : there is a fixed scheduling
	cost in order to spin a GPU thread up. If there is
	not much work to do, then the scheduling overhead
	outweighs the benefits.

3. No memory optimisations : the current approach places
	all data in global memory, which is the slowest memory
	type in the GPU. We should be taking advantage of
	private and shared memory as well.
	

Optimising memory accesses
==========================

The first thing to try is to remove the redundant memory copies: if you look at
`StepWorldV3OpenCL` the enqueue read at the bottom of the time loop
will usually end up getting copied straight back over to the GPU at the
top of the next iteration. We only look at the data right at the end, so
there is no point bringing it back to the CPU in-between. So the only
time the data needs to be transferred is:

1. Before the very first execution of the kernel

2. After the very last execution of the kernel.

Create a new file called `src/your_login/step_world_v4_double_buffered.cpp`,
based on `src/your_login/step_world_v3_opencl.cpp`, and name the new
function `hpce::your_login::StepWorldV4DoubleBuffered`.

We can minimise the transfers by moving the write of `buffState` off the top
of the loop, and the read of `buffBuffer` off the bottom. Note that
as you move the read over the call to `std::swap`, the target
of the read should change: instead of reading into `&buffer[0]`, it
should read directly into `&world.state[0]`. To keep things
simple, make both the copies synchronous, so pass CL_TRUE instead
of CL_FALSE, and remove any input and output events.

The only thing that will be left in the time loop is the enqueue of the
kernel, and the swap to flip the world state with the buffer. First
let's fix the buffer, by observing that swapping the host buffers
now has no effect, and we need to swap the GPU buffers being passed
to the kernel instead.

Every time we call the kernel we want to flip the order of
the buffers passed to the kernel, so we need to take the
two `setArg` calls for the buffers:

    kernel.setArg(3, buffState);
	kernel.setArg(4, buffBuffer);

and bring them inside the loop, just above the point where
the kernel is executed.

We also need to swap the two device pointers, so replace
the `std::swap` of the host buffers with a swap of the
device buffers:

	std::swap(buffState, buffBuffer);	// Used to be std::swap(world.state, buffer)

Now we need to fix the kernel, as currently it has dependencies on
events that no longer exist, so remove the input dependency list
and the output event. You'll now have a loop which just enqueues
kernels, without knowing when they have executed. The problem
is that there is no fixed ordering between the kernels: the run-time
could queue up 10 or 20 calls to enqueue kernel, then issue them
all in parallel. So we need one kernel to finish before the next one starts,
in order to make sure the output buffer is completely written
before the next kernel call uses it as an input buffer.

There are multiple solutions to this, but the easiest is to add a call to
`enqueueBarrier` just after the kernel is enqueue'd:

	queue.enqueueNDRangeKernel(kernel, offset, globalSize, localSize);
		
	queue.enqueueBarrier();	// <- new barrier here
	
	std::swap(buffState, buffBuffer);

This creates a synchronisation point within the command queue,
so anything before the barrier must complete before anything
after the barrier can start. However, it doesn't say anything
about the relationship with the C++ program. It is entirely
possible that the host program could queue up hundreds of
kernel calls and barriers before the OpenCL run-time gets round
to executing them. Indeed, it may only be the synchronised
memory copy _after_ the loop that forces the chain of
kernels and barriers to run at all.

So the inner loop should now look something like:

    kernel.setArg(3, buffState);
	kernel.setArg(4, buffBuffer);
	queue.enqueueNDRangeKernel(kernel, offset, globalSize, localSize);
		
	queue.enqueueBarrier();
	
	std::swap(buffState, buffBuffer);

The kernel code itself is identical to that for the previous
implementation, so you don't need to create a new .cl file. Sometimes
you can optimise execution purely by modifying the host code,
and other times tweaks to the kernel code are all that is needed.

A final problem is that our memory buffers were originally
declared with read-only and write-only flags, but now we
are violating that requirements. Go back to your buffer
declarations, and modify them to:

	cl::Buffer buffState(context, CL_MEM_READ_WRITE, cbBuffer);
	cl::Buffer buffBuffer(context, CL_MEM_READ_WRITE, cbBuffer);
	
There is a lot going on here, so to summarise what the program should
now look like, it should be something like:

``` text
Create buffState : read only
Create buffProperties, buffBuffer : read/write

Create kernel
Set inner, outer, and properties arguments

write properties data to buffProperties

write initial state to buffState

for t = 0 .. n-1) begin
   Set state argument to buffState
   Set output argument to buffBuffer
   
   enqueue kernel
   
   barrier
   
   swap(buffState, buffBuffer);
end

read buffState into output
```
	
You may find it confusing due to the abstraction of
the `cl::Buffer`, but it helps if you think of them
as a special kind of pointer. For example, imagine
`buffState` and `buffBuffer` have notional addresses
0x4000 and 0x8000 on the GPU. Then what happens
for n=2 is:

    buffState = (void*)0x4000;
	buffBuffer = (void*)0x8000;
	
	memcpy(originalState , buffState, ...);
	
	// loop iteration t=0
	// buffState==0x4000, buffBuffer==0x8000
	
	// Reading from 0x4000, writing to 0x8000
	kernel(..., input = buffState (0x4000), output = buffBuffer (0x8000))
	
	swap(buffState, buffBuffer); // swaps the values of the pointers
	
	// loop iteration t=1
	// buffState==0x8000, buffBuffer==0x4000
	
	// reading from 0x8000, writing to 0x4000
	kernel(..., input = buffState (0x8000), output = buffBuffer (0x4000))
	
	swap(buffState, buffBuffer);
	
	// After loop
	// buffState==0x4000, buffBuffer==0x8000
	
	memcpy(buffState, outputState, ...);
	
I highly reccommend you try this with n=1, n=2, and n=3,
as it is easy to think this is working for n=1 when it fails
for larger numbers.

Depending on your platform, you may now start to see a reasonable
speed-up over software (though probably still not over TBB).


Optimising memory accesses
==================

One of the biggest problems in GPU programming is managing the different memory
spaces. Slight differences in memory layout can cause large changes in
execution time, while moving arrays and variables from global to private
memory can have huge performance implications.
In general, the best memory accesses are those which never happen, so
it is worth-while trying to optimise them out. GPU compilers can be
more conservative then CPU compilers, so it is a good idea to help them
out.

Looking at our current kernel, we can see that there are five reads to
the `properties` array for a normal cell (non insulator). However, four
of those reads are getting very little information back, as we only depend
on a single bit of information from the four neighbours. A good approach
in such scenarios is to try to pack data into spare bit-fields, increasing
the information density and reducing the number of memory reads. In
this case, we already have one 32-bit integer describing the properties,
of which only 2 bits are currently being used. So as well as the 2 bits describing
the properties of the current cell, we could quite reasonably include four
bits describing whether the four neigbouring cells are insulators or not,
saving a memory access.

This requires two modifications: one in the host code to set up the more
complex flags, and another in the kernel code to take advantage of the
bits.

### Kernel code

At the top of the code, read the properties for the current cell into
a private uint variable. This value will be read once into fast private
memory, and then from then on it can be accessed very cheaply.
For each of the branches, rewrite it to check bit-flags in the
variable. For example, if your private properties variable is called `myProps`,
and you decided bit 3 of the properties indicated whether the cell
above is an insulator, the first branch could be re-written to:

	// Cell above
	if(myProps & 0x4) {
		contrib += outer;
		acc += outer * world_state[index-w];
	}

The other neighbours will need to depend on different bits in the properties.

### Host code

In the host code, you need to make sure the the correct flags have
been inserted into the properties buffer. This should only have local
effect, so we cannot modify `world.properties` directly. Instead
create a temporary array in host memory:
	
	std::vector<uint32_t> packed(w*h, 0);

and fill it with the appropriate bits. This will involve looping over all
the co-ordinates, using the following process at each (x,y) co-ordinate:

    packed(x,y) = world.properties(x,y)
    if cell(x,y) is normal:
        if packed(x,y-1):
            packed(x,y) = packed(x,y) + 4
        if packed(x,y+1):
            packed(x,y) = packed(x,y) + 8
        # Handle left and right cases

This process takes some time (though it could be parallellised), but we
don't care too much, as it only happens once for multiple time iterations.
Once the array is prepared, it can be transferred to the
GPU instead of the properties array.

This got my laptop up to about 2.5x speedup, so faster than the two
cores in my device can go. On an AWS GPU instance I looked at a 5000x5000
grid, stepped over 1000 time steps, which is more at the resolution
we typically use:

	time (bin/make_world 5000 0.1 1 | bin/step_world 0.1 1000 1 > /dev/null)
	time (bin/make_world 5000 0.1 1 | bin/dt10/step_world_v3_opencl 0.1 1000 1 > /dev/null)
	time (bin/make_world 5000 0.1 1 | bin/dt10/step_world_v4_double_buffered 0.1 1000 1 > /dev/null)
	time (bin/make_world 5000 0.1 1 | bin/dt10/step_world_v5_packed_properties 0.1 1000 1 > /dev/null)
	
For my code, I found:

Method     | time (secs)   | speedup (total) | speedup (incremental)
-----------|---------------|-----------------|---------------------
software   |         164.6 |           1.0   |                  1.0
opencl     |          66.2 |           2.5   |                  2.5
doublebuff |           9.0 |          18.3   |                  7.5
packing    |           6.5 |          25.3   |                  1.4
	

I strongly encourage you to also try the software OpenCL provider.
The AWS GPU instance has both a GPU and software provider installed
(though irritatingly they both just show up as "NVIDIA Corporation"),
and you can use `HPCE_SELECT_PLATFORM` to choose which one you
want. The GPU instance only has 8 cores, and the code is not
optimised for CPU-based OpenCL providers, but it should still be
less than half the time of the original software.


Further optimisations
=====================

In this context of this particular coursework, we won't go
any further, but the next issue to tackle would be the
small amount of work being done by each thread. At each
co-ordinate there are only a handful of operations performed,
so it would be good to batch up or agglomerate some
of those operations, to reduce scheduling overhead.
This agglomeration could be temporal, but partially
unrolling multiple iterations, or spatial, but grouping
together consecutive iterations into one larger iteration.

We are also not using shared memory at all, which is wasting
a precious resource. For this problem we want
to have all processors working on the same problem, which
means they need to communicate via global memory and perform
global barriers (which don't formally exist).  However,
there are certain locking mechanisms that can be used to
co-ordinate this, and a hierarchical approach could be used:

- Registers: each thread manages a cluster of pixels (e.g. 16),
  held locally in registers.
- Shared: after each time-step each thread exhanges its
  halo of 16 pixels with its neighbours via shared memory.
- Global: after each time-step a sub-set of threads exchange
  a coarser halo via global memory. Assume each thread manages
  16 pixels, and each workgroup has 256 threads, there would
  be a total pixel cout of 4096, but the halo drops down to
  256.

Such an approach would rely on hardware level knowledge
to avoid deadlock, but is the kind of thing that might
be done in practise.

Another optimisation is to cache the compiled binary
for the kernel code. Depending on the device, the run-time,
and other system characteristics, compiling the binary
may be very fast, or quite slow. For example, on my
desktop it takes well under a second, but on my laptop
it takes around 10 seconds, both using the AMD OpenCL
implementation. For large numbers of time-steps this
cost will shrink, but for short-lived programs it is
a potentially significant cost. The NVidia provider
actually does this by default, so you may see less of
this effect on such platforms.

Deliverables
============

Make sure all your source files are checked into your
local git repository:

- `src/your_login/step_world_v1_lambda.cpp`

- `src/your_login/step_world_v2_function.cpp`

- `src/your_login/step_world_v3_opencl.cpp`

- `src/your_login/step_world_v3_kernel.cl`

- `src/your_login/step_world_v4_double_buffered.cpp`

- (You may have a seperate kernel `src/your_login/step_world_v4_double_buffered.cl`, or
	you may be using `src/your_login/step_world_v3_kernel.cl` for both).

- `src/your_login/step_world_v5_packed_properties.cpp`

- `src/your_login/step_world_v5_kernel.cl`

Each implementation should perform the same step as
the original step_world. While there may be minor
numerical differences due to the single-precision arithmetic,
there should be no major differences in the output over
time.

Also include any useful documents or supporting
material, your repository does not have to be
clean. However, your repository should not contain
compiled artefacts such as executables and object
files, or other large binary objects.

Push the results up to your private github repository,
then submit a zipped copy (together with .git) to
blackboard.

Compatibility patches
=====================

### Missing alloca.h

Windows doesn't have a version of <alloca.h>, so if necessary
just comment it out.

Debugging tips
==============

### Debug output

In this particular exercise, you shouldn't encounter
too many probles, but one useful feature of modern
GPUs is that they allow debugging output, even from
within kernels running in the GPU. In OpenCL 1.2 the
printf support is enabled by default, but for other
platforms you may need to use a #pragma to enable
it. For example, in AMD platforms you can add the
following to your kernel code:

    #pragma OPENCL EXTENSION cl_amd_printf : enable

and after that it is possible to use printf. Note
that the printf output from different GPU threads
may become interleaved - they are executing truly
in parallel, after all.

In NVidia platforms you may be less lucky, as they try to keep
some functionality CUDA-only for business reasons. However,
you can always try using a software OpenCL implementation
for testing purposes.

### Software implementations

Another useful trick when debugging is to fall back
on a software OpenCL implementation if possible. This
can sometimes make it more obvious where a particular
memory address is causing problems, as the exception
occurs on the processor, rather than in the GPU.

### Dynamically changing kernels

When OpenCL kernels are held as seperate files, it
makes it very easy to play around with them without
recompiling. For example, when trying to debug a particular
crash or bug in a new or optimised kernel, you can
easily comment out particular lines and re-run the
program. Each kernel gets compiled anyway, so there
is very little overhead to experimenting.

### Locked up and crashing machines

Some systems are more or less stable than others when
it comes to OpenCL. In principle it should be impossible
to cause any problems using OpenCL code, and each process
should still be isolated from each other. However, in
practise the drivers can be a bit buggy, and it is not
unheard of to see a blue-screen or kernel panic, The
AMD drivers and hardware are particularly problematic
in this regard, at least historically.

However, any such problems are usually due to incorrect
kernel code, which is reading or writing where it shouldn't.
A good starting point is to comment out suspicious memory
writes, or to try working within a software OpenCL implementation
as suggested above.

Even in a correct program, you may find that the OpenCL
code can cause the machine to stutter or lag, in a way
that is quite different to standard CPU load. This is
usually caused due to time-slicing of the GPU with the
graphics display. Older cards cannot run multiple kernels
at the same time, so if the system is running your code,
it cannot update the windows and display. It is generally
a good idea to limit the execution time of any one kernel,
so if possible tune things so that any given thread does not
take more a few hundred milli-seconds. If you are on a server
this is less of an issue, but even there you may find that
the OS aborts kernel calls which take multiple seconds.


