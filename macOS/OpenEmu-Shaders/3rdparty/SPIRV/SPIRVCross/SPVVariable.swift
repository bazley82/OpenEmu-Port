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

@frozen
public struct SPVVariable {
    let compiler: SPVCompiler
    public let id: SPVVariableID
    public let baseTypeID: SPVTypeID
    
    public var name: String { String(cString: compiler.get_name(id: id)) }
    
    public var stringDecorations: DecorationCollection<CompilerDecorationStringContainer> {
        DecorationCollection(accessor: CompilerDecorationStringContainer(compiler: compiler, id: id))
    }
    
    public var decorations: DecorationCollection<CompilerDecorationUInt32Container> {
        DecorationCollection(accessor: CompilerDecorationUInt32Container(compiler: compiler, id: id))
    }
    
    /// Returns an array containing the members of the struct type associated with
    /// the variable that are potentially in use by the SPIR-V shader.
    ///
    /// This can be used for Buffer (UBO), BufferBlock/StorageBuffer (SSBO) and PushConstant blocks.
    public var activeBufferRanges: [SPVBufferRange] {
        guard let type = compiler.get_type_handle(baseTypeID), type.num_member_types > 0 else {
            return []
        }
        
        var ptr: UnsafePointer<spvc_buffer_range>?
        var size: Int = 0

        if compiler.get_active_buffer_ranges(id: id, list: &ptr, size: &size).errorResult != nil {
            fatalError("Out of memory.")
        }
        
        return ptr!.withMemoryRebound(to: SPVBufferRange.self, capacity: size) {
            Array(UnsafeBufferPointer<SPVBufferRange>(start: $0, count: size))
        }
    }
}

@frozen
public struct SPVBufferRange {
    let data: spvc_buffer_range
    
    /// The index of the struct member.
    public var index: UInt32 { data.index }
    
    /// The offset of this struct member into the buffer.
    public var offset: Int { data.offset }
    
    /// The number of bytes occupied by this struct member
    /// in the buffer.
    public var range: Int { data.range }
}
