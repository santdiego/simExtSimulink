# CoppeliaSim-Simulink-Communication
Plugin and blocks to communicate the CoppeliaSim robotic simulator with the Matlab Simulink environment.

## Instructions for compiling plugin in Windows:


1. Download the source code.
2. Copy the unzipped folder into the "programming" directory within the CoppeliaSim installation path
`(C:/Program Files/CoppeliaRobotics/CoppeliaSimEdu/programming)`.
3. Open the .pro row and configure the project using the same version of the CoppeliaSim QtKit.
4. Compile in Release mode

## Instructions for compiling Simulink s-functions:


1. Open Matlab and place into unziped folder. 
2. run the following commands:
```matlab
mex gettable.cpp
mex settable.cpp
```

## Instructions for running the examples:


1. Download the examples and unzip them.
2. Copy `simExtSimulink.dll` into CoppeliaSim folder : `C:\Program Files\CoppeliaRobotics\CoppeliaSimEdu`
3. Open matlab. Make sure the matlab working path is inside the unzipped folder. Or add the unzipped folder to the matlab path.
4. Open the file `example_xx.slx` and run the simulation.
5. Coppelia sim should start automatically.


>The "`CoppeliaSim Open / Play / Stop`" block is responsible for opening and running the simulation in CoppeliaSim. In case of error, delete this block, open the example "example_xx.ttt" and run the simulation manually.



*Attention. To use on Linux you must compile all the source codes, including the Simulink s-functions. It is not yet tested on Linux.*


