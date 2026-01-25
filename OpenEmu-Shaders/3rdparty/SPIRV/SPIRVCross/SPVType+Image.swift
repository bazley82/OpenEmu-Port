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
    public struct ImageTypeInfo {
        let type: __SPVType
        
        public var sampledType: SPVTypeID { type.image_sampled_type }
        public var dimension: SPVDim { type.image_dimension }
        public var isDepth: Bool { type.image_is_depth }
        public var isArray: Bool { type.image_arrayed }
        public var isMultisample: Bool { type.image_multisampled }
        public var isStorage: Bool { type.image_is_storage }
        public var storageFormat: SPVImageFormat { type.image_storage_format }
        public var accessQualifier: SPVAccessQualifier { type.image_access_qualifier }
    }
}
