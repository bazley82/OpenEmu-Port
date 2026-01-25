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

import CGLSLang

public typealias GLStage = CGLSLang.GLStage

extension GLStage: CustomStringConvertible {
    public var description: String {
        switch self {
        case .vertex:
            return "vertex"
        case .tessControl:
            return "tessControl"
        case .tessEvaluation:
            return "tessEvaluation"
        case .geometry:
            return "geometry"
        case .fragment:
            return "fragment"
        case .compute:
            return "compute"
        case .rayGen:
            return "rayGen"
        case .intersect:
            return "intersect"
        case .anyHit:
            return "anyHit"
        case .closestHit:
            return "closestHit"
        case .miss:
            return "miss"
        case .callable:
            return "callable"
        case .task:
            return "taskNV"
        case .mesh:
            return "meshNV"
        }
    }
}

extension GLStage: CustomDebugStringConvertible {
    public var debugDescription: String { description }
}

public typealias GLStageOptions = CGLSLang.GLStageOptions

extension GLStageOptions: CaseIterable {
    public static var allCases: [Self] = [
        .vertex, .tessControl, .tessEvaluation, .geometry,
        .fragment, .compute, .rayGen, .intersect, .anyHit, .closestHit,
        .miss, .callable, .task, .mesh,
    ]
}

public typealias GLSource = CGLSLang.GLSource

extension GLSource: CustomStringConvertible {
    public var description: String {
        switch self {
        case .none:
            return "none"
        case .glsl:
            return "glsl"
        case .hlsl:
            return "hlsl"
        }
    }
}

public typealias GLClient = CGLSLang.GLClient

extension GLClient: CustomStringConvertible {
    public var description: String {
        switch self {
        case .none:
            return "none"
        case .vulkan:
            return "vulkan"
        case .opengl:
            return "opengl"
        }
    }
}

public typealias GLTargetLanguage = CGLSLang.GLTargetLanguage

extension GLTargetLanguage: CustomStringConvertible {
    public var description: String {
        switch self {
        case .none:
            return "none"
        case .spv:
            return "spv"
        }
    }
}

public typealias GLTargetClientVersion = CGLSLang.GLTargetClientVersion

extension GLTargetClientVersion: CustomStringConvertible {
    public var description: String {
        switch self {
        case .vulkan1_0:
            return "vulkan1_0"
        case .vulkan1_1:
            return "vulkan1_1"
        case .vulkan1_2:
            return "vulkan1_2"
        case .vulkan1_3:
            return "vulkan1_3"
        case .opengl450:
            return "opengl450"
        }
    }
}

public typealias GLTargetLanguageVersion = CGLSLang.GLTargetLanguageVersion

extension GLTargetLanguageVersion: CustomStringConvertible {
    public var description: String {
        switch self {
        case .spv1_0:
            return "spv1_0"
        case .spv1_1:
            return "spv1_1"
        case .spv1_2:
            return "spv1_2"
        case .spv1_3:
            return "spv1_3"
        case .spv1_4:
            return "spv1_4"
        case .spv1_5:
            return "spv1_5"
        case .spv1_6:
            return "spv1_6"
        }
    }
}

public typealias GLExecutable = CGLSLang.GLExecutable

extension GLExecutable: CustomStringConvertible {
    public var description: String {
        switch self {
        case .vertexFragment:
            return "vertexFragment"
        case .fragment:
            return "fragment"
        }
    }
}

public typealias GLOptimizationLevel = CGLSLang.GLOptimizationLevel

extension GLOptimizationLevel: CustomStringConvertible {
    public var description: String {
        switch self {
        case .noGeneration:
            return "noGeneration"
        case .none:
            return "none"
        case .simple:
            return "simple"
        case .full:
            return "full"
        }
    }
}

public typealias GLTextureSamplerTransformMode = CGLSLang.GLTextureSamplerTransformMode

extension GLTextureSamplerTransformMode: CustomStringConvertible {
    public var description: String {
        switch self {
        case .keep:
            return "keep"
        case .upgradeTextureRemoveSampler:
            return "upgradeTextureRemoveSampler"
        }
    }
}

public typealias GLMessageOptions = CGLSLang.GLMessageOptions

extension GLMessageOptions: CaseIterable {
    public static var allCases: [Self] = [
        .default, .relaxedErrors, .suppressWarnings, .ast, .spvRules,
        .vulkanRules, .onlyPreprocessor, .readHlsl, .cascadingErrors,
        .keepUncalled, .hlslOffsets, .debugInfo, .hlslEnable16BitTypes,
        .hlslLegalization, .hlslDx9Compatible, .builtinSymbolTable,
    ]
}

public typealias GLReflectionOptions = CGLSLang.GLReflectionOptions

extension GLReflectionOptions: CaseIterable {
    public static var allCases: [Self] = [
        .default, .strictArraySuffix, .basicArraySuffix, .intermediateIOO,
        .separateBuffers, .allBlockVariables, .unwrapIOBlocks, .allIOVariables,
        .sharedStd140ssbo, .sharedStd140ubo,
    ]
}

public typealias GLProfile = CGLSLang.GLProfile

extension GLProfile: CustomStringConvertible {
    public var description: String {
        switch self {
        case .badProfile:
            return "badProfile"
        case .noProfile:
            return "noProfile"
        case .coreProfile:
            return "coreProfile"
        case .compatibilityProfile:
            return "compatibilityProfile"
        case .esProfile:
            return "esProfile"
        }
    }
}

public typealias GLIncluderType = CGLSLang.GLIncluderType

extension GLIncluderType: CustomStringConvertible {
    public var description: String {
        switch self {
        case .forbid:
            return "forbid"
        case .dirStack:
            return "dirStack"
        case .custom:
            return "custom"
        }
    }
}
