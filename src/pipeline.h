#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <d3dx12.h>

#include <vector>
#include <type_traits>

#include "texture.h"
#include "camera.h"

#include "buffer.h"


class Scene;

class IPipeline {
public:
    struct PipelineStateStream {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
    };

protected:
    // Root signature
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_root_signature;

    // Pipeline state object.
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipeline_state;

    //std::string vertex_shader;
    //std::string pixel_shader;
    ////std::string vertex_shader;
    //std::vector<D3D12_INPUT_ELEMENT_DESC> m_input_layout;
    //D3D12_RT_FORMAT_ARRAY rtvFormats; //getter for this to ? or maybe check this when setting rendertarget?

    //D3D12_CPU_DESCRIPTOR_HANDLE* m_render_target_view; // TODO: multi target rendering maybe pass std::vector
    //D3D12_CPU_DESCRIPTOR_HANDLE* m_depth_stencil_view;

    std::vector<RenderBuffer*> m_rtv_renderbuffers;
    DepthBuffer* m_dsv_depthbuffer;
    std::vector<Texture*> m_rtv_textures;
    Texture* m_dsv_texture;

    D3D_ROOT_SIGNATURE_VERSION m_root_sig_feature_version;

    D3D12_VIEWPORT* m_viewport;
    D3D12_RECT* m_scissor_rect;

    IPipeline();

    virtual void CreateRootSignature() = 0;

    virtual void CreatePipelineState() = 0;

public:
    void Init() {
        // cannot do this in constructor, so need a separate init function...
        CreateRootSignature();
        CreatePipelineState();
    }

    //void SetScene(); // set scene to render ?
    void SetRenderTargets(const std::vector<RenderBuffer*>& renderbuffers, DepthBuffer* depthbuffer) {
        m_rtv_renderbuffers = renderbuffers;
        m_dsv_depthbuffer = depthbuffer;
    }

    // HOW TO RESIZE VIEWPORT AND RENDERTARGET???!!!
    void SetViewport(D3D12_VIEWPORT* viewport) { m_viewport = viewport; }
    void SetScissorRect(D3D12_RECT* scissor_rect) { m_scissor_rect = scissor_rect; }

    void Clear(CommandList& command_list)// not sure if necessary? TODO; check that at creation the clear value is given to rtv/dsv
    {
        m_rtv_renderbuffers[0]->Clear(command_list);
        m_dsv_depthbuffer->Clear(command_list);
    }
    virtual void Render(unsigned int frame_idx, CommandList& command_list) = 0; // image based PSO does not need camera... also dont need scene only need texturedquad
};

class ScenePipeline : public IPipeline {
private:
    Scene* m_scene;

    Camera* m_camera;

public:
    ScenePipeline() : IPipeline() {

    }

    void SetScene(Scene* scene) {
        m_scene = scene;
    }

    void SetCamera(Camera* camera) { m_camera = camera; }

    virtual void CreateRootSignature() override;

    virtual void CreatePipelineState() override;

    virtual void Render(unsigned int frame_idx, CommandList& command_list) override;
};

//class ImagePipeline : public IPipeline {
//private:
//    ScreenQuad m_quad;
//    TextureLibrary* m_texture_lib;
//    Texture* m_texture; // rendered texture to process
//    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbv_srv_heap;
//    unsigned int m_cbv_srv_descriptor_size;
//    unsigned int m_num_frame_descriptors = 1;
//public:
//    ImagePipeline() {
//        Microsoft::WRL::ComPtr<ID3D12Device2> device = Renderer::GetDevice();
//        m_cbv_srv_heap = directx::CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_num_frame_descriptors * Renderer::s_num_frames);
//        m_cbv_srv_descriptor_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
//    }
//
//    virtual void Render(unsigned int frame_idx, CommandList& command_list) override {
//        // setup and draw
//        command_list.SetPipelineState(m_pipeline_state.Get());
//        command_list.SetGraphicsRootSignature(m_root_signature.Get());
//
//        ID3D12DescriptorHeap* heaps[] = { m_cbv_srv_heap.Get(), m_texture_lib->GetSamplerHeap()};
//        command_list.SetDescriptorHeaps(_countof(heaps), heaps); // sparingly do this/ so need a single CBV_SRV Shader visible descriptor heap...
//
//        command_list.SetViewport(*m_viewport);
//        command_list.SetScissorRect(*m_scissor_rect);
//
//        command_list.SetRenderTargets(1, &m_rtv_renderbuffers[0]->GetDescHandle(), &m_dsv_depthbuffer->GetDescHandle());
//        // TODO bind quad
//        CD3DX12_CPU_DESCRIPTOR_HANDLE cbv_srv_cpu_handle(m_cbv_srv_heap->GetCPUDescriptorHandleForHeapStart(), frame_idx * m_num_frame_descriptors * m_cbv_srv_descriptor_size);
//        CD3DX12_GPU_DESCRIPTOR_HANDLE cbv_srv_gpu_handle(m_cbv_srv_heap->GetGPUDescriptorHandleForHeapStart(), frame_idx * m_num_frame_descriptors * m_cbv_srv_descriptor_size);
//        // order of descriptor heap is rendered tex...
//        m_texture->Bind(cbv_srv_cpu_handle);
//        //
//        command_list.SetGraphicsRootDescriptorTable(0, cbv_srv_gpu_handle);
//        command_list.SetGraphicsRootDescriptorTable(1, m_texture_lib->GetSamplerHeapGpuHandle());
//
//        command_list.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//        command_list.SetVertexBuffer(m_quad.GetVertexBufferView());
//        command_list.SetIndexBuffer(m_quad.GetIndexBufferView());
//
//        // Draw
//        command_list.DrawIndexedInstanced(CastToUint(m_quad.GetNumIndices()), 1);
//    }
//};

