/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "dlaerror.h"
#include "dlatypes.h"

#include "nvdla/ILoadable.h"
#include "nvdla/IRuntime.h"
#include "nvdla/ITensor.h"
#include "nvdla/IWisdom.h"
#include "DlaImageUtils.h"

#include "ErrorMacros.h"

#include "nvdla_inf.h"

#include <string>

#define TEST_PARAM_FILE_MAX_SIZE    65536

enum TestImageTypes
{
    IMAGE_TYPE_PGM = 0,
    IMAGE_TYPE_JPG = 1,
    IMAGE_TYPE_UNKNOWN = 2,
};

struct TaskStatus
{
    NvU64 timestamp;
    NvU32 status_engine;
    NvU16 subframe;
    NvU16 status_task;
};

struct SubmitContext
{
    NvU32 id;
    std::string inputName;
    std::map < NvU16, std::vector<NvDlaImage*> > inputImages;
    std::vector<void *> inputBuffers;
    std::vector<void *> inputTaskStatus;
    std::map < NvU16, std::vector<NvDlaImage*> > outputImages;
    std::vector<NvDlaImage*> debugImages;
    std::vector<void *> debugBuffers;
    std::vector<void *> outputBuffers;
    std::vector<void *> outputTaskStatus;
};

struct CompilerTestAppArgs
{
    std::string project;
    std::string inputPath;
    std::string inputName;
    std::string outputPath;
    std::string testname;
    std::string testArgs;
    std::string prototxt; // This should be folded into testArgs
    std::string caffemodel; // This should be folded into testArgs
    std::string cachemodel; // This should be folded into testArgs

    std::string profileName; // ok here?
    std::string profileFile;
    std::string configtarget;
    std::string calibTable;
    nvdla::QuantizationMode quantizationMode;

    NvU16 numBatches;
    nvdla::DataFormat inDataFormat;
    nvdla::DataType computePrecision;

    std::map<std::string, NvF32> tensorScales;
};

struct CompilerTestInfo
{
    // common
    nvdla::IWisdom* wisdom;
    std::string wisdomPath;

    // parse
    std::string modelsPath;
    std::string profilesPath;
    std::string calibTablesPath;

    // runtime
    nvdla::IRuntime* runtime;
    nvdla::ILoadable* compiledLoadable;
    NvU8 *pData;
    std::string inputImagesPath;
    std::string inputLoadablePath;
    std::map<std::string, NvDlaImage*> inputImages;
    std::map<std::string, void *> inputBuffers;
    std::map<std::string, NvDlaImage*> outputImages;
    std::map<std::string, void *> outputBuffers;
    std::vector<SubmitContext*> submits;
    NvU32 timeout;
    NvU16 numBatches; // runtime's point-of-view
    NvU32 numSubmits;
};

// NvDlaError launchTest(const CompilerTestAppArgs* appArgs);
// NvDlaError testSetup(const CompilerTestAppArgs* appArgs, CompilerTestInfo* i);

// Tests
// NvDlaError generate(const CompilerTestAppArgs* appArgs, CompilerTestInfo* i);
// NvDlaError generateAndCompile(const CompilerTestAppArgs* appArgs, CompilerTestInfo* i);
// NvDlaError generateCompileAndRun(const CompilerTestAppArgs* appArgs, CompilerTestInfo* i);
// NvDlaError parse(const CompilerTestAppArgs* appArgs, CompilerTestInfo* i);
// NvDlaError parseAndCompile(const CompilerTestAppArgs* appArgs, CompilerTestInfo* i);
// NvDlaError parseCompileAndRun(const CompilerTestAppArgs* appArgs, CompilerTestInfo* i);
// NvDlaError run(const CompilerTestAppArgs* appArgs, CompilerTestInfo* i);

// Parse
// NvDlaError parseSetup(const CompilerTestAppArgs* appArgs, CompilerTestInfo* i);
// NvDlaError parseTensorScales(const CompilerTestAppArgs* appArgs, CompilerTestInfo *i, nvdla::INetwork* network);

// Generate
// NvDlaError generateProfile(const CompilerTestAppArgs* appArgs, std::string* profileName, CompilerTestInfo* i);
// NvDlaError generateTensorScales(const CompilerTestAppArgs* appArgs, CompilerTestInfo* i, nvdla::INetwork* network);

// Compile
// NvDlaError compileProfile(const CompilerTestAppArgs* appArgs, CompilerTestInfo* i, uint8_t *buffer, uint64_t *size);

// Runtime
// NvDlaError runSetup(const CompilerTestAppArgs* appArgs, CompilerTestInfo* i);
// NvDlaError runTeardown(const CompilerTestAppArgs* appArgs, CompilerTestInfo* i);
// NvDlaError readLoadable(const CompilerTestAppArgs* appArgs, CompilerTestInfo* i);
// NvDlaError loadLoadable(const CompilerTestAppArgs* appArgs, CompilerTestInfo* i);
// NvDlaError setupBuffers(const CompilerTestAppArgs* appArgs, CompilerTestInfo* i, SubmitContext* sc);
// NvDlaError releaseBuffers(const CompilerTestAppArgs* appArgs, CompilerTestInfo* i, SubmitContext* sc);
// NvDlaError runImages(const CompilerTestAppArgs* appArgs, CompilerTestInfo* i, SubmitContext* sc);

// TestUtils
// NvDlaError allocateDlaBuffer(void ** handle, void **pData, NvU32 size, NvDlaHeap heap);
// NvDlaError freeDlaBuffer(void * handle, void *pData);
// NvDlaError DIMG2DlaBuffer(const NvDlaImage* image, void ** phBuffer);
// NvDlaError DlaBuffer2DIMG(void * hBuffer, NvDlaImage* image);
// NvDlaError QINT8DIMG2FP16DIMG(const CompilerTestAppArgs* appArgs, const CompilerTestInfo* i,NvDlaImage* int8Dimg, NvDlaImage* fp16Dimg, NvF32 outSF);
