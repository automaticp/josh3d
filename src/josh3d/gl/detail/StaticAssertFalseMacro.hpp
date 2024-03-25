#pragma once


#define JOSH3D_STATIC_ASSERT_FALSE(TemplateType) \
    static_assert(sizeof(TemplateType) == 0)

#define JOSH3D_STATIC_ASSERT_FALSE_MSG(TemplateType, Message) \
    static_assert(sizeof(TemplateType) == 0, Message)

