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

import Foundation
import CGLSLang

public struct GLEnvironmentInput {
    public enum DefaultVersion: Int32 {
        case es = 100, desktop = 110
    }
    
    public let language: GLSource
    public let defaultVersion: DefaultVersion
    public let defaultProfile: GLProfile
    public let forceDefaultVersionAndProfile: Bool
    /// Return errors for use of deprecated features.
    public let forwardCompatible: Bool
    
    public init(language: GLSource = .glsl, defaultVersion: DefaultVersion = .desktop,
                defaultProfile: GLProfile = .noProfile, forceDefaultVersionAndProfile: Bool = false,
                forwardCompatible: Bool = false) {
        self.language = language
        self.defaultVersion = defaultVersion
        self.defaultProfile = defaultProfile
        self.forceDefaultVersionAndProfile = forceDefaultVersionAndProfile
        self.forwardCompatible = forwardCompatible
    }
}

public struct GLEnvironmentClient {
    public let client: GLClient
    public let version: GLTargetClientVersion
    
    public init(_ client: GLClient = .vulkan, version: GLTargetClientVersion = .vulkan1_1) {
        self.client = client
        self.version = version
    }
}

public struct GLEnvironmentTarget {
    public let language: GLTargetLanguage
    public let version: GLTargetLanguageVersion
    
    public init(language: GLTargetLanguage = .spv, version: GLTargetLanguageVersion = .spv1_5) {
        self.language = language
        self.version = version
    }
}

extension glslang_input_s {
    init(stage: GLStage, input: GLEnvironmentInput, client: GLEnvironmentClient, target: GLEnvironmentTarget, code: UnsafePointer<Int8>? = nil, messages: GLMessageOptions = []) {
        self.init(
            language: input.language,
            stage: stage,
            client: client.client,
            client_version: client.version,
            target_language: target.language,
            target_language_version: target.version,
            code: code,
            default_version: input.defaultVersion.rawValue,
            default_profile: input.defaultProfile,
            force_default_version_and_profile: input.forceDefaultVersionAndProfile ? 1 : 0,
            forward_compatible: input.forwardCompatible ? 1 : 0,
            messages: messages,
            resource: glslang_get_default_resource(),
            includer_type: .forbid,
            callbacks: .init(),
            callbacks_ctx: nil)
    }
}
