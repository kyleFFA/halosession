#include "../HCMInternal/pch.h"
#include "../HCMInternal/H2CV_Renderer.h"
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <cassert>
using Microsoft::WRL::ComPtr;
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"d3dcompiler.lib")

int main()
{
    ComPtr<ID3D11Device> dev; ComPtr<ID3D11DeviceContext> ctx; D3D_FEATURE_LEVEL feature;
    assert(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&dev,&feature,&ctx)));
    D3D11_TEXTURE2D_DESC td{}; td.Width=td.Height=128;td.MipLevels=td.ArraySize=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count=1;td.BindFlags=D3D11_BIND_RENDER_TARGET;
    ComPtr<ID3D11Texture2D> tex,read;ComPtr<ID3D11RenderTargetView> rtv;
    assert(SUCCEEDED(dev->CreateTexture2D(&td,nullptr,&tex)));assert(SUCCEEDED(dev->CreateRenderTargetView(tex.Get(),nullptr,&rtv)));
    td.BindFlags=0;td.Usage=D3D11_USAGE_STAGING;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    assert(SUCCEEDED(dev->CreateTexture2D(&td,nullptr,&read)));
    // Bind real tessellation shaders to prove the overlay disables AND restores
    // them; merely querying null stages would miss a state-leak regression.
    const char* shader=R"(
struct P {float4 p:POSITION;};
struct F {float edge[3]:SV_TessFactor;float inside:SV_InsideTessFactor;};
F factors(InputPatch<P,3> p){F f;f.edge[0]=f.edge[1]=f.edge[2]=1;f.inside=1;return f;}
[domain("tri")][partitioning("integer")][outputtopology("triangle_cw")][outputcontrolpoints(3)][patchconstantfunc("factors")]
P HS(InputPatch<P,3> p,uint id:SV_OutputControlPointID){return p[id];}
[domain("tri")]float4 DS(F f,float3 uv:SV_DomainLocation,const OutputPatch<P,3> p):SV_Position{return p[0].p*uv.x+p[1].p*uv.y+p[2].p*uv.z;}
)";
    ComPtr<ID3DBlob> hsCode,dsCode,error;ComPtr<ID3D11HullShader> hs;ComPtr<ID3D11DomainShader> ds;
    assert(SUCCEEDED(D3DCompile(shader,strlen(shader),nullptr,nullptr,nullptr,"HS","hs_5_0",0,0,&hsCode,&error)));
    assert(SUCCEEDED(D3DCompile(shader,strlen(shader),nullptr,nullptr,nullptr,"DS","ds_5_0",0,0,&dsCode,&error)));
    assert(SUCCEEDED(dev->CreateHullShader(hsCode->GetBufferPointer(),hsCode->GetBufferSize(),nullptr,&hs)));
    assert(SUCCEEDED(dev->CreateDomainShader(dsCode->GetBufferPointer(),dsCode->GetBufferSize(),nullptr,&ds)));
    h2cv::Renderer renderer; assert(renderer.init(dev.Get()));
    const std::vector<h2cv::Tri> wall={{{3,-4,-4},{3,4,-4},{3,4,4}},{{3,-4,-4},{3,4,4},{3,-4,4}}};
    auto render=[&](bool depth,float alpha,float width,uint32_t color) {
        const float bg[]={0,0,0,1};ctx->ClearRenderTargetView(rtv.Get(),bg);
        D3D11_VIEWPORT original{3,4,64,64,0,1};ctx->RSSetViewports(1,&original);
        ctx->HSSetShader(hs.Get(),nullptr,0);ctx->DSSetShader(ds.Get(),nullptr,0);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        ID3D11RenderTargetView* target=rtv.Get();ctx->OMSetRenderTargets(1,&target,nullptr);
        assert(renderer.begin(ctx.Get(),rtv.Get(),128,128,{0,0,0},{1,0,0},{0,0,1},1.2f,.01f));
        ComPtr<ID3D11HullShader> during;ctx->HSGetShader(&during,nullptr,nullptr);assert(!during);
        renderer.clearOwnDepth();renderer.setStaticOccluders(wall,1);renderer.drawStaticOccluders();
        const h2cv::LineVertex line[]={{5,-1,0,color},{5,1,0,color}};
        renderer.drawLines(line,2,width,0,depth,false,alpha);renderer.end();
        ComPtr<ID3D11HullShader> afterHS;ComPtr<ID3D11DomainShader> afterDS;
        ctx->HSGetShader(&afterHS,nullptr,nullptr);ctx->DSGetShader(&afterDS,nullptr,nullptr);
        assert(afterHS.Get()==hs.Get() && afterDS.Get()==ds.Get());
        D3D11_VIEWPORT after{};UINT count=1;ctx->RSGetViewports(&count,&after);assert(after.TopLeftX==3 && after.TopLeftY==4 && after.Width==64);
        D3D11_PRIMITIVE_TOPOLOGY topology;ctx->IAGetPrimitiveTopology(&topology);assert(topology==D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        ComPtr<ID3D11RenderTargetView> afterRT;ctx->OMGetRenderTargets(1,&afterRT,nullptr);assert(afterRT.Get()==rtv.Get());
        ctx->CopyResource(read.Get(),tex.Get());D3D11_MAPPED_SUBRESOURCE map{};assert(SUCCEEDED(ctx->Map(read.Get(),0,D3D11_MAP_READ,0,&map)));
        unsigned lit=0,sum=0;for(unsigned y=0;y<128;++y)for(unsigned x=0;x<128;++x){auto v=(unsigned char*)map.pData+y*map.RowPitch+x*4;if(v[0])++lit;sum+=v[0];}
        ctx->Unmap(read.Get(),0);return std::pair{lit,sum};
    };
    auto hidden=render(true,0,2,0xff0000ff); assert(hidden.first==0);
    auto xray=render(false,0,2,0xff0000ff); assert(xray.first>20);
    auto faint=render(true,.25f,2,0xff0000ff); assert(faint.first>20 && faint.second<xray.second/2);
    auto half=render(false,0,2,0x800000ff); assert(half.second>xray.second/3 && half.second<xray.second*2/3);
    auto thick=render(false,0,4,0xff0000ff); assert(thick.first>xray.first);
    // End without a begun pass is safe, including error recovery; resize works.
    renderer.end();assert(renderer.begin(ctx.Get(),rtv.Get(),64,64,{0,0,0},{1,0,0},{0,0,1},1.2f,.01f));renderer.end();
    renderer.shutdown();
    // Device recreation must re-upload buffers even when scene/version and
    // vertex count match the preceding level. A null VB with stale capacity
    // or a released static buffer with the old version breaks this path.
    assert(renderer.init(dev.Get()));
    assert(render(false,0,2,0xff0000ff).first>20);
    assert(render(true,0,2,0xff0000ff).first==0);
    renderer.invalidateStaticOccluders();
    assert(render(true,0,2,0xff0000ff).first==0);
    renderer.shutdown();
    std::cout<<"PASS: H2 WARP renderer occlusion/X-ray/hidden alpha/line opacity/width/resize and viewport/target/topology/tessellation-state restoration\n";
}
