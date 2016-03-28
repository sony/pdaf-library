# PDAF Library

## What is a PDAF Library
PDAF Library converts phase difference data from image sensor into defocus.  
PDAF gets defocus from PDAF Library and notify defocus to HybridAF.  
HybridAF controls PDAF and ContrastAF to get fine focus in short time.  
Software components of PDAF, ContrastAF and HybridAF are assumed to be  
developed by user.  For detail, please refer to  
[specification document](docs/PDAF_Library_API_Specification.pdf).  

### Repository structure
Folder and file structure of this repository is the following list.  

    root/  
        src/                           // Folder contains source code  
             PdafLibrary.c             // Source code of PDAF Library  
             PdafLibrary.h             // Header file of PDAF Library  
             PdafMathFunc.c            // Source code of math function  
             PdafMathFunc.h            // Header file of math function  
        docs/                          // Folder contains document  
             PDAF_Library_API_Specification.pdf // Specification document  
        LICENSE                        // License file  
        README.md                      // This file  
        RELEASENOTE.md                 // Release note  
        
### How to build PDAF Library

Please build PDAF Library on your environment in which you want.  

The following example is simple "Application.mk" and "Android.mk"  
to build shared library on Android NDK.   
Please put PDAF Library source code and "Application.mk" and "Android.mk"   
on same directory named "jni".   
And command "ndk-build".  

```Makefile
#Application.mk 
APP_ABI        := arm64-v8a 
APP_OPTIM      := release 
APP_PLATFROM   := android-23   # Android 6.0  
```
```Makefile
#Android.mk  
include $(CLEAR_VARS)  
LOCAL_PATH        := .  
LOCAL_MODULE      := PdafLibrary  
LOCAL_SRC_FILES   := PdafLibrary.c PdafMathfunc.c  
include $(BUILD_SHARED_LIBRARY)  
```

### How to use PDAF Library
Please see the following documentation.  

- [PDAF_Library_API_Specification.pdf](docs/PDAF_Library_API_Specification.pdf)

### Support platforms
- android
- windows 10 mobile

## Release Notes
Please see the following link.  

- [Release Notes](RELEASENOTE.md)

## License

The BSD-3-Clause license.  

See [LICENSE](LICENSE) for details.  