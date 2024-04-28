#pragma once

#include <d3d12.h>
#include <d3dx12.h>

#include <vector>

#include "mesh.h"


// Forward declarations
class Scene;
class Camera;
class Texture;
class IRenderTarget;
class FrameDescriptorHeap;

class IPipeline {
public:
    struct PipelineStateStream {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER RasterizerState;
        CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC BlendState;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DepthStencilState;
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

    FrameDescriptorHeap* m_descriptor_heap;

    // TODO: ensure only DSV targets can bind to m_dsv_rendertarget and RTV rendertargets to rtv_rendertargets
    std::vector<IRenderTarget*> m_rtv_rendertargets;
    IRenderTarget* m_dsv_rendertarget;
    //std::vector<RenderBuffer*> m_rtv_renderbuffers;
    //DepthBuffer* m_dsv_depthbuffer;
    //std::vector<Texture*> m_rtv_textures;
    //Texture* m_dsv_texture;

    D3D_ROOT_SIGNATURE_VERSION m_root_sig_feature_version;

    D3D12_VIEWPORT* m_viewport;
    D3D12_RECT* m_scissor_rect;

    virtual void CreateRootSignature() = 0;

    virtual void CreatePipelineState() = 0;

public:
    IPipeline();

    void Init(FrameDescriptorHeap* descriptor_heap, D3D12_VIEWPORT* viewport, D3D12_RECT* scissor_rect) {
        // TODO: check if init is called before other funcs get called
        m_descriptor_heap = descriptor_heap;
        m_viewport = viewport;
        m_scissor_rect = scissor_rect;

        // cannot do this in constructor, so need a separate init function...
        CreateRootSignature();
        CreatePipelineState();
    }

    //void SetScene(); // set scene to render ?
    void SetRenderTargets(const std::vector<IRenderTarget*>& rtv_rendertargets, IRenderTarget* dsv_rendertarget) {
        m_rtv_rendertargets = rtv_rendertargets;
        m_dsv_rendertarget = dsv_rendertarget;
    }

    // HOW TO RESIZE VIEWPORT AND RENDERTARGET???!!!
    void SetViewport(D3D12_VIEWPORT* viewport) { m_viewport = viewport; }
    void SetScissorRect(D3D12_RECT* scissor_rect) { m_scissor_rect = scissor_rect; }

    void Clear(CommandList& command_list)// not sure if necessary? TODO; check that at creation the clear value is given to rtv/dsv
    {
        if (m_rtv_rendertargets[0])
            m_rtv_rendertargets[0]->Clear(command_list);
        
        if (m_dsv_rendertarget)
            m_dsv_rendertarget->Clear(command_list);
    }

    virtual void Render(unsigned int frame_idx, CommandList& command_list);
};

class ScenePipeline : public IPipeline {
private:
    Scene* m_scene;

    Camera* m_camera;

public:
    ScenePipeline() : m_scene(nullptr), m_camera(nullptr), IPipeline() {

    }

    void SetScene(Scene* scene) {
        m_scene = scene;
    }

    void SetCamera(Camera* camera) { m_camera = camera; }

    virtual void CreateRootSignature() override;

    virtual void CreatePipelineState() override;

    virtual void Render(unsigned int frame_idx, CommandList& command_list) override;
};


class ImagePipeline : public IPipeline {
private:
    ScreenQuad m_quad;
    //TextureLibrary* m_texture_lib;
    Texture* m_texture; // rendered texture to process
    std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> m_texture_handles;//[Renderer::s_num_frames];

    unsigned int m_num_frame_descriptors = 1;

private:
    // Make base class Init private
    using IPipeline::Init;

    virtual void CreateRootSignature() override;
    virtual void CreatePipelineState() override;
public:
    ImagePipeline();

    void Init(CommandQueue* command_queue, FrameDescriptorHeap* descriptor_heap, D3D12_VIEWPORT* viewport, D3D12_RECT* scissor_rect) {
        // Maybe load the screen quad at a different location and then give it to the imagepipeline Init.
        m_quad.Load(command_queue);

        IPipeline::Init(descriptor_heap, viewport, scissor_rect);
    }

    // TODO: Maybe add some safety checks SRV view etc
    void SetInputTexture(Texture* texture);

    virtual void Render(unsigned int frame_idx, CommandList& command_list) override;

    unsigned int GetNumFrameDescriptors() const { return m_num_frame_descriptors; }
};

