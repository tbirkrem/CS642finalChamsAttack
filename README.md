# CS642finalChamsAttack

This code is from https://gamehacking.academy/lesson/5/4 

A Chams attack is a popular exploit used in FPS games that modifies a user’s on-screen environment. Its design is to edit the graphics library loaded into the game and modify the appearance of opposing players to appear as a bright color rather than as a typical build of a person. For this exploit we used the Urban Terror game. 

First, let’s take a look at what the graphics library for Urban Terror looks like. Every object we see on-screen is a combination of two elements: a texture and a polygon. The polygon is a 3D mold of the object, for example an oil barrel will have a polygon of a cylinder. A polygon by itself contains no color, design, or visual independencies. On the other hand, textures are 2D images that get wrapped around the 3D polygon to give the object personal features and design elements. The two elements of an object are loaded to the game upon the load of OpenGL, the library storing all graphics for the game.

Now let’s take a look at the exploit itself. Attackers take advantage of the fact that OpenGL does not get loaded upon game load, but after the fact. This allows us to find the exact memory location of the library and modify its components.

DLL Injection: The DLL Injection takes hold when OpenGL is loaded, and we assign pointers at specific DrawElements functions based on the offset of OpenGL’s location. These pointers are going to be used to enable and disable certain functions, allowing us to modify specific graphics as we choose. Namely, we need to disable the textures, color, and lighting elements of specific objects. After the pointers are set, we’ll find the hook location, again using the offset of OpenGL’s location, to assign a JMP command to JMP to our very own codecave. It is within this codecave that we can begin to personalize the graphics.

Codecave: The codecave is going to run before glDrawElements is called, allowing us to set some parameters before the textures are assigned to the polygons. Since we want to target other players, and the polygons of people are very complex shapes, we are going to target all polygons with a total number of vertices over 500. For each polygon, if more than 500 vertices exist, then we are going to disable the depth clipping plane, the textures function, and the color function. Lastly, we are going to enable the color materials function and assign the color that we want the players to be, in this case all players will appear as red.
For objects that are below 500 vertices, we will restore the depth clipping plane and keep the default graphics set, so that when glDrawElements is called it will load the graphics as originally intended. 
