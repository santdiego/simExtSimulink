# CoppeliaSim-Simulink-Communication
Plugin and blocks to communicate the CoppeliaSim robotic simulator with the Matlab Simulink environment.

Instructions for compiling plugin in Windows:


-Download the source code.


-Copy the unzipped folder into the "programming" directory within the CoppeliaSim installation path ("C: \ Program Files \ CoppeliaRobotics \ CoppeliaSimEdu \ programming").


-Open the .pro row and configure the project using the same version of the CoppeliaSim QtKit.


-Compile in Release mode.


Instructions for compiling Simulink s-functions:


-Open Matlab and place into unziped folder. 


-run the following commands:


mex gettable.cpp


mex settable.cpp

Instructions for running the examples:


-Download the examples and unzip them.


-Open matlab. Make sure the matlab working path is inside the unzipped folder. Or add the unzipped folder to the matlab path.


-Open the file example_xxx.slx and run the simulation.


Coppelia sim should start automatically.


The "Coppelia Sim Open / Play / Stop" block is responsible for opening and running the simulation in CoppeliaSim. In case of error, delete this block, open the example "examplexx.ttt" and run the simulation manually.



Attention. To use on Linux you must compile all the source codes, including the Simulink s-functions. It is not yet tested on Linux.


