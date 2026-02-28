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

import CSPIRVTools

extension SPVTargetEnvironment: CustomStringConvertible {
    public var description: String {
        switch self {
        case .universal1_0:
            return "universal1_0"
        case .vulkan1_0:
            return "vulkan1_0"
        case .universal1_1:
            return "universal1_1"
        case .opencl2_1:
            return "opencl2_1"
        case .opencl2_2:
            return "opencl2_2"
        case .opengl4_0:
            return "opengl4_0"
        case .opengl4_1:
            return "opengl4_1"
        case .opengl4_2:
            return "opengl4_2"
        case .opengl4_3:
            return "opengl4_3"
        case .opengl4_5:
            return "opengl4_5"
        case .universal1_2:
            return "universal1_2"
        case .opencl1_2:
            return "opencl1_2"
        case .openclEmbedded1_2:
            return "openclEmbedded1_2"
        case .opencl2_0:
            return "opencl2_0"
        case .openclEmbedded2_0:
            return "openclEmbedded2_0"
        case .openclEmbedded2_1:
            return "openclEmbedded2_1"
        case .openclEmbedded2_2:
            return "openclEmbedded2_2"
        case .universal1_3:
            return "universal1_3"
        case .vulkan1_1:
            return "vulkan1_1"
        case .webgpu0:
            return "webgpu0"
        case .universal1_4:
            return "universal1_4"
        case .vulkan1_1Spirv1_4:
            return "vulkan1_1Spirv1_4"
        case .universal1_5:
            return "universal1_5"
        case .vulkan1_2:
            return "vulkan1_2"
        case .universal1_6:
            return "universal1_6"
        case .vulkan1_3:
            return "vulkan1_3"
        }
    }
}

extension SPVTargetEnvironment: CustomDebugStringConvertible {
    public var debugDescription: String { description }
}

extension SPVLevel: CustomStringConvertible {
    public var description: String {
        switch self {
        case .fatal:
            return "fatal"
        case .internalError:
            return "internalError"
        case .error:
            return "error"
        case .warning:
            return "warning"
        case .info:
            return "info"
        case .debug:
            return "debug"
        }
    }
}

extension SPVLevel: CustomDebugStringConvertible {
    public var debugDescription: String { description }
}
