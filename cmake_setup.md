CMake
================================

This readme will guide you how to setup the project using CMake. CMake generates native makefiles and IDE workspaces that can be used in an environment of your choice.

Building the Project
--

### CMake GUI (Windows and Mac)
1. [Download](http://www.cmake.org/download/), install and launch CMake
2. For `Where is the source code:`, specify the location for `../path/to/hpce-2014-cw4`
3. For `Where to build the binaries:`, specify a location where the project files will be created in i.e. `../path/to/hpce-2014-cw4/bin`
4. Click Configure (if the `bin` folder does not exist, CMake may ask to create one)
5. CMake will now ask to `Specify the generator for this project`. Mac users here should select `Xcode`, Windows users should select (the installed version of) `Visual Studio`
6. Click Done
7. Click Configure and then Generate
8. If all correct, there should be no red.

Inside the `bin` folder there will now be a Xcode Project file `HPCE-2014-CW4.xcodeproj` (if on mac) or a Visual Studio Solution `HPCE-2014-CW4.sln` (if on Windows). Double click this to launch the IDE. 

The IDE will have all the executibles/binaries listed as independent projects.
If `ALL_BUILD` is selected, pressing build on the IDE will build all executibles/binaries.
If a particular executible/binary is selected, pressing build will build only that executible/binary.
The built executables/binaries should be in `bin/Debug`.

### CMake Command Line (Linux/MinGW/Cygwin/Mac Terminal)
`cmake` is required via the command line, linux users can `sudo apt-get install cmake`. 
Not 100% sure if already available on MinGW and Cygwin. Mac users can install cmake into the command line through the CMake app. 

First, `cd` into the `hpce-2014-cw4` folder, then run:

1. `mkdir bin`
2. `cd bin`
3. `cmake ..`
4. `make`

The binaries will be built in the `bin` folder. Note: for MinGW/Cygwin users, if Visual Studio is installed, CMake may default into generating Visual Studio project file.

Adding additional coursework files 
--
Throughout the coursework, you will be adding files into `src/your_login`. You need to rerun CMake so that they are added to the project.

1. Set your `your_login` in `CMakeLists.txt` (around line 80)
2. Add your .cpp files into `src/your_login`
3. rerun CMake (above) to regenerate makefiles or update changes in IDEs

CMakeLists.txt will look in `src/your_login` for `.cpp` files, where it will link the `.cpp` file with appropriate libraries and create an binary/executable. If working with an IDE, this will show up as a new project. 

