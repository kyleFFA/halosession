#include <cmath>
#include <cassert>
#include <iostream>
#include <wrl/client.h>
#include "../HCMInternal/WorldLinesGpu.h"
#include "../HCMInternal/WorldLinesEmit.h"
#pragma comment(lib, "d3d11.lib")
using Microsoft::WRL::ComPtr;
using namespace worldlines;

int main()
{
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    D3D_FEATURE_LEVEL level;
    assert(SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION, &device, &level, &context)));
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = desc.Height = 128; desc.MipLevels = desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; desc.SampleDesc.Count = 1;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    ComPtr<ID3D11Texture2D> target, readback;
    assert(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &target)));
    ComPtr<ID3D11RenderTargetView> rtv;
    assert(SUCCEEDED(device->CreateRenderTargetView(target.Get(), nullptr, &rtv)));
    desc.BindFlags = 0; desc.Usage = D3D11_USAGE_STAGING; desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    assert(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &readback)));
    auto scene = std::make_shared<Scene>();
    // Two lines behind an opaque wall. Visibility toggles must not remove the wall.
    scene->verts = {{3,-4,-4},{3,4,-4},{3,4,4},{3,-4,4}, {5,-1,-0.5f},{5,1,-0.5f},{5,-1,0.5f},{5,1,0.5f}};
    scene->triIndex = {0,1,2,0,2,3};
    scene->edgeIndex[kStructure] = {4,5}; scene->edgeIndex[kInstance] = {6,7};
    Gpu gpu; assert(gpu.init(device.Get())); gpu.setScene(scene);
    Camera cam{}; cam.fwd = {1,0,0}; cam.up = {0,0,1}; cam.right = {0,1,0};
    DrawOptions opt; opt.gridWidth = 2; opt.gridOpacity = 1;
    float clear[4]{};
    unsigned char corner[4]{};
    auto render = [&]() {
        context->ClearRenderTargetView(rtv.Get(), clear);
        assert(gpu.draw(context.Get(), rtv.Get(), 128, 128, cam, opt));
        // The draw must restore depth state, including after X-ray.
        ComPtr<ID3D11DepthStencilState> state; UINT ref;
        context->OMGetDepthStencilState(&state, &ref); assert(!state);
        context->CopyResource(readback.Get(), target.Get());
        D3D11_MAPPED_SUBRESOURCE mapped{};
        assert(SUCCEEDED(context->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped)));
        int pixels = 0;
        std::memcpy(corner, mapped.pData, 4);
        for (unsigned y=0;y<128;y++) for(unsigned x=0;x<128;x++) {
            auto p = static_cast<const unsigned char*>(mapped.pData) + y*mapped.RowPitch + x*4;
            if(p[0] || p[1] || p[2]) pixels++;
        }
        context->Unmap(readback.Get(),0); return pixels;
    };
    assert(render() == 0);
    opt.xray = true; int both = render(); assert(both > 20);
    opt.showStructure = false; int instances = render(); assert(instances > 0 && instances < both);
    opt.showInstances = false; assert(render() == 0);
    opt.showStructure = true; assert(render() > 0);
    opt.xray = false; assert(render() == 0);

    // Mask replaces a colored native frame, preserving visible grid and opaque alpha.
    clear[0] = 1.f; clear[3] = 1.f;
    opt.xray = true; opt.showInstances = true; opt.blackBackground = true;
    assert(render() == both); assert(corner[0] == 0 && corner[1] == 0 && corner[2] == 0 && corner[3] == 255);
    opt.background = {0.2f,0.4f,0.6f,0.f}; render();
    assert(corner[0] == 51 && corner[1] == 102 && corner[2] == 153 && corner[3] == 255);
    opt.blackBackground = false; render();
    assert(corner[0] == 255 && corner[1] == 0 && corner[2] == 0);
    // Missing geometry must not erase the native frame.
    gpu.setScene(nullptr); opt.blackBackground = true;
    context->ClearRenderTargetView(rtv.Get(), clear);
    assert(!gpu.draw(context.Get(), rtv.Get(),128,128,cam,opt));

    EdgeCells cells; cells.build(*scene); EmitScratch scratch; Emitted out;
    EmitCamera ec{}; ec.fwd={1,0,0}; ec.up={0,0,1}; ec.left={0,-1,0};
    EmitOptions eo; eo.gridWidth=1;
    emit(*scene,cells,ec,eo,scratch,out); auto all = out.lines.size(); assert(all == 4);
    eo.showStructure=false; emit(*scene,cells,ec,eo,scratch,out); assert(out.lines.size()==2);
    eo.showInstances=false; emit(*scene,cells,ec,eo,scratch,out); assert(out.lines.empty());
    eo.showStructure=true; emit(*scene,cells,ec,eo,scratch,out); assert(out.lines.size()==2);
    std::cout << "Grid visibility, occlusion, X-ray and state restoration passed (" << both << " X-ray pixels)\n";
}
