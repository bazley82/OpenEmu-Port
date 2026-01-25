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

extension SPVType {
    @frozen
    public struct ArrayElementTypeInfoCollection {
        let type: __SPVType
        
        init(type: __SPVType) {
            assert(type.num_array_dimensions > 0)
            self.type = type
        }
        
        public var count: Int { Int(type.num_array_dimensions) }
        
        public subscript(index: Int) -> ArrayElementTypeInfo {
            precondition(index > 0 && index < count, "Index out of bounds")
            return ArrayElementTypeInfo(type: type, dimension: UInt32(index))
        }
    }
    
    @frozen
    public struct ArrayElementTypeInfo {
        let type: __SPVType
        let dimension: UInt32
        
        public var typeID: SPVTypeID { type.get_array_dimension(dimension: dimension) }
        
        /// Returns true if the array dimension is defined as a constant literal.
        ///
        /// Array elements can be either specialization constants or specialization ops.
        /// This array determines how to interpret the array size.
        /// If an element is true, the element is a literal,
        /// otherwise, it's an expression, which must be resolved on demand.
        /// The actual size is not really known until runtime.
        public var isDimensionLiteral: Bool { type.array_dimension_is_literal(dimension: dimension) }
    }
}
