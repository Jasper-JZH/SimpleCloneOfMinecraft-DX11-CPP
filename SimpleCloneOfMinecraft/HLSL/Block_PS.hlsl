#include"Basic.hlsli"

//���Ʒ����������ɫ��
float4 PS(VertexPosHWNormalTex pIn, uint primID: SV_PrimitiveID):SV_Target
{
    
    //�������
    
    float4 texColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    //uint pID = primID / 2;
    switch (primID)
    {
        case 0:
        case 1:
            texColor = g_TexArray.Sample(g_Sam,
        float3(pIn.Tex,g_Index_P_XYZ[0]));
            break;
        case 2:
        case 3:
            texColor = g_TexArray.Sample(g_Sam,
        float3(pIn.Tex, g_Index_N_XYZ[0]));
            break;
        case 4:
        case 5:
            texColor = g_TexArray.Sample(g_Sam,
        float3(pIn.Tex, g_Index_P_XYZ[1]));
            break;
        case 6:
        case 7:
            texColor = g_TexArray.Sample(g_Sam,
        float3(pIn.Tex, g_Index_N_XYZ[1]));
            break;
        case 8:
        case 9:
            texColor = g_TexArray.Sample(g_Sam,
        float3(pIn.Tex, g_Index_P_XYZ[2]));
            break;
        case 10:
        case 11:
            texColor = g_TexArray.Sample(g_Sam,
        float3(pIn.Tex, g_Index_N_XYZ[2]));
            break;
    }
    
    
    //��׼��������
    pIn.NormalW = normalize(pIn.NormalW);
    
    //�������ָ���۾����������Լ��������۾��ľ���
    float3 toEyeW = normalize(g_EyePosW - pIn.PosW);
    float distToEye = distance(g_EyePosW, pIn.PosW);
    
    //��ʼ���������
    float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 A = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 D = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 S = float4(0.0f, 0.0f, 0.0f, 0.0f);
    int i;
    //���㷽���
     [unroll]
    for (i = 0; i < 5; ++i)
    {
        ComputeDirectionalLight(g_Material, g_DirLight[i], pIn.NormalW, toEyeW, A, D, S);
        ambient += A;
        diffuse += D;
        spec += S;
    }
    //������
    [unroll]
    for (i = 0; i < 5; ++i)
    {
        ComputePointLight(g_Material, g_PointLight[i], pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
        ambient += A;
        diffuse += D;
        spec += S;
    }
    //����۹��
    [unroll]
    for (i = 0; i < 5; ++i)
    {
        ComputeSpotLight(g_Material, g_SpotLight[i], pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
        ambient += A;
        diffuse += D;
        spec += S;
    }
  
    float4 litColor = texColor * (ambient + diffuse) + spec;
    litColor.a = texColor.a * g_Material.Diffuse.a;
    
    return litColor;
}