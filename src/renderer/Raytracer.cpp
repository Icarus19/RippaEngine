#include "Raytracer.h"

#include <glm/gtc/type_ptr.hpp>


void Raytracer::BindRaytraceData(RaytraceUniforms& uniforms, unsigned int program)
{
    
}

void Raytracer::BindCascadeData(CascadeUniforms& uniforms, unsigned int program)
{
    
}

void Raytracer::UpdateCascades(
    Program& cascadeProgram,
    const CascadeUniforms& uniforms,
    const std::vector<Cascade>& cascades,
    const glm::vec3& gridOrigin)
{
    cascadeProgram.Use();
    glUniform3fv(uniforms.gridOrigin, 1, glm::value_ptr(gridOrigin));
    
    for (size_t i = 0; i < cascades.size(); i++)
    {
        const Cascade& cascade = cascades[i];
        glUniform1f(uniforms.spacing, cascade.spacing);
        glUniform1i(uniforms.tile, cascade.tile);
        //glUniform3iv(uniforms.probeCount, 1, glm::value_ptr(cascade.probes));
        glUniform1f(uniforms.tStart, cascade.tStart);
        glUniform1f(uniforms.tEnd, cascade.tEnd);
        glUniform1i(uniforms.isTop, i == cascades.size() - 1);
        
        glBindImageTexture(0, cascade.texture, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        glDispatchCompute((cascade.size.x + 7) / 8, (cascade.size.y + 7) / 8, cascade.size.z);
    }
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
}
