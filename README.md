# OpenGL Motorbike

## What is this?
Welcome to the OpenGL Motorbike repository!

This is an implementation of a basic motorbike driving simulator where you can relax while you control a fast motorbike through the streets of a beautiful country road, heading towards a city. This was implementas as the final project for the Introduction to Graphical Systems class offered at the Polytechnic University of Valencia.

I also tried to have fun with the signs and based most of them on the movie They Live (1988). Hope you enjoy.

## What can be done in-game?
Currently the game supports the following functionalities, which can be controled by the user:

 - Arrows: control vehicle movement.
 - 'S' or 's': toggle between solid and wire drawing modes.
 - 'P' or 'p': cycle between player view, third person view and birds-eye view.
 - 'Y' or 'y': randomly change the wind.
 - 'L' or 'l': toggle between night and day.
 - 'D' or 'd': toggle between active and inactive collision for the road.
 - 'W' or 'w': toggle between clear and rainy weather.
 - 'N' or 'n': toggle between fog and no fog.
 - 'C' or 'c': show/hide HUD.
 - 'E' or 'e': show/hide axis vectors. (Only to be used as a reference for implementation purposes)

## Are there any screenshots?
Yes. Here are three screenshots showing most of the functionalities of the sim:

![First person view of the game, shows the HUD, day mode, rainy, and the motorbike approaching a sign](https://github.com/asarvazyan/sgi-project/blob/main/imgs/third-person-view.png?raw=true)

![Third person view of the game, hidden HUD, night mode, foggy, and the motorbike nearing a tunnel](https://github.com/asarvazyan/sgi-project/blob/main/imgs/first-person-view.png?raw=true)

![Birds-eye-view, shows the HUD, day mode, sunny, shows axes](https://github.com/asarvazyan/sgi-project/blob/main/imgs/birds_eye_view.png?raw=true)


## How to run?

All you need to do is make sure the following libraries are installed correctly: OpenGL, GLU, GLUT, FreeImage.

To compile:

```$ g++ p9.cpp -o p9.out -lGL -lGLU -lglut -lfreeimage```

To un:

```$ ./p9.o```


### Why is the file called p9.c?
Simple, this was the 9th practicum session, which was the one in which we had to do this project. Thus, p9.
