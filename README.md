# Siyana Renderer
An OpenCL-based path tracer and terrain generator I wrote while in high school.

Here are a few videos of it working:
* https://www.youtube.com/watch?v=S3y8lY9vSMU
* https://www.youtube.com/watch?v=pDrg3tHDQHA

## Controls
* Mouse to look around
* W: Forward
* A: Left
* S: Backward
* D: Right
* Q: Down
* E: Up

## Setup
###### Boost
You can install the Boost C++ libraries from source [here](http://www.boost.org/) or more simply with homebrew.

```
brew install boost
```

###### GLFW3
To install GLFW3, you can either install it via homebrew,

```
brew install gflw3
```

or follow the steps below to compile it from source. For more information, visit their [website](http://www.glfw.org/docs/latest/compile.html).

Install cmake if you don't already have it, e.x.

```
brew install cmake
```

Then,

```
mkdir glfw-build
cd glfw-build
cmake <glfw-sources-directory>
make
make install
```

If you are using the Xcode project the rest of the dependencies will already be setup.

## Author
Julian Villella

## License
Siyana Renderer is available under the MIT license. See the LICENSE file for more info.