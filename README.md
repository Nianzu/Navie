
# Navie

A little robot that maps its space autonomously, navigating entirely on its own. Hence the name, Navie.

## Implemented Features

## Desired Features
* Extremely low upfront cost (<$100)
* Stereo camera for navigation
* Use brushless motors as drive system
* Onboard navigation processing
* Onboard vision processing
* Really small (Fit in the palm of the hand)
* Integrated rechargeable battery
* FOC on brushless motors
* Custom motor driver circuit
* WIFI connectivity

![](ProcessDiagram.svg)

## Technologies
Currently being researched. With the intent to run both the stereo image depth processing AND the navigation code onboard, I'm not sure what tech stack will be required. After the stereo image processing code has been written in C, I plan to benchmark it on Raspberry Pi 3, 4, and Picos to determine the feasibility.
https://vision.middlebury.edu/stereo/data/

## Getting Started

### Dev environment setup
* Ubuntu running under WSL with VcXsrv.
* Installing OpenCV is a pain if there isn't already a binary for your system, but this script makes things pretty seamless. I did have to edit the dependencies to get it to work properly, but this is a good place to start.
https://github.com/jayrambhia/Install-OpenCV

### Compile
```g++ main.cpp -o main.o `pkg-config --cflags --libs opencv````

## To-Do
* Consider installing a laser pointer to aid in depth perception of featureless walls.


## Contributions

Contributions are always welcome. If you want to contribute to the project, please create a pull request.

## License

This project is not currently licensed, but I will look into adding a license at a later date.