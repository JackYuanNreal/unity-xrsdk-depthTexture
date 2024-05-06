1. What happened
 * It's all about depth textures in unity XR SDK.
 * I implement display provider plug-in and run successfully to sample color texture on callback of SubmitCurrentFrame.  
 * After that, I wish to obtain depth textures,  and I do it following the Unity Documention(https://docs.unity3d.com/Manual/xrsdk-display.html).  But the depth texture  doesn't work as the color texture.  While I use the depth texture, there is no content in the depth texture which is all black.
  
 I want to make sure if it's a valid and correct way to get depth information. If not, how can I obtain it?
 
2. How can we reproduce it using the example you attached
 a.  open the project in "./Samples/TestProjects/Display" by unity editor;
 b.  open the scene "TestDepthTexture.unity";
 c.  build the apk, and run it on android phone. You will see a 3D scene, and a black rectangle on left-down corner, which is draw by sampling the depth texture;

3. More information about the demo project
# Test demo for unity xrsdk depthTexture
    Description: this demo project base on the public unity-xr-sdk project(https://create.unity.com/vsp-signup-form)

# Plugin:
    Path: Samples/Plugins/Display
    How to compile: As working on macOS, so I use cmake to compile it. 
        1. open the file "Samples/Plugins/Display/run.sh", change $NDK_HOME to local NDK path.
        2. open terminal, change path to Samples/Plugins/Display
        3. run cmd "sh ./run.sh"(You may need to install cmake on your local computer)
    Description:
        4. ExampleDisplayProvider class: implement a displayProvider;
        5. TextureManager class：create and manager the color textures and depth textures;
        6. TextureManager.DebugDraw: debug code, you can modify the code here to verify;
           * DrawColoredTriangle and DrawDepthTriangle each draw a rectangles;
           * search "DEBUG_DEPTHTEXTURE" in source code, to modify the code for debuging.

# Unity project:
    Path: Samples/TestProjects/Display
    Scene: TestDepthTexture
    Description:
        1. XREntry：start XRDisplaySubsystem;
        2. UseRenderingPlugin：interact with DisplayProviderSample;
           * SetTimeFromUnity: give a static texture to native plugin;
           * IssuePluginEvent: issue native draw;