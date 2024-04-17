using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Experimental.XR;
using UnityEngine.XR;

public class XREntry : MonoBehaviour
{
    public string match = "Display Sample";

    // Use this for initialization
    void Start ()
    {
        List<XRDisplaySubsystemDescriptor> displays = new List<XRDisplaySubsystemDescriptor>();
        SubsystemManager.GetSubsystemDescriptors(displays);
        Debug.Log("[XREntry] Number of display providers found: " + displays.Count);

        foreach (var d in displays)
        {
            Debug.Log("[XREntry] Scanning display id: " + d.id);

            if (d.id.Contains(match))
            {
                Debug.Log("[XREntry] Creating display " + d.id);
                XRDisplaySubsystem dispInst = d.Create();

                if (dispInst != null)
                {
                    Debug.Log("[XREntry] Starting display " + d.id);
                    dispInst.Start();
                }
            }
        }
    }
}