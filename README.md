### Short Description
pviewer is a simple image viewer application that enables the caching of images to increase the responsiveness of the user interface. In the command line, there is logging information about which image is currently displayed and, if any, which are cached. The default size of the cache is 10 images. Caching begins when half the cache size minus one `cache_size / 2 - 1` image remains until the cache limit is reached.

For example, assume the following state of the application: At start-up, it fills the cache by loading the first 10 images and starts by pointing at the `0` image. We went through the image list, stopping at the `5` image as follows:

```plaintext
C - is a 'cached' image,
N - is an 'non-cached' image.

0    1    2    3    4    5    6    7    8    9   10   11   12
C    C    C    C    C    C    C    C    C    C    N    N    N
                         Ʌ
                         |
                     displayed
```

After we moved to display image `6`, we were on the threshold of the cache. Consequently, image `10` was added to it, and image `0` was removed, which gives us:
```plaintext
0    1    2    3    4    5    6    7    8    9   10   11   12
N    C    C    C    C    C    C    C    C    C    C    N    N
                              Ʌ
                              |
                          displayed
```
The given caching scheme provides us with sufficient space to move back and forth through cached images if required.

### How to build?
```shell
cmake -B build -S .
cmake --build build
```

### Usage
```plaintext
pviewer [folder_with_images] [image_number_to_start_with]
pviewer [images]...
```
- The first option will load all images in the `folder_with_images` directory. Additionally, with `image_number_to_start_with`, you can specify which image you want to display (starting from 1).
- The second option involves loading individual images, numbered with `images...`.

### Hotkeys
- **q**: enable/disable autoscrolling
- **]**: go to next folder (if subdirectories were loaded)
- **[**: go to previous folder (if subdirectories were loaded)
- **s**: scale image to screen width and back to the standard image width
- **Right_Arrow + Ctrl**: display the next image
- **Left_Arrow + Ctrl**: display the previous image
- **o**: open dialog window, which allows choosing content to display
- **Home_Key**: jump to the beginning of the image list
- **End_Key**: jump to the end of the image list
- **F11**: go fullscreen mode
- **Right Mouse Button**: display the next image
- **Left Mouse Button**: display the previous image

### Used instruments
- Qt 5.15.3
- GoogleTest 1.11.0
- CMake 3.18.0
- clang++-15

 Supported formats: `.jpg`, `.jpeg`, and `.png`.