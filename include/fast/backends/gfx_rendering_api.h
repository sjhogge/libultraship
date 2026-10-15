#pragma once

#include <stdint.h>

#include <unordered_map>
#include <set>
#include "imconfig.h"

namespace Fast {
struct ShaderProgram;

struct GfxClipParameters {
    bool z_is_from_0_to_1;
    bool invertY;
};

enum FilteringMode { FILTER_THREE_POINT, FILTER_LINEAR, FILTER_NONE };

// A hash function used to hash a: pair<float, float>
struct hash_pair_ff {
    size_t operator()(const std::pair<float, float>& p) const {
        const auto hash1 = std::hash<float>{}(p.first);
        const auto hash2 = std::hash<float>{}(p.second);

        // If hash1 == hash2, their XOR is zero.
        return (hash1 != hash2) ? hash1 ^ hash2 : hash1;
    }
};

class GfxRenderingAPI {
  public:
    virtual ~GfxRenderingAPI() = default;
    virtual const char* GetName() = 0;
    virtual int GetMaxTextureSize() = 0;
    virtual GfxClipParameters GetClipParameters() = 0;
    virtual void UnloadShader(ShaderProgram* oldPrg) = 0;
    virtual void LoadShader(ShaderProgram* newPrg) = 0;
    virtual ShaderProgram* CreateAndLoadNewShader(uint64_t shaderId0, uint32_t shaderId1) = 0;
    virtual ShaderProgram* LookupShader(uint64_t shaderId0, uint32_t shaderId1) = 0;
    virtual void ShaderGetInfo(ShaderProgram* prg, uint8_t* numInputs, bool usedTextures[2]) = 0;
    virtual uint32_t NewTexture() = 0;
    virtual void SelectTexture(int tile, uint32_t textureId) = 0;
    virtual void UploadTexture(const uint8_t* rgba32Buf, uint32_t width, uint32_t height) = 0;
    virtual void SetSamplerParameters(int sampler, bool linear_filter, uint32_t cms, uint32_t cmt) = 0;
    virtual void SetDepthTestAndMask(bool depth_test, bool z_upd) = 0;
    virtual void SetZmodeDecal(bool decal) = 0;
    virtual void SetViewport(int x, int y, int width, int height) = 0;
    virtual void SetScissor(int x, int y, int width, int height) = 0;
    virtual void SetUseAlpha(bool useAlpha) = 0;
    virtual void DrawTriangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) = 0;
    virtual void Init() = 0;
    virtual void OnResize() = 0;
    virtual void StartFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void FinishRender() = 0;
    virtual int CreateFramebuffer() = 0;
    virtual void UpdateFramebufferParameters(int fb_id, uint32_t width, uint32_t height, uint32_t msaa_level,
                                             bool opengl_invertY, bool render_target, bool has_depth_buffer,
                                             bool can_extract_depth) = 0;
    // Optional capture capability. Supporting backends configure the target
    // to accumulate real alpha and arrange for each render replay to begin
    // with its full physical extent cleared to transparent black. Returning
    // false tells callers to retain their portable chroma-key fallback.
    virtual bool PrepareFramebufferForTransparentCapture(int fbId) {
        (void)fbId;
        return false;
    }
    virtual void StartDrawToFramebuffer(int fbId, float noiseScale) = 0;
    virtual void CopyFramebuffer(int fbDstId, int fbSrcId, int srcX0, int srcY0, int srcX1, int srcY1, int dstX0,
                                 int dstY0, int dstX1, int dstY1) = 0;
    // Optional exact, full-resource GPU copy. Unlike CopyFramebuffer this has
    // no viewport/scissor dependence and reports unsupported/mismatched targets.
    virtual bool CopyFramebufferSameSize(int fbDstId, int fbSrcId) {
        (void)fbDstId;
        (void)fbSrcId;
        return false;
    }
    virtual void ClearFramebuffer(bool color, bool depth) = 0;
    virtual void ReadFramebufferToCPU(int fbId, uint32_t width, uint32_t height, uint16_t* rgba16Buf) = 0;
    // Optional lossless color readback. The returned bytes are tightly packed
    // RGBA8 in top-to-bottom row order. Backends which do not implement this
    // capability fail closed so callers can retain an older RGBA5551 fallback.
    virtual bool ReadFramebufferColorToCPU(int fbId, uint32_t width, uint32_t height, uint8_t* rgba8Buf) {
        return false;
    }
    // Optional capability. Backends which cannot expose an R32 device-depth
    // plane fail closed; callers must negotiate color-only rendering instead.
    virtual bool ReadFramebufferDepthToCPU(int fbId, uint32_t width, uint32_t height, float* depthBuf) {
        return false;
    }
    // Optional cross-process GPU sharing capability. Synchronization is
    // deliberately nonblocking: AcquireFramebufferSharedTexture must use a
    // zero timeout so a slow or dead peer can never stall the renderer.
    virtual bool SupportsFramebufferSharedTexture() const {
        return false;
    }
    // The returned NT handle remains owned by this process until the resource
    // is resized, disabled, or destroyed. A peer duplicates it out of the
    // producer process; it must not close this numeric value in its own process.
    virtual bool GetFramebufferSharedTextureHandle(int fbId, uint64_t* sourceHandle) {
        (void)fbId;
        if (sourceHandle != nullptr) {
            *sourceHandle = 0;
        }
        return false;
    }
    // Legacy direction: the returned duplicate is owned by targetProcessId.
    virtual bool ExportFramebufferSharedTexture(int fbId, uint32_t targetProcessId, uint64_t* remoteHandle) {
        (void)fbId;
        (void)targetProcessId;
        if (remoteHandle != nullptr) {
            *remoteHandle = 0;
        }
        return false;
    }
    // Closes a remote duplicate only when the caller can prove that its value
    // has never been published to the target process. Published handles are
    // consumer-owned and must never be revoked by numeric value.
    virtual bool AbandonFramebufferSharedTextureHandle(int fbId) {
        (void)fbId;
        return false;
    }
    virtual bool AcquireFramebufferSharedTexture(int fbId, uint64_t key) {
        (void)fbId;
        (void)key;
        return false;
    }
    virtual bool ReleaseFramebufferSharedTexture(int fbId, uint64_t key) {
        (void)fbId;
        (void)key;
        return false;
    }
    virtual void DisableFramebufferSharedTexture(int fbId) {
        (void)fbId;
    }
    virtual void ResolveMSAAColorBuffer(int fbIdTarger, int fbIdSrc) = 0;
    virtual std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff>
    GetPixelDepth(int fb_id, const std::set<std::pair<float, float>>& coordinates) = 0;
    virtual void* GetFramebufferTextureId(int fbId) = 0;
    virtual void SelectTextureFb(int fbId) = 0;
    virtual void DeleteTexture(uint32_t texId) = 0;
    virtual void SetTextureFilter(FilteringMode mode) = 0;
    virtual FilteringMode GetTextureFilter() = 0;
    virtual void SetSrgbMode() = 0;
    virtual ImTextureID GetTextureById(int id) = 0;

  protected:
    int8_t mCurrentDepthTest = 0;
    int8_t mCurrentDepthMask = 0;
    int8_t mCurrentZmodeDecal = 0;
    int8_t mLastDepthTest = -1;
    int8_t mLastDepthMask = -1;
    int8_t mLastZmodeDecal = -1;
    bool mSrgbMode = false;
};
} // namespace Fast
