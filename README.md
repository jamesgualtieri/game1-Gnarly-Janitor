# Game Information
(Note: fill in this portion with information about your game.)

Title: The Gnarly Janitor

Author: James Gualtieri

Design Document: [link](http://graphics.cs.cmu.edu/courses/15-466-f18/game1-designs/jgualtie/)

Screen Shot:

![Screen Shot](screenshot.png)

How To Play:

Using the WASD keys, move the avatar around the super crazy level to clean up spills. Clean up 20 spills before the time runs out to win, otherwise, you lose. If the mop gets too dirty you'll have to go grab another one from the supply closet.

Changes From The Design Document:

Ran out of time to add the reinitilization of the game and resetting to play again. I added some more sound effects than I planned originally because iether Jim or SOs sugested I should.

Good / Bad / Ugly Code:

The entire game is some pretty ugly code to be completely honest. For example, my player collision with spills functiono is just a distance < epsilon funciton. Thankfully, with the help of nice game developers' postings around the internet I was able to get through some of the more annoying bits of math for this one. Certainly some of my ugliest code I've written in a bit.

## Runtime Build Instructions

The runtime code has been set up to be built with [FT Jam](https://www.freetype.org/jam/).

### Getting Jam

For more information on Jam, see the [Jam Documentation](https://www.perforce.com/documentation/jam-documentation) page at Perforce, which includes both reference documentation and a getting started guide.

On unixish OSs, Jam is available from your package manager:
```
	brew install ftjam #on OSX
	apt get ftjam #on Debian-ish Linux
```

On Windows, you can get a binary [from sourceforge](https://sourceforge.net/projects/freetype/files/ftjam/2.5.2/ftjam-2.5.2-win32.zip/download),
and put it somewhere in your `%PATH%`.
(Possibly: also set the `JAM_TOOLSET` variable to `VISUALC`.)

### Libraries

This code uses the [libSDL](https://www.libsdl.org/) library to create an OpenGL context, and the [glm](https://glm.g-truc.net) library for OpenGL-friendly matrix/vector types.
On MacOS and Linux, the code should work out-of-the-box if if you have these installed through your package manager.

If you are compiling on Windows or don't want to install these libraries globally there are pre-built library packages available in the
[kit-libs-linux](https://github.com/ixchow/kit-libs-linux),
[kit-libs-osx](https://github.com/ixchow/kit-libs-osx),
and [kit-libs-win](https://github.com/ixchow/kit-libs-win) repositories.
Simply clone into a subfolder and the build should work.

### Building

Open a terminal (or ```x64 Native Tools Command Prompt for VS 2017``` on Windows), change to the directory containing this code, and type:

```
jam
```

That's it. You can use ```jam -jN``` to run ```N``` parallel jobs if you'd like; ```jam -q``` to instruct jam to quit after the first error; ```jam -dx``` to show commands being executed; or ```jam main.o``` to build a specific file (in this case, main.cpp).  ```jam -h``` will print help on additional options.
