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

/// A type that provides access to SPVDecoration metadata
public protocol SPVDecorationContainer {
    /// A type representing the accessors elements
    associatedtype Element
    
    /// The set of valid keys for this accessor
    static var validKeys: Set<SPVDecoration> { get }
    
    /// Returns a boolean indicating whether the container contains a value for the given key
    /// - Parameter key: The key to find in the container.
    func hasValue(forKey key: SPVDecoration) -> Bool
    
    /// Returns the element for the specified key.
    /// - Parameter key: The key to find in the container.
    func value(forKey key: SPVDecoration) -> Element?
    
    /// Sets the value of the key to the specified value
    /// - Parameters:
    ///   - value: The new value to assign to the key.
    ///   - key: The key to be updated in the container.
    func set(value: Element, forKey key: SPVDecoration)
    
    /// Removes any previously assigned value to the specified key.
    /// - Parameter key: The key to be unset in the container.
    func unsetValue(forKey key: SPVDecoration)
}

@frozen
public struct DecorationCollection<T> where T: SPVDecorationContainer {
    let accessor: T
    
    init(accessor: T) {
        self.accessor = accessor
    }
    
    public var count: Int {
        T.validKeys.reduce(0) { $0 + (accessor.hasValue(forKey: $1) ? 1 : 0) }
    }
    
    public subscript(key: SPVDecoration) -> T.Element? {
        get {
            accessor.value(forKey: key)
        }
        
        mutating set {
            guard T.validKeys.contains(key) else { return }
            if let x = newValue {
                accessor.set(value: x, forKey: key)
            } else {
                removeValue(forKey: key)
            }
        }
    }
    
    @discardableResult
    public mutating func removeValue(forKey key: SPVDecoration) -> T.Element? {
        guard let old = self[key] else { return nil }
        accessor.unsetValue(forKey: key)
        return old
    }
}

extension DecorationCollection: Sequence {
    public typealias Element = (key: SPVDecoration, value: T.Element)
    public typealias Iterator = SPVDecorationAccessorIterator
    
    public struct SPVDecorationAccessorIterator: IteratorProtocol {
        public typealias Element = DecorationCollection.Element
        
        let accessor: T
        var i: Int
        let keys: [SPVDecoration]
        
        init(accessor: T) {
            self.accessor = accessor
            self.i = 0
            self.keys = T.validKeys.compactMap { accessor.hasValue(forKey: $0) ? $0 : nil }
        }
        
        public mutating func next() -> Element? {
            guard i < keys.count else { return nil }
            let key = keys[i]
            guard let val = accessor.value(forKey: key) else { return nil }
            i += 1
            return (key, val)
        }
    }
    
    public func makeIterator() -> Self.Iterator {
        SPVDecorationAccessorIterator(accessor: accessor)
    }
}
