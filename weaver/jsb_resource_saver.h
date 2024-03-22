#ifndef JAVASCRIPT_RESOURCE_SAVER_H
#define JAVASCRIPT_RESOURCE_SAVER_H

#include "core/io/resource_saver.h"

class ResourceFormatSaverJavaScript : public ResourceFormatSaver
{
public:
    virtual Error save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags = 0) override;
    virtual void get_recognized_extensions(const Ref<Resource> &p_resource, List<String> *p_extensions) const override;
    virtual bool recognize(const Ref<Resource> &p_resource) const override;

};

#endif
