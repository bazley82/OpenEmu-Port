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
public struct SPVType {
    let compiler: SPVCompiler
    let type: __SPVType
    
    public var columns: UInt32 { type.columns }
    public var bitWidth: UInt32 { type.bit_width }
    public var vectorSize: UInt32 { type.vector_size }
    public var storageClass: SPVStorageClass { type.storage_class }
    
    public var baseTypeID: SPVTypeID { type.base_type_id }
    public var baseType: SPVBaseType { type.basetype }
    public var baseTypeName: String { String(cString: compiler.get_name(id: type.base_type_id)) }
    
    public var arrayType: ArrayElementTypeInfoCollection? {
        guard type.num_array_dimensions > 0 else { return nil }
        return ArrayElementTypeInfoCollection(type: type)
    }
    
    public var structType: StructTypeInfo? {
        guard type.num_member_types > 0 else { return nil}
        return StructTypeInfo(compiler: compiler, type: type)
    }
    
    public var imageType: ImageTypeInfo? {
        guard
            baseType == .image || baseType == .sampledImage
            else { return nil}
        return ImageTypeInfo(type: type)
    }
}
