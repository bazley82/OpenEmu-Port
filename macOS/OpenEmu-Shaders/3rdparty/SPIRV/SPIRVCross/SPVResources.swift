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

extension SPVResources {
    
    public var uniformBuffers: [SPVReflectedResource] { resources(for: .uniformBuffer) }
    public var storageBuffers: [SPVReflectedResource] { resources(for: .storageBuffer) }
    public var stageInputs: [SPVReflectedResource] { resources(for: .stageInput) }
    public var stageOutputs: [SPVReflectedResource] { resources(for: .stageOutput) }
    public var subpassInputs: [SPVReflectedResource] { resources(for: .subpassInput) }
    public var storageImages: [SPVReflectedResource] { resources(for: .storageImage) }
    public var sampledImages: [SPVReflectedResource] { resources(for: .sampledImage) }
    public var atomicCounters: [SPVReflectedResource] { resources(for: .atomicCounter) }
    public var pushConstants: [SPVReflectedResource] { resources(for: .pushConstant) }
    public var separateImage: [SPVReflectedResource] { resources(for: .separateImage) }
    public var separateSamplers: [SPVReflectedResource] { resources(for: .separateSamplers) }
    public var accelerationStructures: [SPVReflectedResource] { resources(for: .accelerationStructure) }
    public var rayQuery: [SPVReflectedResource] { resources(for: .rayQuery) }
    
    public func resources(for type: SPVResourceType) -> [SPVReflectedResource] {
        var ptr: UnsafePointer<__spvc_reflected_resource>?
        var size: Int = 0
        
        if get_resource_list_for_type(type: type, list: &ptr, size: &size).errorResult != nil {
            fatalError("Out of memory.")
        }
        
        return ptr!.withMemoryRebound(to: SPVReflectedResource.self, capacity: size) {
            Array(UnsafeBufferPointer<SPVReflectedResource>(start: $0, count: size))
        }
    }
}

@frozen
public struct SPVReflectedResource {
    let data: __spvc_reflected_resource
    
    public var id: SPVVariableID { data.id }
    public var baseTypeID: SPVTypeID { data.base_type_id }
    public var typeID: SPVTypeID { data.type_id }
    public var name: String { String(cString: data.name) }
}
