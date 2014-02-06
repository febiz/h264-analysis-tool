h264-analysis-tool
==========

Introduction:
==========
This project implements an extension to the H.264 JM reference decoder 
( http://iphome.hhi.de/suehring/tml ) that allows to visually analyse a video.
For a quick demo visit http://www.youtube.com/watch?v=j1_YCLDdKuY

Features:
==========
(06.02.2014):
- Display macroblock (MB) partitions (16x8, 8x16, 8x8) and sub-partitions
  (8x4, 4x8, 4x4) for intra (I), predicted (P) and bi-predicted (B) MBs.
- Display MB mode (intra: red, inter: blue, SKIP/DIRECT: green)
- Display motion vectors (MV) for each partition/sub-partition and for each
  list. LIST_0 MVs (P blocks) are white while LIST_1 MVs (B blocks) are black.
- Display textual information about MB (left mouse click). Textural information
  includes position, MB mode and detailed MV information.
  
Building project:
===========
(06.02.2014):
Project includes CMake ( http://www.cmake.org )file to build on multiple 
platforms. Currently tested only on Windows 7 x64. 
Required libraries:
- JM reference software ( http://iphome.hhi.de/suehring/tml ): please download
  the reference software and install anywhere on your computer. Currently
  tested with release 18.2 and 18.6.
- OpenCV ( http://opencv.org ): please download the OpenCV library and install
  anywhere on your computer. Currently tested with version 2.4.4.
- Zlib ( http://www.zlib.net ): please donwload the zlib library and install
  anywhere on your computer.
  
CMake:
Create a build directory in the root folder. Open a terminal and go to the build
folder. Execute the command:
> cmake .. -DJM_DIR:PATH=path_to_JM

- Adapt path_to_JM such that it points to the root folder of the JM reference
  software
- In case CMake does not find OpenCV add the argument
> -DOpenCV_DIR:PATH=path_to_OpenCV_build
  Adapt path_to_OpenCV_build such that it points to the build directory of your
  OpenCV installation
  
Now you can compile the generated make/project files.

Executing the project:
===========
Assuming your executable is in root/bin folder. Run the following command:
> ./H264H264AnalysisTool[.exe] -d test_decoder.cfg

You should see the first frame of the test sequence with all the features
enabled.
