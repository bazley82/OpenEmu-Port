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
import Foundation

extension SPVResult {
    enum ErrorResult: Int, LocalizedError {
        case invalidSpirv = -1
        case unsupportedSpirv = -2
        case outOfMemory = -3
        case invalidArgument = -4
        case unknownError = 0xffff
        
        init?(_ res: SPVResult) {
            switch res {
            case .invalidSpirv:
                self = .invalidSpirv
            case .unsupportedSpirv:
                self = .unsupportedSpirv
            case .outOfMemory:
                self = .outOfMemory
            case .invalidArgument:
                self = .invalidArgument
            case .success:
                return nil
            default:
                self = .unknownError
            }
        }
    }
    
    var errorResult: ErrorResult? { return ErrorResult(self) }
}
