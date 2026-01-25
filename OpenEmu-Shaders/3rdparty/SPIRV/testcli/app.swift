//
//  main.swift
//  testcli
//
//  Created by Stuart Carnie on 18/1/21.
//

import Foundation
import SPIRV

@main
struct Test {
    static func main() throws {
        let stage = GLStage.vertex
        let code  = #"""
         #version 450
         #extension GL_GOOGLE_include_directive : enable

         #define static

         #define FOO 10.3

         layout(push_constant) uniform Push
         {
            float phosphor_power;
         } params;

         layout(location = 0) out vec4 FragColor;

         static const float should_go_away = 3.0 / 4.0;

         void main()
         {
             int a = 3;
             a += 5;
             if (false)
                 FragColor = vec4(1, 2, 3, 4);
             a = 6;
             FragColor = vec4(1, FOO, params.phosphor_power, a);
         }
        """#
        let vertShader = GLShader(source: code, stage: stage)
        
        do {
            try vertShader.parse(messages: [.vulkanRules, .spvRules])
        } catch GLShader.ShaderError.preprocess {
            print("Preprocess error")
            print(vertShader.infoLog)
            print(vertShader.debugLog)
            exit(1)
        } catch GLShader.ShaderError.parse {
            print("Parse error")
            print(vertShader.infoLog)
            print(vertShader.debugLog)
            exit(1)
        }
        
        let prog = GLProgram()
        prog.add(shader: vertShader)
        let vertSpv: Data
        do {
            try prog.link()
            let vertSpv1 = try prog.generate(stage: stage)
            let opt = SPVTOptimizer(environment: .universal1_5)
            opt.registerConditionalConstantPropagationPass()
                .registerDeadBranchElimPass()
                .registerAggressiveDeadCodeEliminationPass()
                .registerStaticSingleAssignmentRewritePass()
                .registerAggressiveDeadCodeEliminationPass()

            // vertSpv = opt.optimize(spirv: vertSpv1)!
            vertSpv = vertSpv1
        } catch GLProgram.ProgramError.link {
            print("Link error")
            print(prog.infoLog)
            exit(1)
        } catch GLProgram.ProgramError.noStage {
            print("No stage")
            exit(1)
        }
        
        let ctx = SPVContext()
        
        do {
            let vertMtl = try ctx.makeMetalCompiler(ir: try ctx.parse(data: vertSpv))
            
            var opt = vertMtl.makeOptions()
            let res = vertMtl.makeResources()
            let ubo = res.pushConstants

            let tID  = vertMtl.getType(id: ubo[0].typeID)
            let btID = vertMtl.getType(id: ubo[0].baseTypeID)
            
            let vb = vertMtl.getVariable(resource: ubo[0])
            
            opt.swizzleTextureSamples = false
            opt.forceNativeArrays = false
            opt.version = .version2_1
            let src = try vertMtl.compile(options: opt)
            print(src)
        }
    }
}
