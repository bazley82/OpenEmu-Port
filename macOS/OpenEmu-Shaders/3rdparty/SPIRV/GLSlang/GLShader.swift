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

var initialized: Bool = {
    glslang_initialize_process()
    return true
}()

public class GLShader {
    public enum ShaderError: Error {
        case preprocess, parse
    }
    
    let source: String
    var input: glslang_input_s
    let shader: CGLSLangShader
    
    public init(source: String, stage: GLStage,
                input: GLEnvironmentInput = GLEnvironmentInput(),
                client: GLEnvironmentClient = GLEnvironmentClient(),
                target: GLEnvironmentTarget = GLEnvironmentTarget()) {
        _ = initialized
        self.source = source
        self.input = source.withCString { str -> glslang_input_s in
            glslang_input_s(
                language: input.language,
                stage: stage,
                client: client.client,
                client_version: client.version,
                target_language: target.language,
                target_language_version: target.version,
                code: str,
                default_version: input.defaultVersion.rawValue,
                default_profile: input.defaultProfile,
                force_default_version_and_profile: input.forceDefaultVersionAndProfile ? 1 : 0,
                forward_compatible: input.forwardCompatible ? 1 : 0,
                messages: [],
                resource: glslang_get_default_resource(),
                includer_type: .forbid,
                callbacks: .init(),
                callbacks_ctx: nil)
        }
        shader = CGLSLangShader(input: &self.input)
    }
    
    public func parse(messages: GLMessageOptions = []) throws {
        input.messages = messages
        guard shader.preprocess(input: &input) else { throw ShaderError.preprocess }
        guard shader.parse(input: &input) else { throw ShaderError.parse }
    }
    
    public var infoLog: String {
        String(cString: shader.info_log)
    }
    
    public var debugLog: String {
        String(cString: shader.info_debug_log)
    }
    
    deinit {
        glslang_shader_delete(shader)
    }
}
