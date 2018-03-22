h264-analysis-tool
==========

Introduction:
==========
This project implements an extension to the H.264 JM reference decoder 
(http://iphome.hhi.de/suehring/tml) that allows to visually analyse a video.
For a quick demo visit http://www.youtube.com/watch?v=j1_YCLDdKuY

This project was cloned from https://github.com/febiz/h264-analysis-tool

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
(09.11.2016):

Project includes CMake (http://www.cmake.org) file to build on multiple 
platforms. Currently tested only on OS X 10.11
Required libraries:
- OpenCV (http://opencv.org). Please download the OpenCV library and install
  anywhere on your computer. Currently tested with version 2.4.4.
- Zlib (http://www.zlib.net). Please donwload the zlib library and install
  anywhere on your computer. NOTE(matt): seems to be already installed on OS X
  
Create a build directory in the root folder. Open a terminal and go to the build
folder. Execute the command:
> cmake ..

- In case CMake does not find OpenCV add the argument
> -DOpenCV_DIR:PATH=path_to_OpenCV_build

  Adapt path_to_OpenCV_build such that it points to the build directory of your
  OpenCV installation.
  
Now you can compile the generated make/project files.

Executing the project:
===========
Assuming your executable is in root/bin folder. Run the following command:
> ./H264H264AnalysisTool -d test_decoder.cfg

You should see the first frame of the test sequence with all the features
enabled.

Key Commands:
===========

q - quit
i - toggle frame info (index and type)
f - toggle motion vectors
m - toggle macroblock mode
g - toggle macroblock grid
p - toggle macroblock splits
left-arrow, '<', or ',' - go back to previous frame
right-arrow, '>', or '.' - advance to next frame


Creating H264 Byte Streams:
===========
You can use gstreamer to demux .mp4 files and parse into .h264 streams
> gst-launch-1.0 filesrc location=kitten.mp4 ! qtdemux ! h264parse ! video/x-h264,stream-format=byte-stream ! filesink location=kitten.h264
