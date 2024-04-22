#ifndef __CCIMEDITOR_PROPERTYIMDRAWER_H__
#define __CCIMEDITOR_PROPERTYIMDRAWER_H__

#include <string>
#include "Widget.h"
#include "imgui/imgui.h"
#include "Editor.h"

namespace CCImEditor
{
    template <typename T, typename Enabled = void>
    struct PropertyImDrawer
    {
        static bool draw(const char* label, ...)
        {
            CCLOGWARN("Can't draw property: %s, missing specialization", label);
            return false;
        }

        static bool serialize(cocos2d::ValueMap& target, const char* label, ...)
        {
            CCLOGWARN("Can't serialize property: %s, missing specialization", label);
            return false;
        }

        static bool deserialize(const cocos2d::ValueMap& source, const char* label, ...)
        {
            CCLOGWARN("Can't deserialize property: %s, missing specialization", label);
            return false;
        }
    };

    template <>
    struct PropertyImDrawer<bool> {
        static bool draw(const char* label, bool& v) {
            return ImGui::Checkbox(label, &v);
        }

        static bool serialize(cocos2d::ValueMap& target, const char* label, bool v) {
            target.emplace(label, v);
            return true;
        }

        static bool deserialize(const cocos2d::ValueMap& source, const char* label, bool& v) {
            cocos2d::ValueMap::const_iterator it = source.find(label);
            if (it != source.end())
            {
                v = it->second.asBool();
                return true;
            }
            
            return false;
        }
    };
    
    template <>
    struct PropertyImDrawer<cocos2d::Vec3> {
        static bool draw(const char* label, cocos2d::Vec3& vec, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", ImGuiSliderFlags flags = 0) {
            float v[3] = { vec.x, vec.y, vec.z };
            if (ImGui::DragFloat3(label, v, v_speed, v_min,  v_max, format, flags))
            {
                vec.set(v);
                return true;
            }
            return false;
        }

        static bool serialize(cocos2d::ValueMap& target, const char* label, const cocos2d::Vec3& vec) {
            cocos2d::ValueVector v;
            v.push_back(cocos2d::Value(vec.x));
            v.push_back(cocos2d::Value(vec.y));
            v.push_back(cocos2d::Value(vec.z));
            target.emplace(label, v);
            return true;
        }

        static bool deserialize(const cocos2d::ValueMap& source, const char* label, cocos2d::Vec3& vec) {
            cocos2d::ValueMap::const_iterator it = source.find(label);
            if (it != source.end())
            {
                const cocos2d::ValueVector& v = it->second.asValueVector();
                vec.x = v[0].asFloat();
                vec.y = v[1].asFloat();
                vec.z = v[2].asFloat();
                return true;
            }
            
            return false;
        }
    };

    struct FilePath {};
    template <>
    struct PropertyImDrawer<FilePath> {
        static bool draw(const char* label, cocos2d::Value& filePath) {
            std::string v = filePath.asString();
            // assume imgui does not modify the c string because of the read-only flag
            ImGui::InputText(label, const_cast<char*>(v.c_str()), v.size(), ImGuiInputTextFlags_ReadOnly);
            if (ImGui::IsItemActivated())
            {
                Editor::getInstance()->openLoadFileDialog();
            }

            if (Editor::getInstance()->fileDialogResult(v) && !v.empty())
            {
                filePath = v;
                return true;
            }

            return false;
        }

        static bool serialize(cocos2d::ValueMap& target, const char* label, const cocos2d::Value& filePath) {
            target.emplace(label, filePath);
            return true;
        }

        static bool deserialize(const cocos2d::ValueMap& source, const char* label, cocos2d::Value& filePath) {
            cocos2d::ValueMap::const_iterator it = source.find(label);
            if (it != source.end())
            {
                filePath = it->second;
                return true;
            }
            
            return false;
        }
    };

    namespace Internal
    {
        struct EnumBase {};
        struct MaskBase {};
    }

    template <typename T>
    struct Enum: public Internal::EnumBase
    {
        using Type = typename T;
    };
    
    template <typename T>
    struct Mask: public Internal::MaskBase
    {
        using Type = typename T;
    };

    // 64bit mask is not supported because cocos2d::Value does not handle 64bit int.
    template <typename T>
    struct PropertyImDrawer<T, typename std::enable_if<std::is_base_of<Internal::MaskBase, T>::value>::type> {
        static_assert(IM_ARRAYSIZE(T::Type::s_names) == IM_ARRAYSIZE(T::Type::s_values), "Size of enum names and values do not match");
        static_assert(sizeof(T::Type::MaskType) <= 4,
            "64bit mask is not supported because cocos2d::Value does not handle 64bit int");

        static bool draw(const char* label, typename T::Type::MaskType& mask) {
            std::stringstream stream;
            bool firstItem = true;
            for (int i = 0; i < IM_ARRAYSIZE(T::Type::s_names); i++)
            {
                if ((mask & T::Type::s_values[i]) > 0)
                {
                    if (!firstItem) {
                        stream << ",";
                    }
                    firstItem = false;
                    stream << T::Type::s_names[i];
                }
            }
            
            bool valueChanged = false;
            if (ImGui::BeginCombo(label, stream.str().c_str()))
            {
                for (int i = 0; i < IM_ARRAYSIZE(T::Type::s_names); i++)
                {
                    bool v = (mask & T::Type::s_values[i]) > 0;
                    if (ImGui::Checkbox(T::Type::s_names[i], &v))
                    {
                        if (v)
                            mask |= T::Type::s_values[i];
                        else
                            mask &= ~T::Type::s_values[i];
                        valueChanged = true;
                    }

                    if (i == 0)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            return valueChanged;
        }

        static bool serialize(cocos2d::ValueMap& target, const char* label, typename T::Type::MaskType mask) {
            target.emplace(label, mask);
            return true;
        }

        static bool deserialize(const cocos2d::ValueMap& source, const char* label, typename T::Type::MaskType& mask) {
            cocos2d::ValueMap::const_iterator it = source.find(label);
            if (it != source.end())
            {
                mask = it->second.asUnsignedInt();
                return true;
            }
            
            return false;
        }
    };

    struct LightFlag {
        using MaskType = typename unsigned int;
        static constexpr const char* s_names[] = {
            "LIGHT0",
            "LIGHT1",
            "LIGHT2",
            "LIGHT3",
            "LIGHT4",
            "LIGHT5",
            "LIGHT6",
            "LIGHT7",
            "LIGHT8",
            "LIGHT9",
            "LIGHT10",
            "LIGHT11",
            "LIGHT12",
            "LIGHT13",
            "LIGHT14",
            "LIGHT15",
        };

        static constexpr int s_values[] = {
            (int)cocos2d::LightFlag::LIGHT0,
            (int)cocos2d::LightFlag::LIGHT1,
            (int)cocos2d::LightFlag::LIGHT2,
            (int)cocos2d::LightFlag::LIGHT3,
            (int)cocos2d::LightFlag::LIGHT4,
            (int)cocos2d::LightFlag::LIGHT5,
            (int)cocos2d::LightFlag::LIGHT6,
            (int)cocos2d::LightFlag::LIGHT7,
            (int)cocos2d::LightFlag::LIGHT8,
            (int)cocos2d::LightFlag::LIGHT9,
            (int)cocos2d::LightFlag::LIGHT10,
            (int)cocos2d::LightFlag::LIGHT11,
            (int)cocos2d::LightFlag::LIGHT12,
            (int)cocos2d::LightFlag::LIGHT13,
            (int)cocos2d::LightFlag::LIGHT14,
            (int)cocos2d::LightFlag::LIGHT15,
        };
    };

    struct CameraFlag {
        using MaskType = typename unsigned short;
        static constexpr const char* s_names[] = {
            "DEFAULT",
            "USER1",
            "USER2",
            "USER3",
            "USER4",
            "USER5",
            "USER6",
            "USER7",
            "USER8",
        };

        static constexpr int s_values[] = {
            (int)cocos2d::CameraFlag::DEFAULT,
            (int)cocos2d::CameraFlag::USER1,
            (int)cocos2d::CameraFlag::USER2,
            (int)cocos2d::CameraFlag::USER3,
            (int)cocos2d::CameraFlag::USER4,
            (int)cocos2d::CameraFlag::USER5,
            (int)cocos2d::CameraFlag::USER6,
            (int)cocos2d::CameraFlag::USER7,
            (int)cocos2d::CameraFlag::USER8,
        };
    };
}

#endif
