// Copyright (c) 2020 Stuart Carnie
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

import CSPIRVCross

public typealias SPVExecutionModel = CSPIRVCross.SPVExecutionModel

extension SPVExecutionModel: CustomStringConvertible {
    public var description: String {
        switch self {
        case .vertex:
            return "vertex"
        case .tessellationControl:
            return "tessellationControl"
        case .tessellationEvaluation:
            return "tessellationEvaluation"
        case .geometry:
            return "geometry"
        case .fragment:
            return "fragment"
        case .glCompute:
            return "glCompute"
        case .kernel:
            return "kernel"
        case .taskNV:
            return "taskNV"
        case .meshNV:
            return "meshNV"
        case .rayGenerationKHR:
            return "rayGenerationKHR"
        case .intersectionKHR:
            return "intersectionKHR"
        case .anyHitKHR:
            return "anyHitKHR"
        case .closestHitKHR:
            return "closestHitKHR"
        case .missKHR:
            return "missKHR"
        case .callableKHR:
            return "callableKHR"
        case .taskEXT:
            return "taskEXT"
        case .meshEXT:
            return "meshEXT"
        }
    }
}

extension SPVExecutionModel: CustomDebugStringConvertible {
    public var debugDescription: String { description }
}

public typealias SPVBaseType = CSPIRVCross.SPVBaseType

extension SPVBaseType: CustomStringConvertible {
    public var description: String {
        switch self {
        case .unknown:
            return "unknown"
        case .void:
            return "void"
        case .boolean:
            return "boolean"
        case .int8:
            return "int8"
        case .uint8:
            return "uint8"
        case .int16:
            return "int16"
        case .uint16:
            return "uint16"
        case .int32:
            return "int32"
        case .uint32:
            return "uint32"
        case .int64:
            return "int64"
        case .uint64:
            return "uint64"
        case .atomicCounter:
            return "atomicCounter"
        case .fp16:
            return "fp16"
        case .fp32:
            return "fp32"
        case .fp64:
            return "fp64"
        case .struct:
            return "struct"
        case .image:
            return "image"
        case .sampledImage:
            return "sampledImage"
        case .sampler:
            return "sampler"
        case .accelerationStructure:
            return "accelerationStructure"
        }
    }
}

extension SPVBaseType: CustomDebugStringConvertible {
    public var debugDescription: String { description }
}

extension SPVBackend: CustomStringConvertible {
    public var description: String {
        switch self {
        case .none:
            return "none"
        case .glsl:
            return "glsl"
        case .hlsl:
            return "hlsl"
        case .msl:
            return "msl"
        case .cpp:
            return "cpp"
        case .json:
            return "json"
        }
    }
}

public typealias SPVDecoration = CSPIRVCross.SPVDecoration

extension SPVDecoration: CustomStringConvertible {
    public var description: String {
        switch self {
        case .relaxedPrecision:
            return "relaxedPrecision"
        case .specId:
            return "specId"
        case .block:
            return "block"
        case .bufferBlock:
            return "bufferBlock"
        case .rowMajor:
            return "rowMajor"
        case .colMajor:
            return "colMajor"
        case .arrayStride:
            return "arrayStride"
        case .matrixStride:
            return "matrixStride"
        case .glslShared:
            return "glslShared"
        case .glslPacked:
            return "glslPacked"
        case .cPacked:
            return "cPacked"
        case .builtIn:
            return "builtIn"
        case .noPerspective:
            return "noPerspective"
        case .flat:
            return "flat"
        case .patch:
            return "patch"
        case .centroid:
            return "centroid"
        case .sample:
            return "sample"
        case .invariant:
            return "invariant"
        case .restrict:
            return "restrict"
        case .aliased:
            return "aliased"
        case .volatile:
            return "volatile"
        case .constant:
            return "constant"
        case .coherent:
            return "coherent"
        case .nonWritable:
            return "nonWritable"
        case .nonReadable:
            return "nonReadable"
        case .uniform:
            return "uniform"
        case .uniformId:
            return "uniformId"
        case .saturatedConversion:
            return "saturatedConversion"
        case .stream:
            return "stream"
        case .location:
            return "location"
        case .component:
            return "component"
        case .index:
            return "index"
        case .binding:
            return "binding"
        case .descriptorSet:
            return "descriptorSet"
        case .offset:
            return "offset"
        case .xfbBuffer:
            return "xfbBuffer"
        case .xfbStride:
            return "xfbStride"
        case .funcParamAttr:
            return "funcParamAttr"
        case .floatingPointRoundingMode:
            return "floatingPointRoundingMode"
        case .floatingPointFastMathMode:
            return "floatingPointFastMathMode"
        case .linkageAttributes:
            return "linkageAttributes"
        case .noContraction:
            return "noContraction"
        case .inputAttachmentIndex:
            return "inputAttachmentIndex"
        case .alignment:
            return "alignment"
        case .maxByteOffset:
            return "maxByteOffset"
        case .alignmentId:
            return "alignmentId"
        case .maxByteOffsetId:
            return "maxByteOffsetId"
        case .noSignedWrap:
            return "noSignedWrap"
        case .noUnsignedWrap:
            return "noUnsignedWrap"
        case .explicitInterpAMD:
            return "explicitInterpAMD"
        case .overrideCoverageNV:
            return "overrideCoverageNV"
        case .passthroughNV:
            return "passthroughNV"
        case .viewportRelativeNV:
            return "viewportRelativeNV"
        case .secondaryViewportRelativeNV:
            return "secondaryViewportRelativeNV"
        case .perPrimitiveEXT:
            return "perPrimitiveEXT"
        case .perPrimitiveNV:
            return "perPrimitiveNV"
        case .perViewNV:
            return "perViewNV"
        case .perTaskNV:
            return "perTaskNV"
        case .perVertexKHR:
            return "perVertexKHR"
        case .perVertexNV:
            return "perVertexNV"
        case .nonUniform:
            return "nonUniform"
        case .nonUniformEXT:
            return "nonUniformEXT"
        case .restrictPointer:
            return "restrictPointer"
        case .restrictPointerEXT:
            return "restrictPointerEXT"
        case .aliasedPointer:
            return "aliasedPointer"
        case .aliasedPointerEXT:
            return "aliasedPointerEXT"
        case .bindlessSamplerNV:
            return "bindlessSamplerNV"
        case .bindlessImageNV:
            return "bindlessImageNV"
        case .boundSamplerNV:
            return "boundSamplerNV"
        case .boundImageNV:
            return "boundImageNV"
        case .sIMTCallINTEL:
            return "sIMTCallINTEL"
        case .referencedIndirectlyINTEL:
            return "referencedIndirectlyINTEL"
        case .clobberINTEL:
            return "clobberINTEL"
        case .sideEffectsINTEL:
            return "sideEffectsINTEL"
        case .vectorComputeVariableINTEL:
            return "vectorComputeVariableINTEL"
        case .funcParamIOKindINTEL:
            return "funcParamIOKindINTEL"
        case .vectorComputeFunctionINTEL:
            return "vectorComputeFunctionINTEL"
        case .stackCallINTEL:
            return "stackCallINTEL"
        case .globalVariableOffsetINTEL:
            return "globalVariableOffsetINTEL"
        case .counterBuffer:
            return "counterBuffer"
        case .hlslCounterBufferGOOGLE:
            return "hlslCounterBufferGOOGLE"
        case .hlslSemanticGOOGLE:
            return "hlslSemanticGOOGLE"
        case .userSemantic:
            return "userSemantic"
        case .userTypeGOOGLE:
            return "userTypeGOOGLE"
        case .functionRoundingModeINTEL:
            return "functionRoundingModeINTEL"
        case .functionDenormModeINTEL:
            return "functionDenormModeINTEL"
        case .registerINTEL:
            return "registerINTEL"
        case .memoryINTEL:
            return "memoryINTEL"
        case .numbanksINTEL:
            return "numbanksINTEL"
        case .bankwidthINTEL:
            return "bankwidthINTEL"
        case .maxPrivateCopiesINTEL:
            return "maxPrivateCopiesINTEL"
        case .singlepumpINTEL:
            return "singlepumpINTEL"
        case .doublepumpINTEL:
            return "doublepumpINTEL"
        case .maxReplicatesINTEL:
            return "maxReplicatesINTEL"
        case .simpleDualPortINTEL:
            return "simpleDualPortINTEL"
        case .mergeINTEL:
            return "mergeINTEL"
        case .bankBitsINTEL:
            return "bankBitsINTEL"
        case .forcePow2DepthINTEL:
            return "forcePow2DepthINTEL"
        case .burstCoalesceINTEL:
            return "burstCoalesceINTEL"
        case .cacheSizeINTEL:
            return "cacheSizeINTEL"
        case .dontStaticallyCoalesceINTEL:
            return "dontStaticallyCoalesceINTEL"
        case .prefetchINTEL:
            return "prefetchINTEL"
        case .stallEnableINTEL:
            return "stallEnableINTEL"
        case .fuseLoopsInFunctionINTEL:
            return "fuseLoopsInFunctionINTEL"
        case .aliasScopeINTEL:
            return "aliasScopeINTEL"
        case .noAliasINTEL:
            return "noAliasINTEL"
        case .bufferLocationINTEL:
            return "bufferLocationINTEL"
        case .iOPipeStorageINTEL:
            return "iOPipeStorageINTEL"
        case .functionFloatingPointModeINTEL:
            return "functionFloatingPointModeINTEL"
        case .singleElementVectorINTEL:
            return "singleElementVectorINTEL"
        case .vectorComputeCallableFunctionINTEL:
            return "vectorComputeCallableFunctionINTEL"
        case .mediaBlockIOINTEL:
            return "mediaBlockIOINTEL"
        }
    }
}

extension SPVDecoration: CustomDebugStringConvertible {
    public var debugDescription: String { description }
}

public typealias SPVResourceType = CSPIRVCross.SPVResourceType

extension SPVResourceType: CustomStringConvertible {
    public var description: String {
        switch self {
        case .unknown:
            return "unknown"
        case .uniformBuffer:
            return "uniformBuffer"
        case .storageBuffer:
            return "storageBuffer"
        case .stageInput:
            return "stageInput"
        case .stageOutput:
            return "stageOutput"
        case .subpassInput:
            return "subpassInput"
        case .storageImage:
            return "storageImage"
        case .sampledImage:
            return "sampledImage"
        case .atomicCounter:
            return "atomicCounter"
        case .pushConstant:
            return "pushConstant"
        case .separateImage:
            return "separateImage"
        case .separateSamplers:
            return "separateSamplers"
        case .accelerationStructure:
            return "accelerationStructure"
        case .rayQuery:
            return "rayQuery"
        case .shaderRecordBuffer:
            return "shaderRecordBuffer"
        }
    }
}

extension SPVResourceType: CustomDebugStringConvertible {
    public var debugDescription: String { description }
}

public typealias SPVStorageClass = CSPIRVCross.SPVStorageClass

extension SPVStorageClass: CustomStringConvertible {
    public var description: String {
        switch self {
        case .uniformConstant:
            return "uniformConstant"
        case .input:
            return "input"
        case .uniform:
            return "uniform"
        case .output:
            return "output"
        case .workgroup:
            return "workgroup"
        case .crossWorkgroup:
            return "crossWorkgroup"
        case .private:
            return "private"
        case .function:
            return "function"
        case .generic:
            return "generic"
        case .pushConstant:
            return "pushConstant"
        case .atomicCounter:
            return "atomicCounter"
        case .image:
            return "image"
        case .storageBuffer:
            return "storageBuffer"
        case .callableDataKHR:
            return "callableDataKHR"
        case .callableDataNV:
            return "callableDataNV"
        case .incomingCallableDataKHR:
            return "incomingCallableDataKHR"
        case .incomingCallableDataNV:
            return "incomingCallableDataNV"
        case .rayPayloadKHR:
            return "rayPayloadKHR"
        case .rayPayloadNV:
            return "rayPayloadNV"
        case .hitAttributeKHR:
            return "hitAttributeKHR"
        case .hitAttributeNV:
            return "hitAttributeNV"
        case .incomingRayPayloadKHR:
            return "incomingRayPayloadKHR"
        case .incomingRayPayloadNV:
            return "incomingRayPayloadNV"
        case .shaderRecordBufferKHR:
            return "shaderRecordBufferKHR"
        case .shaderRecordBufferNV:
            return "shaderRecordBufferNV"
        case .physicalStorageBuffer:
            return "physicalStorageBuffer"
        case .physicalStorageBufferEXT:
            return "physicalStorageBufferEXT"
        case .taskPayloadWorkgroupEXT:
            return "taskPayloadWorkgroupEXT"
        case .codeSectionINTEL:
            return "codeSectionINTEL"
        case .deviceOnlyINTEL:
            return "deviceOnlyINTEL"
        case .hostOnlyINTEL:
            return "hostOnlyINTEL"
        }
    }
}

extension SPVStorageClass: CustomDebugStringConvertible {
    public var debugDescription: String { description }
}

public typealias SPVDim = CSPIRVCross.SPVDim

extension SPVDim: CustomStringConvertible {
    public var description: String {
        switch self {
        case .dim1D:
            return "dim1D"
        case .dim2D:
            return "dim2D"
        case .dim3D:
            return "dim3D"
        case .cube:
            return "cube"
        case .rect:
            return "rect"
        case .buffer:
            return "buffer"
        case .subpassData:
            return "subpassData"
        }
    }
}

extension SPVDim: CustomDebugStringConvertible {
    public var debugDescription: String { description }
}

public typealias SPVImageFormat = CSPIRVCross.SPVImageFormat

extension SPVImageFormat: CustomStringConvertible {
    public var description: String {
        switch self {
        case .unknown:
            return "unknown"
        case .rgba32f:
            return "rgba32f"
        case .rgba16f:
            return "rgba16f"
        case .r32f:
            return "r32f"
        case .rgba8:
            return "rgba8"
        case .rgba8Snorm:
            return "rgba8Snorm"
        case .rg32f:
            return "rg32f"
        case .rg16f:
            return "rg16f"
        case .r11fG11fB10f:
            return "r11fG11fB10f"
        case .r16f:
            return "r16f"
        case .rgba16:
            return "rgba16"
        case .rgb10A2:
            return "rgb10A2"
        case .rg16:
            return "rg16"
        case .rg8:
            return "rg8"
        case .r16:
            return "r16"
        case .r8:
            return "r8"
        case .rgba16Snorm:
            return "rgba16Snorm"
        case .rg16Snorm:
            return "rg16Snorm"
        case .rg8Snorm:
            return "rg8Snorm"
        case .r16Snorm:
            return "r16Snorm"
        case .r8Snorm:
            return "r8Snorm"
        case .rgba32i:
            return "rgba32i"
        case .rgba16i:
            return "rgba16i"
        case .rgba8i:
            return "rgba8i"
        case .r32i:
            return "r32i"
        case .rg32i:
            return "rg32i"
        case .rg16i:
            return "rg16i"
        case .rg8i:
            return "rg8i"
        case .r16i:
            return "r16i"
        case .r8i:
            return "r8i"
        case .rgba32ui:
            return "rgba32ui"
        case .rgba16ui:
            return "rgba16ui"
        case .rgba8ui:
            return "rgba8ui"
        case .r32ui:
            return "r32ui"
        case .rgb10a2ui:
            return "rgb10a2ui"
        case .rg32ui:
            return "rg32ui"
        case .rg16ui:
            return "rg16ui"
        case .rg8ui:
            return "rg8ui"
        case .r16ui:
            return "r16ui"
        case .r8ui:
            return "r8ui"
        case .r64ui:
            return "r64ui"
        case .r64i:
            return "r64i"
        }
    }
}

extension SPVImageFormat: CustomDebugStringConvertible {
    public var debugDescription: String { description }
}

public typealias SPVAccessQualifier = CSPIRVCross.SPVAccessQualifier

extension SPVAccessQualifier: CustomStringConvertible {
    public var description: String {
        switch self {
        case .readOnly:
            return "readOnly"
        case .writeOnly:
            return "writeOnly"
        case .readWrite:
            return "readWrite"
        }
    }
}

extension SPVAccessQualifier: CustomDebugStringConvertible {
    public var debugDescription: String { description }
}
