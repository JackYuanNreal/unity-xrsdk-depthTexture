# Test demo for unity xrsdk depthTexture
    Description: this demo project base on the public unity-xr-sdk project(https://create.unity.com/vsp-signup-form)

# Plugin:
    Path: Samples/Plugins/Display
    How to compile: As working on macOS, so I use cmake to compile it. 
        1. open the file "Samples/Plugins/Display/run.sh", change $NDK_HOME to local NDK path.
        2. open terminal, change path to Samples/Plugins/Display
        3. run cmd "sh ./run.sh"
    Description:
        4. ExampleDisplayProvider class: implement a displayProvider;
        5. TextureManager class：create and manager the color textures and depth textures;
        6. TextureManager.DebugDraw: debug code, you can modify the code here to verify;
           * DrawColoredTriangle and DrawDepthTriangle each draw a rectangles;
           * search "DEBUG_DEPTHTEXTURE" in source code;

# Unity project:
    Path: Samples/TestProjects/Display
    Scene: TestDepthTexture
    Description:
        1. XREntry：start XRDisplaySubsystem;
        2. UseRenderingPlugin：interact with DisplayProviderSample;
           * SetTimeFromUnity: give a static texture to native plugin;
           * IssuePluginEvent: issue native draw;
