/* <editor-fold desc="MIT License">

Copyright(c) 2018 Robert Osfield

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include <vsg/traversals/DispatchVisitor.h>

#include <vsg/nodes/Commands.h>
#include <vsg/nodes/CullGroup.h>
#include <vsg/nodes/CullNode.h>
#include <vsg/nodes/Group.h>
#include <vsg/nodes/LOD.h>
#include <vsg/nodes/MatrixTransform.h>
#include <vsg/nodes/QuadGroup.h>
#include <vsg/nodes/StateGroup.h>

#include <vsg/vk/Command.h>
#include <vsg/vk/CommandBuffer.h>
#include <vsg/vk/RenderPass.h>

#include <vsg/maths/plane.h>

#include <iostream>

using namespace vsg;

//#define INLINE_TRAVERSE

DispatchVisitor::DispatchVisitor(CommandBuffer* commandBuffer) :
    _commandBuffer(commandBuffer)
{
    //std::cout << "DispatchVisitor::DispatchVisitor(" << commandBuffer << ")" << std::endl;

    _frustumUnit = Polytope{
        vsg::plane(1.0, 0.0, 0.0, 1.0),  // left plane
        vsg::plane(-1.0, 0.0, 0.0, 1.0), // right plane
        vsg::plane(0.0, 1.0, 0.0, 1.0),  // bottom plane
        vsg::plane(0.0, -1.0, 0.0, 1.0)  // top plane
    };

    _frustumDirty = true;
}

DispatchVisitor::~DispatchVisitor()
{
}

void DispatchVisitor::setProjectionMatrix(const dmat4& projMatrix)
{
    _state.projectionMatrixStack.set(projMatrix);
}

void DispatchVisitor::setViewMatrix(const dmat4& viewMatrix)
{
    _state.viewMatrixStack.set(viewMatrix);
}

void DispatchVisitor::apply(const Object& object)
{
    //    std::cout<<"Visiting object"<<std::endl;
    object.traverse(*this);
}

void DispatchVisitor::apply(const Group& group)
{
//    std::cout<<"Visiting Group "<<std::endl;
#ifdef INLINE_TRAVERSE
    vsg::Group::t_traverse(group, *this);
#else
    group.traverse(*this);
#endif
}

void DispatchVisitor::apply(const QuadGroup& group)
{
//    std::cout<<"Visiting QuadGroup "<<std::endl;
#ifdef INLINE_TRAVERSE
    vsg::QuadGroup::t_traverse(group, *this);
#else
    group.traverse(*this);
#endif
}

void DispatchVisitor::apply(const LOD& object)
{
    //    std::cout<<"Visiting LOD "<<std::endl;
    object.traverse(*this);
}

void DispatchVisitor::apply(const CullGroup& cullGroup)
{
#if 0
    // no culling
    cullGroup.traverse(*this);
#else
    if (intersect(cullGroup.getBound()))
    {
        //std::cout<<"Passed node"<<std::endl;
    #ifdef INLINE_TRAVERSE
        vsg::CullGroup::t_traverse(cullGroup, *this);
    #else
        cullGroup.traverse(*this);
    #endif
    }
    else
    {
        //std::cout<<"Culling node"<<std::endl;
    }
#endif
}


void DispatchVisitor::apply(const CullNode& cullNode)
{
#if 0
    // no culling
    cullGroup.traverse(*this);
#else
    if (intersect(cullNode.getBound()))
    {
#ifdef INLINE_TRAVERSE
        cullNode.getChild()->accept(*this);
#else
        //std::cout<<"Passed node"<<std::endl;
        cullNode.traverse(*this);
#endif
    }
    else
    {
        //std::cout<<"Culling node"<<std::endl;
    }
#endif
}

void DispatchVisitor::apply(const StateGroup& stateGroup)
{
    //    std::cout<<"Visiting StateGroup "<<std::endl;
    stateGroup.pushTo(_state);

#ifdef INLINE_TRAVERSE
    vsg::StateGroup::t_traverse(stateGroup, *this);
#else
    stateGroup.traverse(*this);
#endif

    stateGroup.popFrom(_state);
}

void DispatchVisitor::apply(const MatrixTransform& mt)
{
    _state.modelMatrixStack.push(mt.getMatrix());
    _state.dirty = true;

#ifdef INLINE_TRAVERSE
    vsg::MatrixTransform::t_traverse(mt, *this);
#else
    mt.traverse(*this);
#endif

    _state.modelMatrixStack.pop();
    _state.dirty = true;
}

// Vulkan nodes
void DispatchVisitor::apply(const Commands& commands)
{
    //    std::cout<<"Visiting Command "<<std::endl;
    _state.dispatch(*(_commandBuffer));
    for (auto& command : commands.getChildren())
    {
        command->dispatch(*(_commandBuffer));
    }
}

void DispatchVisitor::apply(const Command& command)
{
    //    std::cout<<"Visiting Command "<<std::endl;
    _state.dispatch(*(_commandBuffer));
    command.dispatch(*(_commandBuffer));
}
