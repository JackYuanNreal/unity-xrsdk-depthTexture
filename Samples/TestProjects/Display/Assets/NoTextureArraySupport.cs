using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.XR;

public class NoTextureArraySupport : MonoBehaviour
{
    // Start is called before the first frame update
    void Start()
    {
        var displays = new List<XRDisplaySubsystem>();
        SubsystemManager.GetInstances(displays);
            Debug.LogFormat("XRDisplaySubsystem : cnt={0}", displays.Count);
        if (displays.Count > 0)
        {
            Debug.LogFormat("XRDisplaySubsystem : cnt={0}, display0={1}", displays.Count, displays[0].SubsystemDescriptor.ToString());
            displays[0].singlePassRenderingDisabled = true;
        }
    }

    // Update is called once per frame
    void Update()
    {
        
    }
}
