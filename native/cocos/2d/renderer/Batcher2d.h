/****************************************************************************
 Copyright (c) 2019-2021 Xiamen Yaji Software Co., Ltd.

 http://www.cocos.com

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated engine source code (the "Software"), a limited,
 worldwide, royalty-free, non-assignable, revocable and non-exclusive license
 to use Cocos Creator solely to develop games on your target platforms. You shall
 not use Cocos Creator software for developing other software or tools that's
 used for developing games. You are not granted to publish, distribute,
 sublicense, and/or sell copies of Cocos Creator.

 The software or tools in this License Agreement are licensed, not sold.
 Xiamen Yaji Software Co., Ltd. reserves all rights not expressly granted to you.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
****************************************************************************/

#pragma once
#include "2d/renderer/RenderDrawInfo.h"
#include "2d/renderer/RenderEntity.h"
#include "2d/renderer/UIMeshBuffer.h"
#include "base/Macros.h"
#include "base/Ptr.h"
#include "base/TypeDef.h"
#include "core/assets/Material.h"
#include "core/memop/Pool.h"
#include "renderer/gfx-base/GFXTexture.h"
#include "renderer/gfx-base/states/GFXSampler.h"
#include "scene/DrawBatch2D.h"

namespace cc {
class Root;
using UIMeshBufferArray = ccstd::vector<UIMeshBuffer*>;
using UIMeshBufferMap = ccstd::unordered_map<uint32_t, UIMeshBufferArray>;

class Batcher2d final {
public:
    Batcher2d();
    explicit Batcher2d(Root* root);
    ~Batcher2d();

    void syncMeshBuffersToNative(uint32_t accId, ccstd::vector<UIMeshBuffer*>&& buffers);

    bool initialize();
    void update();
    void uploadBuffers();
    void reset();

    void addRootNode(Node* node);
    void releaseDescriptorSetCache(gfx::Texture* texture, gfx::Sampler* sampler);

    UIMeshBuffer* getMeshBuffer(uint32_t accId, uint32_t bufferId);
    gfx::Device* getDevice();

    void updateDescriptorSet();

    void fillBuffersAndMergeBatches();
    void walk(Node* node);
    void handlePostRender(RenderEntity* entity);
    void handleColor(RenderEntity* entity, RenderDrawInfo* drawInfo, Node* curNode);
    void handleStaticDrawInfo(RenderEntity* entity, RenderDrawInfo* drawInfo, Node* curNode);
    void handleDynamicDrawInfo(RenderEntity* entity, RenderDrawInfo* drawInfo);
    void generateBatch(RenderEntity* entity, RenderDrawInfo* drawInfo);
    void resetRenderStates();

private:
    bool _isInit = false;

    inline void fillIndexBuffers(RenderDrawInfo* drawInfo) { // NOLINT(readability-convert-member-functions-to-static)
        uint32_t vertexOffset = drawInfo->getVertexOffset();
        uint16_t* ib = drawInfo->getIDataBuffer();

        UIMeshBuffer* buffer = drawInfo->getMeshBuffer();
        uint32_t indexOffset = buffer->getIndexOffset();

        uint16_t* indexb = drawInfo->getIbBuffer();
        uint32_t indexCount = drawInfo->getIbCount();

        memcpy(&ib[indexOffset], indexb, indexCount * sizeof(uint16_t));
        indexOffset += indexCount;

        buffer->setIndexOffset(indexOffset);
    }

    inline void fillVertexBuffers(RenderEntity* entity, RenderDrawInfo* drawInfo) { // NOLINT(readability-convert-member-functions-to-static)
        Node* node = entity->getNode();
        const Mat4& matrix = node->getWorldMatrix();
        uint8_t stride = drawInfo->getStride();
        uint32_t size = drawInfo->getVbCount() * stride;
        float* vbBuffer = drawInfo->getVbBuffer();

        Vec3 temp;
        uint32_t offset = 0;
        for (int i = 0; i < size; i += stride) {
            Render2dLayout* curLayout = drawInfo->getRender2dLayout(i);
            temp.transformMat4(curLayout->position, matrix);

            offset = i;
            vbBuffer[offset++] = temp.x;
            vbBuffer[offset++] = temp.y;
            vbBuffer[offset++] = temp.z;
        }
    }

    inline void setIndexRange(RenderDrawInfo* drawInfo) { // NOLINT(readability-convert-member-functions-to-static)
        UIMeshBuffer* buffer = drawInfo->getMeshBuffer();
        uint32_t indexOffset = drawInfo->getIndexOffset();
        uint32_t indexCount = drawInfo->getIbCount();
        indexOffset += indexCount;
        if (buffer->getIndexOffset() < indexOffset) {
            buffer->setIndexOffset(indexOffset);
        }
    }

    inline void fillColors(RenderEntity* entity, RenderDrawInfo* drawInfo) { // NOLINT(readability-convert-member-functions-to-static)
        Color temp = entity->getColor();

        uint8_t stride = drawInfo->getStride();
        uint32_t size = drawInfo->getVbCount() * stride;
        float* vbBuffer = drawInfo->getVbBuffer();

        uint32_t offset = 0;
        for (int i = 0; i < size; i += stride) {
            offset = i + 5;
            vbBuffer[offset++] = static_cast<float>(temp.r) / 255.0F;
            vbBuffer[offset++] = static_cast<float>(temp.g) / 255.0F;
            vbBuffer[offset++] = static_cast<float>(temp.b) / 255.0F;
            vbBuffer[offset++] = entity->getOpacity();
        }
    }

    gfx::DescriptorSet* getDescriptorSet(gfx::Texture* texture, gfx::Sampler* sampler, gfx::DescriptorSetLayout* dsLayout);

    StencilManager* _stencilManager{nullptr};

    // weak reference
    Root* _root{nullptr};
    // weak reference
    ccstd::vector<Node*> _rootNodeArr;

    // manage memory manually
    ccstd::vector<scene::DrawBatch2D*> _batches;
    memop::Pool<scene::DrawBatch2D> _drawBatchPool;

    // weak reference
    gfx::Device* _device{nullptr}; // use getDevice()

    // weak reference
    RenderEntity* _currEntity{nullptr};
    // weak reference
    RenderDrawInfo* _currDrawInfo{nullptr};
    // weak reference
    UIMeshBuffer* _currMeshBuffer{nullptr};
    uint32_t _indexStart{0};
    ccstd::hash_t _currHash{0};
    uint32_t _currLayer{0};
    StencilStage _currStencilStage{StencilStage::DISABLED};

    // weak reference
    Material* _currMaterial{nullptr};
    // weak reference
    gfx::Texture* _currTexture{nullptr};
    ccstd::hash_t _currTextureHash{0};
    // weak reference
    gfx::Sampler* _currSampler{nullptr};
    ccstd::hash_t _currSamplerHash{0};

    // weak reference
    ccstd::vector<RenderDrawInfo*> _meshRenderDrawInfo;

    // manage memory manually
    ccstd::unordered_map<ccstd::hash_t, gfx::DescriptorSet*> _descriptorSetCache;
    gfx::DescriptorSetInfo _dsInfo;

    UIMeshBufferMap _meshBuffersMap;

    CC_DISALLOW_COPY_MOVE_ASSIGN(Batcher2d);
};
} // namespace cc