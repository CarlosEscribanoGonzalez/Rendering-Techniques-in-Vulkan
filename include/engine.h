#pragma once

#include "common.h"
#include "runtime.h"

namespace MiniEngine
{
    class RenderPassVK;
    class WindowVK;
    class Scene;

    class Engine final
    {
    public:
        static Engine& instance();

        Engine ();
        ~Engine();

        bool initialize();
        void run       ();
        void shutdown  ();
        void resetRenderPasses() 
        { 
            destroyAttachments();
            createAttachments();
            destroyRenderPasses();
            createRenderPasses(); 
        }

        void loadScene     ( const std::string& i_path );
        inline std::array<VkSampler, 1> getSamplers() const { return m_global_samplers; }
        inline Scene& getScene() { return *m_scene; }

    private:
        Engine( const Engine& ) = delete;
        Engine& operator=(const Engine& ) = delete;

        void createSyncObjects  ();
        void destroySyncObjects ();
        void createRenderPasses ();
        void destroyRenderPasses();
        void createAttachments  ();
        void destroyAttachments ();
        void createSamplers     ();
        void destroySamplers    ();
        void updateGlobalBuffers();

        std::vector<std::shared_ptr<RenderPassVK>> m_render_passes;

        struct FrameSemaphores
        {
            VkSemaphore m_render_semaphore;
            VkSemaphore m_presentation_semaphore;
        };
        
        std::array<FrameSemaphores, 3> m_frame_semaphore;
        std::array<VkFence        , 3> m_frame_fence;
        uint32_t                       m_current_frame;

        bool m_resize;
        bool m_close;

        Runtime m_runtime;


        std::shared_ptr<Scene> m_scene;
        Matrix4f m_prev_view;
        
        Attachments m_render_target_attachments;
        std::array<VkSampler, 1> m_global_samplers;
    };
};
