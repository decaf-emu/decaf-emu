/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once
#include <vulkan/vulkan.hpp>

#include <QWindow>
#include <QImage>
#include <QMatrix4x4>
#include <QSet>
#include <QVulkanInstance>
#include <QVulkanWindowRenderer>

class QVulkanWindow2 : public QWindow
{
    Q_OBJECT

public:
    enum Flag {
        PersistentResources = 0x01
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    explicit QVulkanWindow2(QWindow *parent = nullptr);
    ~QVulkanWindow2();

    void setFlags(Flags flags);
    Flags flags() const;

    QVector<VkPhysicalDeviceProperties> availablePhysicalDevices();
    void setPhysicalDeviceIndex(int idx);

    QVulkanInfoVector<QVulkanExtension> supportedDeviceExtensions();
    void setDeviceExtensions(const QByteArrayList &extensions);

    void setPreferredColorFormats(const QVector<VkFormat> &formats);

    QVector<int> supportedSampleCounts();
    void setSampleCount(int sampleCount);

    bool isValid() const;

    virtual QVulkanWindowRenderer *createRenderer();
    void frameReady();

    VkPhysicalDevice physicalDevice() const;
    const VkPhysicalDeviceProperties *physicalDeviceProperties() const;
    VkDevice device() const;
    int graphicsQueueFamilyIdx() const;
    VkQueue graphicsQueue() const;
    VkQueue graphicsDriverQueue() const;
    VkCommandPool graphicsCommandPool() const;
    uint32_t hostVisibleMemoryIndex() const;
    uint32_t deviceLocalMemoryIndex() const;
    VkRenderPass defaultRenderPass() const;

    VkFormat colorFormat() const;
    VkFormat depthStencilFormat() const;
    QSize swapChainImageSize() const;

    VkCommandBuffer currentCommandBuffer() const;
    VkFramebuffer currentFramebuffer() const;
    int currentFrame() const;

    static const int MAX_CONCURRENT_FRAME_COUNT = 3;
    int concurrentFrameCount() const;

    int swapChainImageCount() const;
    int currentSwapChainImageIndex() const;
    VkImage swapChainImage(int idx) const;
    VkImageView swapChainImageView(int idx) const;
    VkImage depthStencilImage() const;
    VkImageView depthStencilImageView() const;

    VkSampleCountFlagBits sampleCountFlagBits() const;
    VkImage msaaColorImage(int idx) const;
    VkImageView msaaColorImageView(int idx) const;

    bool supportsGrab() const;
    QImage grab();

    QMatrix4x4 clipCorrectionMatrix();

Q_SIGNALS:
    void frameGrabbed(const QImage &image);

protected:
    void exposeEvent(QExposeEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    bool event(QEvent *) override;

private:
    Q_DISABLE_COPY(QVulkanWindow2)

private:
    void ensureStarted();
    void init();
    void reset();
    bool createDefaultRenderPass();
    void recreateSwapChain();
    uint32_t chooseTransientImageMemType(VkImage img, uint32_t startIndex);
    bool createTransientImage(VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspectMask,
                              VkImage *images, VkDeviceMemory *mem, VkImageView *views, int count);
    void releaseSwapChain();
    void beginFrame();
    void endFrame();
    bool checkDeviceLost(VkResult err);
    void addReadback();
    void finishBlockingReadback();

    enum Status {
        StatusUninitialized,
        StatusFail,
        StatusFailRetry,
        StatusDeviceReady,
        StatusReady
    };
    Status status = StatusUninitialized;
    QVulkanWindowRenderer *renderer = nullptr;
    QVulkanInstance *inst = nullptr;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    int physDevIndex = 0;
    QVector<VkPhysicalDevice> physDevs;
    QVector<VkPhysicalDeviceProperties> physDevProps;
    QVulkanWindow2::Flags mFlags = 0;
    QByteArrayList requestedDevExtensions;
    QHash<VkPhysicalDevice, QVulkanInfoVector<QVulkanExtension> > supportedDevExtensions;
    QVector<VkFormat> requestedColorFormats;
    VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;

    VkDevice dev = VK_NULL_HANDLE;
    QVulkanDeviceFunctions *devFuncs;
    uint32_t gfxQueueFamilyIdx;
    uint32_t presQueueFamilyIdx;
    VkQueue gfxQueue;
    VkQueue gfxDriverQueue; // Graphics queue passed to libgpu
    VkQueue presQueue;
    VkCommandPool cmdPool = VK_NULL_HANDLE;
    VkCommandPool presCmdPool = VK_NULL_HANDLE;
    uint32_t hostVisibleMemIndex;
    uint32_t deviceLocalMemIndex;
    VkFormat mColorFormat;
    VkColorSpaceKHR colorSpace;
    VkFormat dsFormat = VK_FORMAT_D24_UNORM_S8_UINT;

    PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = nullptr;
    PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
    PFN_vkQueuePresentKHR vkQueuePresentKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;

    static const int MAX_SWAPCHAIN_BUFFER_COUNT = 3;
    static const int MAX_FRAME_LAG = QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT;
    // QVulkanWindow only supports the always available FIFO mode. The
    // rendering thread will get throttled to the presentation rate (vsync).
    // This is in effect Example 5 from the VK_KHR_swapchain spec.
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    int swapChainBufferCount = 2;
    int frameLag = 2;

    QSize mSwapChainImageSize;
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    bool swapChainSupportsReadBack = false;

    struct ImageResources {
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkCommandBuffer cmdBuf = VK_NULL_HANDLE;
        VkFence cmdFence = VK_NULL_HANDLE;
        bool cmdFenceWaitable = false;
        VkFramebuffer fb = VK_NULL_HANDLE;
        VkCommandBuffer presTransCmdBuf = VK_NULL_HANDLE;
        VkImage msaaImage = VK_NULL_HANDLE;
        VkImageView msaaImageView = VK_NULL_HANDLE;
    } imageRes[MAX_SWAPCHAIN_BUFFER_COUNT];

    VkDeviceMemory msaaImageMem = VK_NULL_HANDLE;

    uint32_t currentImage;

    struct FrameResources {
        VkFence fence = VK_NULL_HANDLE;
        bool fenceWaitable = false;
        VkSemaphore imageSem = VK_NULL_HANDLE;
        VkSemaphore drawSem = VK_NULL_HANDLE;
        VkSemaphore presTransSem = VK_NULL_HANDLE;
        bool imageAcquired = false;
        bool imageSemWaitable = false;
    } frameRes[MAX_FRAME_LAG];

    uint32_t mCurrentFrame;

    VkRenderPass mDefaultRenderPass = VK_NULL_HANDLE;

    VkDeviceMemory dsMem = VK_NULL_HANDLE;
    VkImage dsImage = VK_NULL_HANDLE;
    VkImageView dsView = VK_NULL_HANDLE;

    bool framePending = false;
    bool frameGrabbing = false;
    QImage frameGrabTargetImage;
    VkImage frameGrabImage = VK_NULL_HANDLE;
    VkDeviceMemory frameGrabImageMem = VK_NULL_HANDLE;

    QMatrix4x4 m_clipCorrect;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QVulkanWindow2::Flags)
