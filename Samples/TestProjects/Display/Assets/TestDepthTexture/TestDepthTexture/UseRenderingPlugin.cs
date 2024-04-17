using UnityEngine;
using System;
using System.Collections;
using System.Runtime.InteropServices;
using UnityEngine.Rendering;


public class UseRenderingPlugin : MonoBehaviour
{
    public Texture2D testTexture;
    
    // Native plugin rendering events are only called if a plugin is used
    // by some script. This means we have to DllImport at least
    // one function in some active script.
    // For this example, we'll call into plugin's SetTimeFromUnity
    // function and pass the current time so the plugin can animate.
    [DllImport("DisplayProviderSample")]
    private static extern void SetTimeFromUnity(float t);


    // We'll also pass native pointer to a texture in Unity.
    // The plugin will fill texture data from native code.
    [DllImport("DisplayProviderSample")]
    private static extern void SetTextureFromUnity(System.IntPtr texture, int w, int h);

    [DllImport("DisplayProviderSample")]
    private static extern IntPtr GetRenderEventFunc();


    IEnumerator Start()
    {
        Screen.sleepTimeout = SleepTimeout.NeverSleep;
        
        CreateTextureAndPassToPlugin();
        
        Camera cam = Camera.main;
        if (cam)
            cam.depthTextureMode = cam.depthTextureMode | DepthTextureMode.Depth;
        
        yield return StartCoroutine("CallPluginAtEndOfFrames");
    }

    void OnDisable()
    {
    }
    
    private void OnRenderObject()
    {
        Debug.LogFormat("TestDepth OnRenderObject: {0}", gameObject.name);
#if !UNITY_EDITOR
        GL.IssuePluginEvent(GetRenderEventFunc(), 1);
#endif
    }

    private void CreateTextureAndPassToPlugin()
    {
        // Pass texture pointer to the plugin
        SetTextureFromUnity(testTexture.GetNativeTexturePtr(), testTexture.width, testTexture.height);
    }

    // custom "time" for deterministic results
    int updateTimeCounter = 0;

    private IEnumerator CallPluginAtEndOfFrames()
    {
        while (true)
        {
            // Wait until all frame rendering is done
            yield return new WaitForEndOfFrame();

            // Set time for the plugin
            // Unless it is D3D12 whose renderer is allowed to overlap with the next frame update and would cause instabilities due to no synchronization.
            // On Switch, with multithreaded mode, setting the time for frame 1 may overlap with the render thread calling the plugin event for frame 0 resulting in the plugin writing data for frame 1 at frame 0.
            if (SystemInfo.graphicsDeviceType != GraphicsDeviceType.Direct3D12 && SystemInfo.graphicsDeviceType != GraphicsDeviceType.Switch)
            {
                ++updateTimeCounter;
                SetTimeFromUnity((float)updateTimeCounter * 0.016f);
            }

            // Issue a plugin event with arbitrary integer identifier.
            // The plugin can distinguish between different
            // things it needs to do based on this ID.
            // On some backends the choice of eventID matters e.g on DX12 where
            // eventID == 1 means the plugin callback will be called from the render thread
            // and eventID == 2 means the callback is called from the submission thread
            // GL.IssuePluginEvent(GetRenderEventFunc(), 1);
        }
    }
}
