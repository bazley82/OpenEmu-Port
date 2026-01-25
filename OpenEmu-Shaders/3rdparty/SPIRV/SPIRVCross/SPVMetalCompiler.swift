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
import Metal

@frozen
public struct SPVMetalCompiler {
    let compiler: SPVCompiler
    
    public var isRasterizationDisabled: Bool { compiler.isMslRasterizationDisabled }
    public var needsSwizzleBuffer: Bool { compiler.mslNeedsSwizzleBuffer }
    public var needsBufferSizeBuffer: Bool { compiler.mslNeedsBufferSizeBuffer }
    public var needsOutputBuffer: Bool { compiler.mslNeedsOutputBuffer }
    public var needsPatchOutputBuffer: Bool { compiler.mslNeedsPatchOutputBuffer }
    public var needsInputThreadgroupMem: Bool { compiler.mslNeedsInputThreadgroupMem }
    
    public func getAutomaticResourceBinding(resource: SPVReflectedResource) -> UInt32 {
        compiler.mslGetAutomaticResourceBinding(resource.id)
    }
    
    public func getSecondaryAutomaticResourceBinding(resource: SPVReflectedResource) -> UInt32 {
        compiler.mslGetSecondaryAutomaticResourceBinding(resource.id)
    }
    
    public var entryPoints: [SPVEntryPoint] {
        var ptr: UnsafePointer<__spvc_entry_point>?
        var size: Int = 0
        
        if compiler.get_entry_points(list: &ptr, size: &size).errorResult != nil {
            fatalError("Out of memory.")
        }
        
        return ptr!.withMemoryRebound(to: SPVEntryPoint.self, capacity: size) {
            Array(UnsafeBufferPointer<SPVEntryPoint>(start: $0, count: size))
        }
    }
    
    public func makeOptions() -> Self.Options {
        var options: SPVCompilerOptions?
        if compiler.create_compiler_options(&options).errorResult != nil {
            fatalError("Out of memory")
        }
        return Self.Options(options: options!)
    }

    public func makeResources() -> SPVResources {
        var resources: SPVResources?
        if compiler.create_shader_resources(&resources).errorResult != nil {
            fatalError("Out of memory")
        }
        
        return resources!
    }
    
    public func compile() throws -> String {
        var src: UnsafePointer<Int8>?
        if let res = compiler.compile(&src).errorResult {
            throw res
        }
        return String(cString: src!)
    }
    
    public func compile(options: Self.Options) throws -> String {
        options.apply(to: compiler)
        return try compile()
    }
    
    public func getType(id: SPVTypeID) -> SPVType? {
        guard let type = compiler.get_type_handle(id) else { return nil }
        return SPVType(compiler: compiler, type: type)
    }
    
    public func getVariable(resource: SPVReflectedResource) -> SPVVariable? {
        SPVVariable(compiler: compiler, id: resource.id, baseTypeID: resource.baseTypeID)
    }
    
    public func name(id: SPVTypeID) -> String {
        String(cString: compiler.get_name(id: id))
    }
}

extension SPVMetalCompiler {
    public enum Platform {
        case iOS, macOS
        
        var spvcPlatform: UInt32 {
            (self == .macOS ? SPVC_MSL_PLATFORM_MACOS : SPVC_MSL_PLATFORM_IOS).rawValue
        }
    }
    
    public enum IndexType {
        case none, uint16, uint32
        
        var spvcIndexType: UInt32 {
            switch self {
            case .none:
                return SPVC_MSL_INDEX_TYPE_NONE.rawValue
            case .uint16:
                return SPVC_MSL_INDEX_TYPE_UINT16.rawValue
            case .uint32:
                return SPVC_MSL_INDEX_TYPE_UINT32.rawValue
            }
        }
    }
    
    @frozen
    public struct Options {
        let options: SPVCompilerOptions
        
        init(options: SPVCompilerOptions) {
            self.options  = options
        }
        
        // MARK: Common Options
        
        /// Debug option to always emit temporary variables for all expressions.
        public var forceTemporary: Bool = false

        /// Flattens multidimensional arrays, e.g. `float foo[a][b][c]` into single-dimensional arrays,
        /// e.g. `float foo[a * b * c]`.
        /// This function does not change the actual `SPIRType` of any object.
        /// Only the generated code, including declarations of interface variables are changed to be single array dimension.
        public var flattenMultidimensionalArrays: Bool = false

        /// In vertex shaders, rewrite `[-w, w]` depth (GL style) to `[0, w]` depth.
        public var fixupDepthConvention: Bool = false

        /// Inverts `gl_Position.y` or equivalent.
        public var flipVertexY: Bool = false

        /// Emit OpLine directives if present in the module.
        /// May not correspond exactly to original source, but should be a good approximation.
        public var emitLineDirectives: Bool = false

        /// In cases where readonly/write-only decoration are not used at all,
        /// we try to deduce which qualifier(s) we should actually used, since actually emitting
        /// read-write decoration is very rare, and older glslang/HLSL compilers tend to just emit readwrite as a matter of fact.
        /// The default (true) is to enable automatic deduction for these cases, but if you trust the decorations set
        /// by the SPIR-V, it's recommended to set this to false.
        public var enableStorageImageQualifierDeduction: Bool = true

        // MARK: Metal Shader Language Options

        public var platform: Platform = .macOS
        public var version: MTLLanguageVersion = .version1_2
        public var texelBufferTextureWidth: UInt32 = 4096
        public var r32UILinearTextureAlignment: UInt32 = 4
        public var r32UIAlignmentConstantID: UInt32 = 65535
        public var swizzleBufferIndex: UInt32 = 30
        public var indirectParamsBufferIndex: UInt32 = 29
        public var shaderOutputBufferIndex: UInt32 = 28
        public var shaderPatchOutputBufferIndex: UInt32 = 27
        public var shaderTessFactorOutputBufferIndex: UInt32 = 26
        public var bufferSizeBufferIndex: UInt32 = 25
        public var viewMaskBufferIndex: UInt32 = 24
        public var dynamicOffsetsBufferIndex: UInt32 = 23
        public var shaderInputBufferIndex: UInt32 = 22
        public var shaderIndexBufferIndex: UInt32 = 21
        public var shaderInputWorkgroupIndex: UInt32 = 0
        public var deviceIndex: UInt32 = 0
        public var enableFragOutputMask: UInt32 = 0xffff_ffff
        public var vertexIndexType: IndexType = .none

        public var enablePointSizeBuiltin: Bool = true
        public var enableFragDepthBuiltin: Bool = true
        public var enableFragStencilRefBuiltin: Bool = true
        public var disableRasterization: Bool = false
        public var captureOutputToBuffer: Bool = false
        public var swizzleTextureSamples: Bool = false
        public var tessDomainOriginLowerLeft: Bool = false
        public var multiview: Bool = false
        public var multiviewLayeredRendering: Bool = false
        public var viewIndexFromDeviceIndex: Bool = false
        public var dispatchBase: Bool = false
        public var texture1DAs2D: Bool = false

        /// Enable use of MSL 2.0 indirect argument buffers.
        /// MSL 2.0 must also be enabled.
        public var argumentBuffers: Bool = false

        /// Ensures vertex and instance indices start at zero.
        /// This reflects the behavior of HLSL with `SV_VertexID` and `SV_InstanceID`.
        public var enableBaseIndexZero: Bool = false

        /// Fragment output in MSL must have at least as many components as the render pass.
        /// Add support to explicit pad out components.
        public var padFragmentOutputComponents: Bool = false

        /// Use Metal's native frame-buffer fetch API for subpass inputs.
        public var useFramebufferFetchSubpasses: Bool = false

        /// Enables use of "fma" intrinsic for invariant float math
        public var invariantFloatMath: Bool = false

        /// Emulate `texturecube_array` with `texture2d_array` for iOS where this type is not available
        public var emulateCubemapArray: Bool = false

        // Allow user to enable decoration binding
        public  var enableDecorationBinding: Bool = false

        /// Requires MSL 2.1, use the native support for texel buffers.
        public var textureBufferNative: Bool = false

        /// Forces all resources which are part of an argument buffer to be considered active.
        /// This ensures ABI compatibility between shaders where some resources might be unused,
        /// and would otherwise declare a different IAB.
        public var forceActiveArgumentBufferResources: Bool = false

        /// Forces the use of plain arrays, which works around certain driver bugs on certain versions
        /// of Intel Macbooks. See https://github.com/KhronosGroup/SPIRV-Cross/issues/1210.
        /// May reduce performance in scenarios where arrays are copied around as value-types.
        public var forceNativeArrays: Bool = false

        /// If a shader writes clip distance, also emit user varyings which
        /// can be read in subsequent stages.
        public var enableClipDistanceUserVarying: Bool = false
        
        /// In a tessellation control shader, assume that more than one patch can be processed in a
        /// single workgroup. This requires changes to the way the InvocationId and PrimitiveId
        /// builtins are processed, but should result in more efficient usage of the GPU.
        public var multiPatchWorkgroup: Bool = false
        
        /// If set, a vertex shader will be compiled as part of a tessellation pipeline.
        /// It will be translated as a compute kernel, so it can use the global invocation ID
        /// to index the output buffer.
        public var vertexForTessellation: Bool = false
        
        /// Assume that SubpassData images have multiple layers. Layered input attachments
        /// are addressed relative to the Layer output from the vertex pipeline. This option
        /// has no effect with multiview, since all input attachments are assumed to be layered
        /// and will be addressed using the current ViewIndex.
        public var arrayedSubpassInput: Bool = false
        
        /// Whether to use SIMD-group or quadgroup functions to implement group nnon-uniform
        /// operations. Some GPUs on iOS do not support the SIMD-group functions, only the
        /// quadgroup functions.
        public var iosUseSimdgroupFunctions: Bool = false
        
        /// If set, the subgroup size will be assumed to be one, and subgroup-related
        /// builtins and operations will be emitted accordingly. This mode is intended to
        /// be used by MoltenVK on hardware/software configurations which do not provide
        /// sufficient support for subgroups.
        public var emulateSubgroups: Bool = false
        
        /// If nonzero, a fixed subgroup size to assume. Metal, similarly to `VK_EXT_subgroup_size_control`,
        /// allows the SIMD-group size (aka thread execution width) to vary depending on
        /// register usage and requirements. In certain circumstances--for example, a pipeline
        /// in MoltenVK without `VK_PIPELINE_SHADER_STAGE_CREATE_ALLOW_VARYING_SUBGROUP_SIZE_BIT_EXT`--
        /// this is undesirable. This fixes the value of the SubgroupSize builtin, instead of
        /// mapping it to the Metal builtin `[[thread_execution_width]]`. If the thread
        /// execution width is reduced, the extra invocations will appear to be inactive.
        /// If zero, the SubgroupSize will be allowed to vary, and the builtin will be mapped
        /// to the Metal `[[thread_execution_width]]` builtin.
        public var fixedSubgroupSize: UInt32 = 0
        
        /// If set, a dummy `[[sample_id]]` input is added to a fragment shader if none is present.
        /// This will force the shader to run at sample rate, assuming Metal does not optimize
        /// the extra threads away.
        public var forceSampleRateShading: Bool = false
        
        // MARK: Private
        
        static let commonBools: [(spvc_compiler_option, KeyPath<Self, Bool>)] = [
            (SPVC_COMPILER_OPTION_FORCE_TEMPORARY, \.forceTemporary),
            (SPVC_COMPILER_OPTION_FLATTEN_MULTIDIMENSIONAL_ARRAYS, \.flattenMultidimensionalArrays),
            (SPVC_COMPILER_OPTION_FIXUP_DEPTH_CONVENTION, \.fixupDepthConvention),
            (SPVC_COMPILER_OPTION_FLIP_VERTEX_Y, \.flipVertexY),
            (SPVC_COMPILER_OPTION_EMIT_LINE_DIRECTIVES, \.emitLineDirectives),
            (SPVC_COMPILER_OPTION_ENABLE_STORAGE_IMAGE_QUALIFIER_DEDUCTION, \.enableStorageImageQualifierDeduction),
        ]
        
        static let mslBools: [(spvc_compiler_option, KeyPath<Self, Bool>)] = [
            (SPVC_COMPILER_OPTION_MSL_ENABLE_POINT_SIZE_BUILTIN, \.enablePointSizeBuiltin),
            (SPVC_COMPILER_OPTION_MSL_ENABLE_FRAG_DEPTH_BUILTIN, \.enableFragDepthBuiltin),
            (SPVC_COMPILER_OPTION_MSL_ENABLE_FRAG_STENCIL_REF_BUILTIN, \.enableFragStencilRefBuiltin),
            (SPVC_COMPILER_OPTION_MSL_DISABLE_RASTERIZATION, \.disableRasterization),
            (SPVC_COMPILER_OPTION_MSL_CAPTURE_OUTPUT_TO_BUFFER, \.captureOutputToBuffer),
            (SPVC_COMPILER_OPTION_MSL_SWIZZLE_TEXTURE_SAMPLES, \.swizzleTextureSamples),
            (SPVC_COMPILER_OPTION_MSL_TESS_DOMAIN_ORIGIN_LOWER_LEFT, \.tessDomainOriginLowerLeft),
            (SPVC_COMPILER_OPTION_MSL_MULTIVIEW, \.multiview),
            (SPVC_COMPILER_OPTION_MSL_MULTIVIEW_LAYERED_RENDERING, \.multiviewLayeredRendering),
            (SPVC_COMPILER_OPTION_MSL_VIEW_INDEX_FROM_DEVICE_INDEX, \.viewIndexFromDeviceIndex),
            (SPVC_COMPILER_OPTION_MSL_DISPATCH_BASE, \.dispatchBase),
            (SPVC_COMPILER_OPTION_MSL_TEXTURE_1D_AS_2D, \.texture1DAs2D),
            (SPVC_COMPILER_OPTION_MSL_ARGUMENT_BUFFERS, \.argumentBuffers),
            (SPVC_COMPILER_OPTION_MSL_ENABLE_BASE_INDEX_ZERO, \.enableBaseIndexZero),
            (SPVC_COMPILER_OPTION_MSL_PAD_FRAGMENT_OUTPUT_COMPONENTS, \.padFragmentOutputComponents),
            (SPVC_COMPILER_OPTION_MSL_FRAMEBUFFER_FETCH_SUBPASS, \.useFramebufferFetchSubpasses),
            (SPVC_COMPILER_OPTION_MSL_INVARIANT_FP_MATH, \.invariantFloatMath),
            (SPVC_COMPILER_OPTION_MSL_EMULATE_CUBEMAP_ARRAY, \.emulateCubemapArray),
            (SPVC_COMPILER_OPTION_MSL_ENABLE_DECORATION_BINDING, \.enableDecorationBinding),
            (SPVC_COMPILER_OPTION_MSL_TEXTURE_BUFFER_NATIVE, \.textureBufferNative),
            (SPVC_COMPILER_OPTION_MSL_FORCE_ACTIVE_ARGUMENT_BUFFER_RESOURCES, \.forceActiveArgumentBufferResources),
            (SPVC_COMPILER_OPTION_MSL_FORCE_NATIVE_ARRAYS, \.forceNativeArrays),
            (SPVC_COMPILER_OPTION_MSL_ENABLE_CLIP_DISTANCE_USER_VARYING, \.enableClipDistanceUserVarying),
            (SPVC_COMPILER_OPTION_MSL_MULTI_PATCH_WORKGROUP, \.multiPatchWorkgroup),
            (SPVC_COMPILER_OPTION_MSL_VERTEX_FOR_TESSELLATION, \.vertexForTessellation),
            (SPVC_COMPILER_OPTION_MSL_ARRAYED_SUBPASS_INPUT, \.arrayedSubpassInput),
            (SPVC_COMPILER_OPTION_MSL_IOS_USE_SIMDGROUP_FUNCTIONS, \.iosUseSimdgroupFunctions),
            (SPVC_COMPILER_OPTION_MSL_EMULATE_SUBGROUPS, \.emulateSubgroups),
            (SPVC_COMPILER_OPTION_MSL_FORCE_SAMPLE_RATE_SHADING, \.forceSampleRateShading),
        ]
        
        static let mslUInt32s: [(spvc_compiler_option, KeyPath<Self, UInt32>)] = [
            (SPVC_COMPILER_OPTION_MSL_TEXEL_BUFFER_TEXTURE_WIDTH, \.texelBufferTextureWidth),
            (SPVC_COMPILER_OPTION_MSL_R32UI_LINEAR_TEXTURE_ALIGNMENT, \.r32UILinearTextureAlignment),
            (SPVC_COMPILER_OPTION_MSL_R32UI_ALIGNMENT_CONSTANT_ID, \.r32UIAlignmentConstantID),
            (SPVC_COMPILER_OPTION_MSL_SWIZZLE_BUFFER_INDEX, \.swizzleBufferIndex),
            (SPVC_COMPILER_OPTION_MSL_INDIRECT_PARAMS_BUFFER_INDEX, \.indirectParamsBufferIndex),
            (SPVC_COMPILER_OPTION_MSL_SHADER_OUTPUT_BUFFER_INDEX, \.shaderOutputBufferIndex),
            (SPVC_COMPILER_OPTION_MSL_SHADER_PATCH_OUTPUT_BUFFER_INDEX, \.shaderPatchOutputBufferIndex),
            (SPVC_COMPILER_OPTION_MSL_SHADER_TESS_FACTOR_OUTPUT_BUFFER_INDEX, \.shaderTessFactorOutputBufferIndex),
            (SPVC_COMPILER_OPTION_MSL_BUFFER_SIZE_BUFFER_INDEX, \.bufferSizeBufferIndex),
            (SPVC_COMPILER_OPTION_MSL_VIEW_MASK_BUFFER_INDEX, \.viewMaskBufferIndex),
            (SPVC_COMPILER_OPTION_MSL_DYNAMIC_OFFSETS_BUFFER_INDEX, \.dynamicOffsetsBufferIndex),
            (SPVC_COMPILER_OPTION_MSL_SHADER_INPUT_BUFFER_INDEX, \.shaderInputBufferIndex),
            (SPVC_COMPILER_OPTION_MSL_SHADER_INDEX_BUFFER_INDEX, \.shaderIndexBufferIndex),
            (SPVC_COMPILER_OPTION_MSL_SHADER_INPUT_WORKGROUP_INDEX, \.shaderInputWorkgroupIndex),
            (SPVC_COMPILER_OPTION_MSL_DEVICE_INDEX, \.deviceIndex),
            (SPVC_COMPILER_OPTION_MSL_ENABLE_FRAG_OUTPUT_MASK, \.enableFragOutputMask),
            (SPVC_COMPILER_OPTION_MSL_FIXED_SUBGROUP_SIZE, \.fixedSubgroupSize),
        ]
        
        func apply(to compiler: SPVCompiler) {
            for (option, kp) in Self.commonBools {
                options.set_bool(option: option, with: self[keyPath: kp].spvcBoolValue)
            }

            options.set_uint(option: SPVC_COMPILER_OPTION_MSL_PLATFORM, with: platform.spvcPlatform)
            options.set_uint(option: SPVC_COMPILER_OPTION_MSL_VERSION, with: version.spvcMetalVersion)
            options.set_uint(option: SPVC_COMPILER_OPTION_MSL_VERTEX_INDEX_TYPE, with: vertexIndexType.spvcIndexType)
            
            for (option, kp) in Self.mslBools {
                options.set_bool(option: option, with: self[keyPath: kp].spvcBoolValue)
            }
            
            for (option, kp) in Self.mslUInt32s {
                options.set_uint(option: option, with: self[keyPath: kp])
            }
            
            compiler.install_compiler_options(options: options)
        }
    }
}

extension MTLLanguageVersion {
    func makeVersion(major: Int, minor: Int, patch: Int = 0) -> UInt32 {
        UInt32(major * 10000 + minor * 100 + patch)
    }
    
    @usableFromInline
    var spvcMetalVersion: UInt32 {
        switch self {
        case .version1_0:
            return makeVersion(major: 1, minor: 0)
        case .version1_1:
            return makeVersion(major: 1, minor: 1)
        case .version1_2:
            return makeVersion(major: 1, minor: 2)
        case .version2_0:
            return makeVersion(major: 2, minor: 0)
        case .version2_1:
            return makeVersion(major: 2, minor: 1)
        case .version2_2:
            return makeVersion(major: 2, minor: 2)
        case .version2_3:
            return makeVersion(major: 2, minor: 3)
        case .version2_4:
            return makeVersion(major: 2, minor: 4)
#if swift(>=5.6)
        case .version3_0:
            return makeVersion(major: 3, minor: 0)
#endif
        @unknown default:
            fatalError()
        }
    }
}
