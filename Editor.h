#ifndef __CCIMEDITOR_EDITOR_H__
#define __CCIMEDITOR_EDITOR_H__

#include "cocos2d.h"

#include "Widget.h"

namespace CCImEditor
{
    class Editor: public cocos2d::Layer
    {
    public:
        static Editor* getInstance();

        Ref* getUserObject(const std::string& path) { return _userObjects[path]; };
        void setUserObject(const std::string& path, Ref* handle) { _userObjects[path] = handle; };

        void onEnter() override;
        void onExit() override;
        
        void addWidget(Widget* widget);
        Widget* getWidget(const std::string& typeName) const;
        
        cocos2d::Node* getEditingNode() const { return _editingNode; };

        void setDebugMode(bool isDebugMode) { _isDebugMode = isDebugMode; }
        bool isDebugMode() const { return _isDebugMode; };

        void registerWidgets();
        void registerNodes();
        
        void visit(cocos2d::Renderer *renderer, const cocos2d::Mat4 &parentTransform, uint32_t parentFlags);

    CC_CONSTRUCTOR_ACCESS: 
        Editor();
        ~Editor();

    private:
        Editor(const Editor&) = delete;
        void operator=(const Editor&) = delete;

        bool init() override;
        void update(float) override;
        void callback();

        std::unordered_map<std::string, cocos2d::WeakPtr<cocos2d::Ref>> _userObjects;
        std::vector<cocos2d::RefPtr<Widget>> _widgets;
        cocos2d::WeakPtr<cocos2d::Node> _editingNode;

        bool _isDebugMode = true;
        cocos2d::CustomCommand _command;
    };
}

#endif