#ifndef WM_ITRAITSMANAGER_H
#define WM_ITRAITSMANAGER_H

#include <memory>

namespace tinyxml2 {
    class XMLElement;
}

class ITraitsCollection;
class ITraitSpec;

// Manages and loads the traits file
class ITraitsManager
{
public:
    virtual ~ITraitsManager() = default;

    virtual void load_xml(const tinyxml2::XMLElement& root) = 0;

    virtual std::unique_ptr<ITraitsCollection> create_collection() const = 0;
    virtual const ITraitSpec* lookup(const char* name) const = 0;
};

#endif //WM_ITRAITSMANAGER_H
