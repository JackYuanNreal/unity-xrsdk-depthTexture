Shader "Unlit/PreviewDepth"
{
    Properties
    {
    }

    SubShader
    {
        Tags { "RenderType"="Transparent" "Queue" = "Transparent"}
        
        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #include "UnityCG.cginc"
            
            struct appdata
            {
                float4 vertex : POSITION;
                float2 uv : TEXCOORD0;
            };
            
            struct v2f
            {
                float2 uv : TEXCOORD0;
                float4 vertex : SV_POSITION;
            };
            
            // sampler2D _DepthTex;
            
            v2f vert(appdata v)
            {
                v2f o;
                o.vertex = UnityObjectToClipPos(v.vertex);
                o.uv = v.uv;
                return o;
            }
            
            sampler2D _LastCameraDepthTexture;
            fixed4 frag(v2f i) : SV_Target
            {
                // 从深度纹理中采样深度值
                float4 tex = tex2D(_LastCameraDepthTexture, i.uv);
                float depth = tex.r;
                #if defined(UNITY_REVERSED_Z)
                    depth = 1.0f - depth;
                #endif
                
                //linear depth between camera and far clipping plane
                // depth = Linear01Depth(depth);

                depth = depth * depth * depth * depth;
                // 输出灰度颜色
                return float4(depth, depth, depth, 1.0);
            }
            ENDCG
        }
    }
}
