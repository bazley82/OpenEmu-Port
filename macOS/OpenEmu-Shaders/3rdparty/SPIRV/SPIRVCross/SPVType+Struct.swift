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

extension SPVType {
    @frozen
    public struct StructTypeInfo {
        let compiler: SPVCompiler
        let type: __SPVType
        
        /// Returns the size of the struct in bytes.
        public var size: Int {
            guard type.num_member_types > 0 else { return 0 }
            var size: Int = 0
            if let res = compiler.get_declared_struct_size(type: type, size: &size).errorResult {
                fatalError("Unexpected error \(res.rawValue)")
            }
            return size
        }
        
        public var members: MemberTypeInfoCollection {
            precondition(type.num_member_types > 0)
            return MemberTypeInfoCollection(compiler: compiler, type: type)
        }
    }
}

extension SPVType {
    @frozen
    public struct MemberTypeInfoCollection {
        let compiler: SPVCompiler
        let type: __SPVType
        
        public var count: UInt32 { type.num_member_types }
    }
    
    @frozen
    public struct MemberTypeInfo {
        let compiler: SPVCompiler
        let type: __SPVType
        let index: UInt32
        
        public var name: String {
            return String(cString: compiler.get_member_name(id: type.base_type_id, index: index))
        }
        
        public var typeID: SPVTypeID { type.get_member_type(index: index) }
        
        public var stringDecorations: DecorationCollection<MemberDecorationStringContainer> {
            DecorationCollection(accessor: MemberDecorationStringContainer(compiler: compiler, typeID: type.base_type_id, memberIndex: index))
        }
        public var decorations: DecorationCollection<MemberDecorationUInt32Container> {
            DecorationCollection(accessor: MemberDecorationUInt32Container(compiler: compiler, typeID: type.base_type_id, memberIndex: index))
        }
    }
}

extension SPVType.MemberTypeInfoCollection: Collection {
    public typealias Index = UInt32
    public typealias Element = SPVType.MemberTypeInfo
    
    // The upper and lower bounds of the collection, used in iterations
    public var startIndex: Index { return 0 }
    public var endIndex: Index { return count }
    
    // Required subscript, based on a dictionary index
    public subscript(index: Index) -> Iterator.Element {
        precondition(index >= 0 && index < count, "Index out of bounds")
        
        return SPVType.MemberTypeInfo(compiler: compiler, type: type, index: UInt32(index))
    }
    
    // Method that returns the next index when iterating
    public func index(after i: Index) -> Index {
        return i+1
    }
}

// MARK: - Accessors

extension SPVType {
    @frozen
    public struct MemberDecorationStringContainer: SPVDecorationContainer {
        public typealias Element = String
        // source: ParsedIR::get_member_decoration_string
        public static let validKeys: Set<SPVDecoration> = [.hlslSemanticGOOGLE]
        
        let compiler: SPVCompiler
        let typeID: SPVTypeID
        let memberIndex: UInt32
        
        public func hasValue(forKey key: SPVDecoration) -> Bool {
            compiler.has_member_decoration(id: typeID, index: memberIndex, decoration: key)
        }
        
        public func value(forKey key: SPVDecoration) -> Element? {
            guard hasValue(forKey: key) else { return nil }
            return String(cString: compiler.get_member_decoration_string(id: typeID, index: memberIndex, decoration: key))
        }
        
        public func set(value: Element, forKey key: SPVDecoration) {
            value.withCString { ptr in
                compiler.set_member_decoration_string(id: typeID, index: memberIndex, decoration: key, value: ptr)
            }
        }
        
        public func unsetValue(forKey key: SPVDecoration) {
            compiler.unset_member_decoration(id: typeID, index: memberIndex, decoration: key)
        }
    }
    
    @frozen
    public struct MemberDecorationUInt32Container: SPVDecorationContainer {
        public typealias Element = UInt32
        // source: ParsedIR::get_member_decoration
        public static let validKeys: Set<SPVDecoration> = [
            .builtIn, .location, .component, .binding,
            .offset, .xfbBuffer, .xfbStride, .specId, .index,
        ]
        
        let compiler: SPVCompiler
        let typeID: SPVTypeID
        let memberIndex: UInt32
        
        public func hasValue(forKey key: SPVDecoration) -> Bool {
            compiler.has_member_decoration(id: typeID, index: memberIndex, decoration: key)
        }
        
        public func value(forKey key: SPVDecoration) -> Element? {
            guard hasValue(forKey: key) else { return nil }
            return compiler.get_member_decoration(id: typeID, index: memberIndex, decoration: key)
        }
        
        public func set(value: Element, forKey key: SPVDecoration) {
            compiler.set_member_decoration(id: typeID, index: memberIndex, decoration: key, value: value)
        }
        
        public func unsetValue(forKey key: SPVDecoration) {
            compiler.unset_member_decoration(id: typeID, index: memberIndex, decoration: key)
        }
    }
}
