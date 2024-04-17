# Test demo for unity xrsdk depthTexture
    Description: this demo project base on the public unity-xr-sdk project(https://create.unity.com/vsp-signup-form)

# Plugin:
    Path: Samples/Plugins/Display
    How to compile: As working on macOS, so I use cmake to compile it. 
        1. open the file "Samples/Plugins/Display/run.sh", change $NDK_HOME to local NDK path.
        2. open terminal, change path to Samples/Plugins/Display
        3. run cmd "sh ./run.sh"
    Description:
        1. ExampleDisplayProvider class: 通过实现displayProvider实现上屏;
        2. TextureManager class：color texture和depth texture的创建与管理;
        3. TextureManager.DebugDraw: 主要调试接口，通过修改其中代码进行验证;
           * 通过DrawColoredTriangle和DrawDepthTriangle绘制两个面片到当前屏幕上;
           * 搜索DEBUG_DEPTHTEXTURE，查看具体的调试说明;

# Unity project:
    Path: Samples/TestProjects/Display
    Scene: TestDepthTexture
    Description:
        1. XREntry脚本：启动XRDisplaySubsystem;
        2. UseRenderingPlugin脚本：与DisplayProviderSample库交互;
           * 通过SetTimeFromUnity传入静态texture;
           * 通过IssuePluginEvent触发native draw;
