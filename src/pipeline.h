#pragma once

#include <d3d12.h>
#include <d3dx12.h>

#include <vector>

#include "mesh.h"
#include "rendertarget.h"


// Forward declarations
class Scene;
class Camera;
class RenderTargetTexture;
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

    FrameDescriptorHeap* m_descriptor_heap;

    // RenderTargets
    std::vector<IRenderTarget*> m_rtv_rendertargets;
    IDepthStencilTarget* m_dsv_rendertarget;

    D3D_ROOT_SIGNATURE_VERSION m_root_sig_feature_version;

    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissor_rect;

    // Compiled shader path different for debug and release
    static const std::wstring s_compiled_shader_path;

    bool m_initialized;

    virtual void CreateRootSignature() = 0;
    virtual void CreatePipelineState() = 0;

public:
    IPipeline();

    void Init(FrameDescriptorHeap* descriptor_heap, unsigned int width, unsigned int height);

    void SetRenderTargets(const std::vector<IRenderTarget*>& rtv_rendertargets, IDepthStencilTarget* dsv_rendertarget) 
    {
        m_rtv_rendertargets = rtv_rendertargets;
        m_dsv_rendertarget = dsv_rendertarget;
    }

    void Resize(unsigned int width, unsigned int height) 
    {
        m_viewport.Width = static_cast<float>(width);
        m_viewport.Height = static_cast<float>(height);
    }

    virtual void Clear(CommandList& command_list);
    virtual void Render(unsigned int frame_idx, CommandList& command_list);
};

class DepthMapPipeline : public IPipeline {
private:
    Scene* m_scene;

private:
    // TODO: hide SetRenderTargets for this pipeline
    // using IPipeline::SetRenderTargets;
    using IPipeline::Init;

    virtual void CreateRootSignature() override;
    virtual void CreatePipelineState() override;
public:
    DepthMapPipeline() : m_scene(nullptr), IPipeline() {

    }

    void Init(FrameDescriptorHeap* descriptor_heap, Scene* scene);

    // The lights are stored in the scene
    void SetScene(Scene* scene) { m_scene = scene; }
    
    virtual void Clear(CommandList& command_list) override;// TODO: for adding pointlights clear needs to clear all depthmaps
    virtual void Render(unsigned int frame_idx, CommandList& command_list) override;
};

class ScenePipeline : public IPipeline {
private:
    Scene* m_scene;
    Camera* m_camera;

private:
    using IPipeline::Init;

    virtual void CreateRootSignature() override;
    virtual void CreatePipelineState() override;

public:
    ScenePipeline() : m_scene(nullptr), m_camera(nullptr), IPipeline() {

    }

    void Init(FrameDescriptorHeap* descriptor_heap, unsigned int width, unsigned int height, Scene* scene, Camera* camera);

    void SetScene(Scene* scene) { m_scene = scene; }
    void SetCamera(Camera* camera) { m_camera = camera; }

    virtual void Render(unsigned int frame_idx, CommandList& command_list) override;
};


class ImagePipeline : public IPipeline {
public:
    struct Options {
        float gamma;
        float exposure;

        static unsigned int GetNum32BitValues() { return sizeof(Options) / 4; }
    };

private:
    ScreenQuad m_quad;
    std::vector<RenderTargetTexture*> m_textures;
    std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> m_texture_handles;

    unsigned int m_num_frame_descriptors = 1;

    Options m_img_options;

private:
    // Make base class Init private
    using IPipeline::Init;

    virtual void CreateRootSignature() override;
    virtual void CreatePipelineState() override;

public:
    ImagePipeline();

    void Init(CommandQueue* command_queue, FrameDescriptorHeap* descriptor_heap, unsigned int width, unsigned int height);
    void SetInputTexture(int frame_idx, RenderTargetTexture* texture);

    virtual void Render(unsigned int frame_idx, CommandList& command_list) override;

    unsigned int GetNumFrameDescriptors() const { return m_num_frame_descriptors; }
    Options* GetOptions() { return &m_img_options; }
};

