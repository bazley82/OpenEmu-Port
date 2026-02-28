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
import CSPIRVCross

public final class SPVContext {
    let ctx: __SPVContext

    public init() {
        var ctx: __SPVContext?
        if __spvc_context_create(&ctx).errorResult != nil {
            fatalError("Out of memory")
        }
        self.ctx = ctx!
    }
    
    public func parse(data: Data) throws -> SPVParsedIR {
        var v: SPVParsedIR?
        let res = data.withUnsafeBytes { a -> SPVResult in
            let spirv = a.bindMemory(to: SpvId.self)
            return ctx.parse(data: spirv.baseAddress, data.count / MemoryLayout<SpvId>.size, &v)
        }
        
        if let res = res.errorResult {
            throw res
        }
        
        return v!
    }
    
    /// Create a Metal compiler backend
    /// - Parameters:
    ///   - ir: The IR used to generate the source.
    ///   - captureMode: Specifies if the compiler takes ownership of the passed IR.
    /// - Throws: SPVResult.Error
    /// - Returns: A Metal compiler
    public func makeMetalCompiler(ir: SPVParsedIR, captureMode: SPVCaptureMode = .takeOwnership) throws -> SPVMetalCompiler {
        var c: SPVCompiler?
        if let res = ctx.create_compiler(backend: .msl, ir: ir, captureMode: captureMode, compiler: &c).errorResult {
            throw res
        }
        return SPVMetalCompiler(compiler: c!)
    }
    
    deinit {
        // TODO: This is not safe if there are references to child objects
        print("destroying SPVContext")
        ctx.destroy()
    }
}
