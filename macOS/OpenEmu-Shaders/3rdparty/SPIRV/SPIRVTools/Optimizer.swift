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
import CSPIRVTools

public class SPVTOptimizer {
    let optimizer: CSPVTOptimizer
    
    public init(environment: SPVTargetEnvironment) {
        optimizer = CSPVTOptimizer(environment: environment)
    }
    
    deinit {
        optimizer.destroy()
    }
    
    public func optimize(spirv: Data) -> Data? {
        spirv.withUnsafeBytes { (a: UnsafeRawBufferPointer) -> Data? in
            let opt = CSPVTOptimizerOptions()
            opt.setRunValidator(false)
            
            // TODO: Expose this
            spvt_optimizer_set_consumer(optimizer) { (lvl, _, _, message) in
                guard let msg = message else { return }
                
                switch lvl {
                case .error:
                    print(String(cString: msg))
                default:
                    return
                }
            }
            defer {
                spvt_optimizer_clear_consumer(optimizer)
            }
            
            let spirv = a.bindMemory(to: UInt32.self)
            guard let vec = optimizer.run(original: spirv.baseAddress!, size: spirv.count, options: opt) else {
                return nil
            }

            return Data(bytesNoCopy: vec.ptr, count: vec.size, deallocator: .custom({ _, _ in
                vec.destroy()
            }))
        }
    }
}
