SwiftSPIRV-Cross
================

This package provides thoughtful bindings to [glslang][0], [SPIRV-Cross][1] and 
[SPIRV-Tools][2].

SwiftSPIRV-Cross' raison d'être is to convert Vulkan shaders into Metal shaders.

Three packages provide the Swift APIs for achieving this:

* GLSlang: The Swift bindings to glslang to parse .glsl files and generate
  SPIR-V.

* SPIRVTools: The Swift bindings to SPIRV-Tools, with support to
  use advanced optimization passes.
  
* SPIRVCross: The Swift bindings to SPIRV-Cross to generate Metal shader
  files.

## How does it work?

### Parse the glsl source

The first sequence of steps use the GLSlang package to
read, parse and compile a Vulkan shader and produce SPIRV output. 

1. Create a GLShader using Vulkan source
2. Parse the shader to validate it
3. Create a GLProgram and add the shader
4. Link the program
5. Generate SPIR-V from the program

This can be expensive, so if the files don't change,
it is worth caching the SPIR-V binary data.

### Optimize SPIR-V (optional)

The next step is optional and utilizes the SPIRVTools package to
optimize the SPIR-V.

1. Create an instance of `SPVTOptimizer`
2. Register any of the optimization passes, all of which are
   documented from their original C++ souce.
3. Call the `optimize` method to execute the registered optimization
   passes.

### Reflect and generate Metal Shader Language

These next steps use the SPIRVCross package to take a compiled program in
SPIRV and produce valid Metal Shader Language source:

1. Create an `SPVContext` to manage the SPIRVCross environment
2. Parse the SPIRV
3. Create a Metal compiler from the parsed SPIRV
4. Use the reflection APIs to determine required buffers, bindings, etc
5. Generate Metal source

The following example demonstrates converting a Vulkan vertext shader into
Metal Shader language

```swift
let vert = #"""
#version 450

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;
}
"""#

// load the shader
let shader = GLShader(source: vert, stage: .vertex)

// parse the shader according to rules 
try shader.parse(messages: [.vulkanRules, .spvRules])

let prog = GLProgram()
prog.add(shader: shader)
try prog.link()

let spirv = try prog.generate(stage: .vertex)

let ctx = SPVContext()
let ir  = try ctx.parse(data: spirv)

let mtl: SPVMetalCompiler = try ctx.makeCompiler(ir: ir)
let res = mtl.makeResources()

guard let r = res.uniformBuffers.first else { return }
guard let v = mtl.getVariable(resource: r) else { return }
guard let t = mtl.getType(id: r.baseTypeID) else { return }
```

Finally, call `try mtl.compile()` to produce the Metal Shader source:

```c++
#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct UBO
{
    float4x4 MVP;
};

struct main0_out
{
    float2 vTexCoord [[user(locn0)]];
    float4 gl_Position [[position]];
};

struct main0_in
{
    float4 Position [[attribute(0)]];
    float2 TexCoord [[attribute(1)]];
};

vertex main0_out main0(main0_in in [[stage_in]], constant UBO& global [[buffer(0)]])
{
    main0_out out = {};
    out.gl_Position = global.MVP * in.Position;
    out.vTexCoord = in.TexCoord;
    return out;
}
```

## Updating Dependencies

Most of the dependencies require a merge from their respective 
upstream repositories. Any additional steps are detailed in the
following sections.

### SPIRV-Tools

SPIRV-Tools requires additional steps to build the 
generated files.

There is a `Makefile` in the `3rdparty` directory which automates
most of the steps.

1. Clean `spirv-tools`

    ```sh
    make clean-spirv
    ```

2. Sync new headers

    ```sh
    make spirv-sync-headers
    ```

4. Generate additional files

    ```sh
    make spirv-update-ext
    ```

[0]: https://github.com/khronosGroup/glslang
[1]: https://github.com/khronosGroup/SPIRV-cross
[2]: https://github.com/khronosGroup/SPIRV-Tools