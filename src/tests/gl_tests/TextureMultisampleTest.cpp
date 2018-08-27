//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureMultisampleTest: Tests of multisampled texture

#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"

using namespace angle;

namespace
{
// Sample positions of d3d standard pattern. Some of the sample positions might not the same as
// opengl.
using SamplePositionsArray                                            = std::array<float, 32>;
static constexpr std::array<SamplePositionsArray, 5> kSamplePositions = {
    {{{0.5f, 0.5f}},
     {{0.75f, 0.75f, 0.25f, 0.25f}},
     {{0.375f, 0.125f, 0.875f, 0.375f, 0.125f, 0.625f, 0.625f, 0.875f}},
     {{0.5625f, 0.3125f, 0.4375f, 0.6875f, 0.8125f, 0.5625f, 0.3125f, 0.1875f, 0.1875f, 0.8125f,
       0.0625f, 0.4375f, 0.6875f, 0.9375f, 0.9375f, 0.0625f}},
     {{0.5625f, 0.5625f, 0.4375f, 0.3125f, 0.3125f, 0.625f,  0.75f,   0.4375f,
       0.1875f, 0.375f,  0.625f,  0.8125f, 0.8125f, 0.6875f, 0.6875f, 0.1875f,
       0.375f,  0.875f,  0.5f,    0.0625f, 0.25f,   0.125f,  0.125f,  0.75f,
       0.0f,    0.5f,    0.9375f, 0.25f,   0.875f,  0.9375f, 0.0625f, 0.0f}}}};

class TextureMultisampleTest : public ANGLETest
{
  protected:
    TextureMultisampleTest()
    {
        setWindowWidth(64);
        setWindowHeight(64);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }

    void SetUp() override
    {
        ANGLETest::SetUp();

        glGenFramebuffers(1, &mFramebuffer);
        glGenTextures(1, &mTexture);

        ASSERT_GL_NO_ERROR();
    }

    void TearDown() override
    {
        glDeleteFramebuffers(1, &mFramebuffer);
        mFramebuffer = 0;
        glDeleteTextures(1, &mTexture);
        mTexture = 0;

        ANGLETest::TearDown();
    }

    GLuint mFramebuffer = 0;
    GLuint mTexture     = 0;

    // Returns a sample count that can be used with the given texture target for all the given
    // formats. Assumes that if format A supports a number of samples N and another format B
    // supports a number of samples M > N then format B also supports number of samples N.
    GLint getSamplesToUse(GLenum texTarget, const std::vector<GLenum> &formats)
    {
        GLint maxSamples = 65536;
        for (GLenum format : formats)
        {
            GLint maxSamplesFormat = 0;
            glGetInternalformativ(texTarget, format, GL_SAMPLES, 1, &maxSamplesFormat);
            maxSamples = std::min(maxSamples, maxSamplesFormat);
        }
        return maxSamples;
    }
};

class TextureMultisampleTestES31 : public TextureMultisampleTest
{
  protected:
    TextureMultisampleTestES31() : TextureMultisampleTest() {}
};

class TextureMultisampleArrayWebGLTest : public TextureMultisampleTest
{
  protected:
    TextureMultisampleArrayWebGLTest() : TextureMultisampleTest()
    {
        // These tests run in WebGL mode so we can test with both extension off and on.
        setWebGLCompatibilityEnabled(true);
    }

    // Requests the ANGLE_texture_multisample_array extension and returns true if the operation
    // succeeds.
    bool requestArrayExtension()
    {
        if (extensionRequestable("GL_ANGLE_texture_multisample_array"))
        {
            glRequestExtensionANGLE("GL_ANGLE_texture_multisample_array");
        }

        if (!extensionEnabled("GL_ANGLE_texture_multisample_array"))
        {
            return false;
        }
        return true;
    }
};

// Tests that if es version < 3.1, GL_TEXTURE_2D_MULTISAMPLE is not supported in
// GetInternalformativ. Checks that the number of samples returned is valid in case of ES >= 3.1.
TEST_P(TextureMultisampleTest, MultisampleTargetGetInternalFormativBase)
{
    // This query returns supported sample counts in descending order. If only one sample count is
    // queried, it should be the maximum one.
    GLint maxSamplesR8 = 0;
    glGetInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, GL_R8, GL_SAMPLES, 1, &maxSamplesR8);
    if (getClientMajorVersion() < 3 || getClientMinorVersion() < 1)
    {
        ASSERT_GL_ERROR(GL_INVALID_ENUM);
    }
    else
    {
        ASSERT_GL_NO_ERROR();

        // GLES 3.1 section 19.3.1 specifies the required minimum of how many samples are supported.
        GLint maxColorTextureSamples;
        glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &maxColorTextureSamples);
        GLint maxSamples;
        glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
        GLint maxSamplesR8Required = std::min(maxColorTextureSamples, maxSamples);

        EXPECT_GE(maxSamplesR8, maxSamplesR8Required);
    }
}

// Tests that if es version < 3.1, GL_TEXTURE_2D_MULTISAMPLE is not supported in
// FramebufferTexture2D.
TEST_P(TextureMultisampleTest, MultisampleTargetFramebufferTexture2D)
{
    GLint samples = 1;
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mTexture);
    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA8, 64, 64, GL_FALSE);
    if (getClientMajorVersion() < 3 || getClientMinorVersion() < 1)
    {
        ASSERT_GL_ERROR(GL_INVALID_ENUM);
    }
    else
    {
        ASSERT_GL_NO_ERROR();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
                           mTexture, 0);
    if (getClientMajorVersion() < 3 || getClientMinorVersion() < 1)
    {
        ASSERT_GL_ERROR(GL_INVALID_OPERATION);
    }
    else
    {
        ASSERT_GL_NO_ERROR();
    }
}

// Tests basic functionality of glTexStorage2DMultisample.
TEST_P(TextureMultisampleTestES31, ValidateTextureStorageMultisampleParameters)
{
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mTexture);
    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 1, GL_RGBA8, 1, 1, GL_FALSE);
    ASSERT_GL_NO_ERROR();

    GLint params = 0;
    glGetTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_IMMUTABLE_FORMAT, &params);
    EXPECT_EQ(1, params);

    glTexStorage2DMultisample(GL_TEXTURE_2D, 1, GL_RGBA8, 1, 1, GL_FALSE);
    ASSERT_GL_ERROR(GL_INVALID_ENUM);

    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 1, GL_RGBA8, 0, 0, GL_FALSE);
    ASSERT_GL_ERROR(GL_INVALID_VALUE);

    GLint maxSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 1, GL_RGBA8, maxSize + 1, 1, GL_FALSE);
    ASSERT_GL_ERROR(GL_INVALID_VALUE);

    GLint maxSamples = 0;
    glGetInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, GL_R8, GL_SAMPLES, 1, &maxSamples);
    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, maxSamples + 1, GL_RGBA8, 1, 1, GL_FALSE);
    ASSERT_GL_ERROR(GL_INVALID_OPERATION);

    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 0, GL_RGBA8, 1, 1, GL_FALSE);
    ASSERT_GL_ERROR(GL_INVALID_VALUE);

    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 1, GL_RGBA, 0, 0, GL_FALSE);
    ASSERT_GL_ERROR(GL_INVALID_VALUE);

    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 1, GL_RGBA8, 1, 1, GL_FALSE);
    ASSERT_GL_ERROR(GL_INVALID_OPERATION);
}

// Tests the value of MAX_INTEGER_SAMPLES is no less than 1.
// [OpenGL ES 3.1 SPEC Table 20.40]
TEST_P(TextureMultisampleTestES31, MaxIntegerSamples)
{
    GLint maxIntegerSamples;
    glGetIntegerv(GL_MAX_INTEGER_SAMPLES, &maxIntegerSamples);
    EXPECT_GE(maxIntegerSamples, 1);
    EXPECT_NE(std::numeric_limits<GLint>::max(), maxIntegerSamples);
}

// Tests the value of MAX_COLOR_TEXTURE_SAMPLES is no less than 1.
// [OpenGL ES 3.1 SPEC Table 20.40]
TEST_P(TextureMultisampleTestES31, MaxColorTextureSamples)
{
    GLint maxColorTextureSamples;
    glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &maxColorTextureSamples);
    EXPECT_GE(maxColorTextureSamples, 1);
    EXPECT_NE(std::numeric_limits<GLint>::max(), maxColorTextureSamples);
}

// Tests the value of MAX_DEPTH_TEXTURE_SAMPLES is no less than 1.
// [OpenGL ES 3.1 SPEC Table 20.40]
TEST_P(TextureMultisampleTestES31, MaxDepthTextureSamples)
{
    GLint maxDepthTextureSamples;
    glGetIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &maxDepthTextureSamples);
    EXPECT_GE(maxDepthTextureSamples, 1);
    EXPECT_NE(std::numeric_limits<GLint>::max(), maxDepthTextureSamples);
}

// The value of sample position should be equal to standard pattern on D3D.
TEST_P(TextureMultisampleTestES31, CheckSamplePositions)
{
    ANGLE_SKIP_TEST_IF(!IsD3D11());

    GLsizei maxSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);

    GLfloat samplePosition[2];

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFramebuffer);

    for (int sampleCount = 1; sampleCount <= maxSamples; sampleCount++)
    {
        GLTexture texture;
        size_t indexKey = static_cast<size_t>(ceil(log2(sampleCount)));
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture);
        glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, sampleCount, GL_RGBA8, 1, 1, GL_TRUE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
                               texture, 0);
        EXPECT_GLENUM_EQ(GL_FRAMEBUFFER_COMPLETE, glCheckFramebufferStatus(GL_FRAMEBUFFER));
        ASSERT_GL_NO_ERROR();

        for (int sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++)
        {
            glGetMultisamplefv(GL_SAMPLE_POSITION, sampleIndex, samplePosition);
            EXPECT_EQ(samplePosition[0], kSamplePositions[indexKey][2 * sampleIndex]);
            EXPECT_EQ(samplePosition[1], kSamplePositions[indexKey][2 * sampleIndex + 1]);
        }
    }

    ASSERT_GL_NO_ERROR();
}

// Tests that GL_TEXTURE_2D_MULTISAMPLE_ARRAY is not supported in GetInternalformativ when the
// extension is not supported.
TEST_P(TextureMultisampleArrayWebGLTest, MultisampleArrayTargetGetInternalFormativWithoutExtension)
{
    GLint maxSamples = 0;
    glGetInternalformativ(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_RGBA8, GL_SAMPLES, 1,
                          &maxSamples);
    ASSERT_GL_ERROR(GL_INVALID_ENUM);
}

// Attempt to bind a texture to multisample array binding point when extension is not supported.
TEST_P(TextureMultisampleArrayWebGLTest, BindMultisampleArrayTextureWithoutExtension)
{
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, mTexture);
    ASSERT_GL_ERROR(GL_INVALID_ENUM);
}

// Tests that GL_TEXTURE_2D_MULTISAMPLE_ARRAY is supported in GetInternalformativ.
TEST_P(TextureMultisampleArrayWebGLTest, MultisampleArrayTargetGetInternalFormativ)
{
    ANGLE_SKIP_TEST_IF(!requestArrayExtension());

    // This query returns supported sample counts in descending order. If only one sample count is
    // queried, it should be the maximum one.
    GLint maxSamplesRGBA8 = 0;
    glGetInternalformativ(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_RGBA8, GL_SAMPLES, 1,
                          &maxSamplesRGBA8);
    GLint maxSamplesDepth = 0;
    glGetInternalformativ(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_DEPTH_COMPONENT24, GL_SAMPLES,
                          1, &maxSamplesDepth);
    ASSERT_GL_NO_ERROR();

    // GLES 3.1 section 19.3.1 specifies the required minimum of how many samples are supported.
    GLint maxColorTextureSamples;
    glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &maxColorTextureSamples);
    GLint maxDepthTextureSamples;
    glGetIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &maxDepthTextureSamples);
    GLint maxSamples;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);

    GLint maxSamplesRGBA8Required = std::min(maxColorTextureSamples, maxSamples);
    EXPECT_GE(maxSamplesRGBA8, maxSamplesRGBA8Required);

    GLint maxSamplesDepthRequired = std::min(maxDepthTextureSamples, maxSamples);
    EXPECT_GE(maxSamplesDepth, maxSamplesDepthRequired);
}

// Tests that TexImage3D call cannot be used for GL_TEXTURE_2D_MULTISAMPLE_ARRAY.
TEST_P(TextureMultisampleArrayWebGLTest, MultiSampleArrayTexImage)
{
    ANGLE_SKIP_TEST_IF(!requestArrayExtension());

    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, mTexture);
    ASSERT_GL_NO_ERROR();

    glTexImage3D(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, 0, GL_RGBA8, 1, 1, 1, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);
}

// Tests passing invalid parameters to TexStorage3DMultisample.
TEST_P(TextureMultisampleArrayWebGLTest, InvalidTexStorage3DMultisample)
{
    ANGLE_SKIP_TEST_IF(!requestArrayExtension());

    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, mTexture);
    ASSERT_GL_NO_ERROR();

    // Invalid target
    glTexStorage3DMultisampleANGLE(GL_TEXTURE_2D_MULTISAMPLE, 2, GL_RGBA8, 1, 1, 1, GL_TRUE);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);

    // Samples 0
    glTexStorage3DMultisampleANGLE(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, 0, GL_RGBA8, 1, 1, 1,
                                   GL_TRUE);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);

    // Unsized internalformat
    glTexStorage3DMultisampleANGLE(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, 2, GL_RGBA, 1, 1, 1,
                                   GL_TRUE);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);

    // Width 0
    glTexStorage3DMultisampleANGLE(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, 2, GL_RGBA8, 0, 1, 1,
                                   GL_TRUE);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);

    // Height 0
    glTexStorage3DMultisampleANGLE(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, 2, GL_RGBA8, 1, 0, 1,
                                   GL_TRUE);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);

    // Depth 0
    glTexStorage3DMultisampleANGLE(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, 2, GL_RGBA8, 1, 1, 0,
                                   GL_TRUE);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);
}

// Tests passing invalid parameters to TexParameteri.
TEST_P(TextureMultisampleArrayWebGLTest, InvalidTexParameteri)
{
    ANGLE_SKIP_TEST_IF(!requestArrayExtension());

    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, mTexture);
    ASSERT_GL_NO_ERROR();

    // None of the sampler parameters can be set on GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE.
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);

    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);

    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_TEXTURE_MIN_LOD, 0);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_TEXTURE_MAX_LOD, 0);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);

    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_TEXTURE_COMPARE_FUNC, GL_ALWAYS);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);

    // Only valid base level on GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE is 0.
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_TEXTURE_BASE_LEVEL, 1);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
}

// Test a valid TexStorage3DMultisample call and check that the queried texture level parameters
// match. Does not do any drawing.
TEST_P(TextureMultisampleArrayWebGLTest, TexStorage3DMultisample)
{
    ANGLE_SKIP_TEST_IF(!requestArrayExtension());

    GLint maxSamplesRGBA8 = 0;
    glGetInternalformativ(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_RGBA8, GL_SAMPLES, 1,
                          &maxSamplesRGBA8);

    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, mTexture);
    ASSERT_GL_NO_ERROR();

    glTexStorage3DMultisampleANGLE(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, maxSamplesRGBA8, GL_RGBA8,
                                   8, 4, 2, GL_TRUE);
    ASSERT_GL_NO_ERROR();

    GLint width = 0, height = 0, depth = 0, samples = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, 0, GL_TEXTURE_HEIGHT, &height);
    glGetTexLevelParameteriv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, 0, GL_TEXTURE_DEPTH, &depth);
    glGetTexLevelParameteriv(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, 0, GL_TEXTURE_SAMPLES,
                             &samples);
    ASSERT_GL_NO_ERROR();

    EXPECT_EQ(8, width);
    EXPECT_EQ(4, height);
    EXPECT_EQ(2, depth);
    EXPECT_EQ(maxSamplesRGBA8, samples);
}

// Test for invalid FramebufferTextureLayer calls with GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE
// textures.
TEST_P(TextureMultisampleArrayWebGLTest, InvalidFramebufferTextureLayer)
{
    ANGLE_SKIP_TEST_IF(!requestArrayExtension());

    GLint maxSamplesRGBA8 = 0;
    glGetInternalformativ(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_RGBA8, GL_SAMPLES, 1,
                          &maxSamplesRGBA8);

    GLint maxArrayTextureLayers;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxArrayTextureLayers);

    // Test framebuffer status with just a color texture attached.
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, mTexture);
    glTexStorage3DMultisampleANGLE(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, maxSamplesRGBA8, GL_RGBA8,
                                   4, 4, 2, GL_TRUE);
    ASSERT_GL_NO_ERROR();

    // Test with mip level 1 and -1 (only level 0 is valid for multisample textures).
    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTexture, 1, 0);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTexture, -1, 0);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);

    // Test with layer -1 and layer == MAX_ARRAY_TEXTURE_LAYERS
    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTexture, 0, -1);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTexture, 0,
                              maxArrayTextureLayers);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);
}

// Attach layers of TEXTURE_2D_MULTISAMPLE_ARRAY textures to a framebuffer and check for
// completeness.
TEST_P(TextureMultisampleArrayWebGLTest, FramebufferCompleteness)
{
    ANGLE_SKIP_TEST_IF(!requestArrayExtension());

    std::vector<GLenum> testFormats = {{GL_RGBA8, GL_DEPTH_COMPONENT24, GL_DEPTH24_STENCIL8}};
    GLint samplesToUse = getSamplesToUse(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, testFormats);

    // Test framebuffer status with just a color texture attached.
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, mTexture);
    glTexStorage3DMultisampleANGLE(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, samplesToUse, GL_RGBA8, 4,
                                   4, 2, GL_TRUE);
    ASSERT_GL_NO_ERROR();

    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTexture, 0, 0);
    ASSERT_GL_NO_ERROR();

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    ASSERT_GLENUM_EQ(GL_FRAMEBUFFER_COMPLETE, status);

    // Test framebuffer status with both color and depth textures attached.
    GLTexture depthTexture;
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, depthTexture);
    glTexStorage3DMultisampleANGLE(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, samplesToUse,
                                   GL_DEPTH_COMPONENT24, 4, 4, 2, GL_TRUE);
    ASSERT_GL_NO_ERROR();

    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0, 0);
    ASSERT_GL_NO_ERROR();

    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    ASSERT_GLENUM_EQ(GL_FRAMEBUFFER_COMPLETE, status);

    // Test with color and depth/stencil textures attached.
    GLTexture depthStencilTexture;
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, depthStencilTexture);
    glTexStorage3DMultisampleANGLE(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, samplesToUse,
                                   GL_DEPTH24_STENCIL8, 4, 4, 2, GL_TRUE);
    ASSERT_GL_NO_ERROR();

    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, depthStencilTexture, 0,
                              0);
    ASSERT_GL_NO_ERROR();

    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    ASSERT_GLENUM_EQ(GL_FRAMEBUFFER_COMPLETE, status);
}

// Attach a layer of TEXTURE_2D_MULTISAMPLE_ARRAY texture to a framebuffer, clear it, and resolve by
// blitting.
TEST_P(TextureMultisampleArrayWebGLTest, FramebufferColorClearAndBlit)
{
    ANGLE_SKIP_TEST_IF(!requestArrayExtension());

    const GLsizei kWidth  = 4;
    const GLsizei kHeight = 4;

    std::vector<GLenum> testFormats = {GL_RGBA8};
    GLint samplesToUse = getSamplesToUse(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, testFormats);

    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, mTexture);
    glTexStorage3DMultisampleANGLE(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, samplesToUse, GL_RGBA8,
                                   kWidth, kHeight, 2, GL_TRUE);

    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTexture, 0, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    ASSERT_GL_NO_ERROR();
    ASSERT_GLENUM_EQ(GL_FRAMEBUFFER_COMPLETE, status);

    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    GLFramebuffer resolveFramebuffer;
    GLTexture resolveTexture;
    glBindTexture(GL_TEXTURE_2D, resolveTexture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, kWidth, kHeight);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFramebuffer);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resolveTexture,
                           0);
    glBlitFramebuffer(0, 0, kWidth, kHeight, 0, 0, kWidth, kHeight, GL_COLOR_BUFFER_BIT,
                      GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    glBindFramebuffer(GL_READ_FRAMEBUFFER, resolveFramebuffer);
    EXPECT_PIXEL_RECT_EQ(0, 0, kWidth, kHeight, GLColor::green);
}

ANGLE_INSTANTIATE_TEST(TextureMultisampleTest,
                       ES31_D3D11(),
                       ES3_OPENGL(),
                       ES3_OPENGLES(),
                       ES31_OPENGL(),
                       ES31_OPENGLES());
ANGLE_INSTANTIATE_TEST(TextureMultisampleTestES31, ES31_D3D11(), ES31_OPENGL(), ES31_OPENGLES());
ANGLE_INSTANTIATE_TEST(TextureMultisampleArrayWebGLTest,
                       ES31_D3D11(),
                       ES31_OPENGL(),
                       ES31_OPENGLES());

}  // anonymous namespace
