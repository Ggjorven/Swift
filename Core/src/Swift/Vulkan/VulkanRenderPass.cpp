#include "swpch.h"
#include "VulkanRenderPass.hpp"

#include "Swift/Core/Logging.hpp"
#include "Swift/Core/Application.hpp"

#include "Swift/Renderer/Renderer.hpp"

#include "Swift/Vulkan/VulkanUtils.hpp"
#include "Swift/Vulkan/VulkanRenderer.hpp"

namespace Swift
{

    VulkanRenderPass::VulkanRenderPass(RenderPassSpecification specs, Ref<CommandBuffer> commandBuffer)
        : m_Specification(specs), m_CommandBuffer(RefHelper::RefAs<VulkanCommandBuffer>(commandBuffer))
    {
        Create();
    }


    VulkanRenderPass::~VulkanRenderPass()
    {
        Destroy();
    }

    void VulkanRenderPass::Begin()
    {
        m_CommandBuffer->Begin();

        auto renderer = (VulkanRenderer*)Renderer::GetInstance();
        VkExtent2D extent = { Application::Get().GetWindow().GetWidth(), Application::Get().GetWindow().GetHeight() };

        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_RenderPass;
        renderPassInfo.framebuffer = m_Framebuffers[renderer->GetSwapChain()->GetAquiredImage()];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = extent;

        std::vector<VkClearValue> clearValues = {};
        if (m_Specification.Attachments & RenderPassAttachments::Colour)
        {
            VkClearValue colourClear = {{ { m_Specification.ColourClearColour.r, m_Specification.ColourClearColour.g, m_Specification.ColourClearColour.b, m_Specification.ColourClearColour.a } }};
            clearValues.push_back(colourClear);
        }
        if (m_Specification.Attachments & RenderPassAttachments::Depth)
        {
            VkClearValue depthClear = { { { 1.0f, 0 } } };
            clearValues.push_back(depthClear);
        }

        renderPassInfo.clearValueCount = (uint32_t)clearValues.size();
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(m_CommandBuffer->GetVulkanCommandBuffer(renderer->GetSwapChain()->GetCurrentFrame()), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)extent.width;
        viewport.height = (float)extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(m_CommandBuffer->GetVulkanCommandBuffer(renderer->GetSwapChain()->GetCurrentFrame()), 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset = { 0, 0 };
        scissor.extent = extent;
        vkCmdSetScissor(m_CommandBuffer->GetVulkanCommandBuffer(renderer->GetSwapChain()->GetCurrentFrame()), 0, 1, &scissor);
    }

    void VulkanRenderPass::End()
    {
        auto renderer = (VulkanRenderer*)Renderer::GetInstance();
        vkCmdEndRenderPass(m_CommandBuffer->GetVulkanCommandBuffer(renderer->GetSwapChain()->GetCurrentFrame()));

        m_CommandBuffer->End();
    }

    void VulkanRenderPass::Submit()
    {
        m_CommandBuffer->Submit(Queue::Graphics);
    }

    void VulkanRenderPass::Resize(uint32_t width, uint32_t height)
    {
        auto renderer = (VulkanRenderer*)Renderer::GetInstance();

        auto frameBuffers = m_Framebuffers;
        Renderer::SubmitFree([renderer, frameBuffers]()
        {
            for (auto& framebuffer : frameBuffers)
                vkDestroyFramebuffer(renderer->GetLogicalDevice()->GetVulkanDevice(), framebuffer, nullptr);
        });
        m_Framebuffers.clear();

        auto imageViews = renderer->GetSwapChain()->GetImageViews();
        m_Framebuffers.resize(imageViews.size());
        for (size_t i = 0; i < imageViews.size(); i++)
        {
            std::vector<VkImageView> attachments = { };
            if (m_Specification.Attachments & RenderPassAttachments::Colour)
                attachments.push_back(renderer->GetSwapChain()->GetImageViews()[i]);
            if (m_Specification.Attachments & RenderPassAttachments::Depth)
                attachments.push_back(renderer->GetSwapChain()->GetDepthImageView());

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_RenderPass;
            framebufferInfo.attachmentCount = (uint32_t)attachments.size();
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = width;
            framebufferInfo.height = height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(renderer->GetLogicalDevice()->GetVulkanDevice(), &framebufferInfo, nullptr, &m_Framebuffers[i]) != VK_SUCCESS)
                APP_LOG_ERROR("Failed to create framebuffer!");
        }
    }

    void VulkanRenderPass::Create()
    {
        auto renderer = (VulkanRenderer*)Renderer::GetInstance();

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Renderpass
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        std::vector<VkAttachmentDescription> attachments = { };
        std::vector<VkAttachmentReference> attachmentRefs = { };

        if (m_Specification.Attachments & RenderPassAttachments::Colour)
        {
            VkAttachmentDescription& colorAttachment = attachments.emplace_back();
            colorAttachment.format = renderer->GetSwapChain()->GetColourFormat();
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachment.loadOp = (VkAttachmentLoadOp)m_Specification.ColourLoadOp;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = (VkImageLayout)m_Specification.PreviousImageLayout;
            colorAttachment.finalLayout = (VkImageLayout)m_Specification.FinalImageLayout;

            VkAttachmentReference& colorAttachmentRef = attachmentRefs.emplace_back();
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        if (m_Specification.Attachments & RenderPassAttachments::Depth)
        {
            VkAttachmentDescription& depthAttachment = attachments.emplace_back();
            depthAttachment.format = VulkanAllocator::FindDepthFormat();
            depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkAttachmentReference& depthAttachmentRef = attachmentRefs.emplace_back();
            depthAttachmentRef.attachment = 1;
            depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        if (m_Specification.Attachments & RenderPassAttachments::Colour)
        {
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &attachmentRefs[0];
        }
        else
        {
            subpass.colorAttachmentCount = 0;
            subpass.pColorAttachments = nullptr;
        }

        if (m_Specification.Attachments & RenderPassAttachments::Depth)
        {
            if (m_Specification.Attachments & RenderPassAttachments::Colour)
                subpass.pDepthStencilAttachment = &attachmentRefs[1];
            else
                subpass.pDepthStencilAttachment = &attachmentRefs[0];
        }

        std::array<VkSubpassDependency, 2> dependencies = { };
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = (uint32_t)attachments.size();
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = (uint32_t)dependencies.size();
        renderPassInfo.pDependencies = dependencies.data();

        if (vkCreateRenderPass(renderer->GetLogicalDevice()->GetVulkanDevice(), &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
            APP_LOG_ERROR("Failed to create render pass!");

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Framebuffers // TODO: Framebuffer class
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        auto imageViews = renderer->GetSwapChain()->GetImageViews();
        m_Framebuffers.resize(imageViews.size());
        for (size_t i = 0; i < imageViews.size(); i++)
        {
            std::vector<VkImageView> attachments = { };
            if (m_Specification.Attachments & RenderPassAttachments::Colour)
                attachments.push_back(renderer->GetSwapChain()->GetImageViews()[i]);
            if (m_Specification.Attachments & RenderPassAttachments::Depth)
                attachments.push_back(renderer->GetSwapChain()->GetDepthImageView());

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_RenderPass;
            framebufferInfo.attachmentCount = (uint32_t)attachments.size();
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = Application::Get().GetWindow().GetWidth();
            framebufferInfo.height = Application::Get().GetWindow().GetHeight();
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(renderer->GetLogicalDevice()->GetVulkanDevice(), &framebufferInfo, nullptr, &m_Framebuffers[i]) != VK_SUCCESS)
                APP_LOG_ERROR("Failed to create framebuffer!");
        }
    }

    void VulkanRenderPass::Destroy()
    {
        auto frameBuffers = m_Framebuffers;
        auto renderPass = m_RenderPass;

        Renderer::SubmitFree([frameBuffers, renderPass]()
        {
            auto device = ((VulkanRenderer*)Renderer::GetInstance())->GetLogicalDevice()->GetVulkanDevice();
            vkDeviceWaitIdle(device);

            for (auto& framebuffer : frameBuffers)
                vkDestroyFramebuffer(device, framebuffer, nullptr);

            vkDestroyRenderPass(device, renderPass, nullptr);
        });
    }

}