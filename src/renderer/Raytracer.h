#pragma once
#include <glm/vec3.hpp>

#include "Program.h"

struct Cascade
{
    unsigned int texture = 0;
    glm::ivec3 probes{0}, size{0};
    int tile = 4;
    float spacing = 0, tStart = 0, tEnd = 0;
};

struct RaytraceUniforms
{
    unsigned int 
        inverseProjection,
        inverseView,
        cascade0,
        gridOrigin,
        spacing0,
        probeCount0,
        tile0;
    void Query(Program& program)
    {
        inverseProjection = program.GetUniformLocation("inverseProjection");
        inverseView = program.GetUniformLocation("inverseView");
        cascade0 = program.GetUniformLocation("cascade0");
        gridOrigin = program.GetUniformLocation("gridOrigin");
        spacing0 = program.GetUniformLocation("spacing0");
        probeCount0 = program.GetUniformLocation("probeCount0");
        tile0 = program.GetUniformLocation("tile0");
    }
};

struct CascadeUniforms
{
    unsigned int
        gridOrigin,
        spacing,
        tile,
        //probeCount,
        tStart,
        tEnd,
        isTop;
    void Query(Program& program)
    {
        gridOrigin = program.GetUniformLocation("gridOrigin");
        spacing = program.GetUniformLocation("spacing");
        tile = program.GetUniformLocation("tile");
        //probeCount = program.GetUniformLocation("probeCount");
        tStart = program.GetUniformLocation("tStart");
        tEnd = program.GetUniformLocation("tEnd");
        isTop = program.GetUniformLocation("isTop");
    }
};

class Raytracer
{
public:
    static void BindRaytraceData(RaytraceUniforms& uniforms, unsigned int program);
    static void BindCascadeData(CascadeUniforms& uniforms, unsigned int program);
    
    static void UpdateCascades(
        Program& cascadeProgram,
        const CascadeUniforms& uniforms,
        const std::vector<Cascade>& cascades,
        const glm::vec3& gridOrigin
    );
};
