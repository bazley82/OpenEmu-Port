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

public class GLProgram {
    public enum ProgramError: Error {
        case link, noStage
    }
    
    let program = CGLSLangProgram()
    
    public init() {}
    
    deinit {
        glslang_program_delete(program)
    }
    
    public func add(shader: GLShader) {
        program.add_shader(shader.shader)
    }
    
    public func link(messages: GLMessageOptions = [.vulkanRules, .spvRules]) throws {
        guard program.link(messages: messages) else { throw ProgramError.link }
    }
    
    public var infoLog: String { String(cString: program.info_log) }
    public var debugLog: String { String(cString: program.info_debug_log) }
    
    public func generate(stage: GLStage) throws -> Data {
        program.spirv_generate(stage: stage)
        let size = program.spirv_size * MemoryLayout<UInt32>.size
        return program.spirv_pointer.withMemoryRebound(to: UInt8.self, capacity: size) { bytes in
            Data(bytes: bytes, count: size)
        }
    }
}

public struct GLCompiledProgram {
    
}
