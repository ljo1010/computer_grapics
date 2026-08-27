#pragma once

#include "RenderContext.h"

// ============================================================================
// IRenderPass: 렌더 패스 추상 인터페이스 (Render Pass Strategy Pattern)
// ============================================================================
class IRenderPass
{
public:
    virtual ~IRenderPass() = default;

    virtual const char* GetName() const = 0;
    virtual void Execute(const RenderContext& ctx) = 0;
};