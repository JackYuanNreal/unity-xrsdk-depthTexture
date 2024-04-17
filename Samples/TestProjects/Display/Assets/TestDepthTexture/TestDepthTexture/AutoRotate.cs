
using UnityEngine;

/// <summary> An automatic rotate. </summary>
public class AutoRotate : MonoBehaviour
{
    /// <summary> Updates this object. </summary>
    void Update()
    {
        transform.Rotate(Vector3.up, 1f);
    }
}
