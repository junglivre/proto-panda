#pragma once
#include <stdint.h>
#include "tools/psrammap.hpp"
#include "tools/displays.hpp"
#include "tools/sectionview.hpp"
#include "drawing/rendering/shader.hpp"

class BasicTexture{
    public:
        BasicTexture():pixels(nullptr),width(0),height(0),transparentColor(0){}
        BasicTexture(uint16_t w, uint16_t h);
        uint16_t *pixels;
        uint16_t width, height;
        uint16_t transparentColor;
};

class Sprite{
    public:
        Sprite():x(0),y(0),w(0),h(0),frames(),id(0),currentFrame(-1),cosA(0.0f),sinA(1.0f),shaderStrenght(1.0f),rotated(false),usingShader(false),visibility(true),shader(SHADER_NONE){};
        uint16_t x,y;
        uint16_t w,h;
        SectionMap<1> view;
        std::vector<BasicTexture*> frames;
        
        int id;
        int currentFrame;
        float cosA,sinA,shaderStrenght;
        bool rotated, usingShader, visibility;
        ShaderType shader;

        Sprite *Clone();

        void UseCustomShader(bool use){
            usingShader = use;
        }
        void SetShader(ShaderType shdr, float strenght=1.0f){
            shaderStrenght = strenght;
            shader = shdr;
        };

        int NextFrame(){
            if (currentFrame >= frames.size()-1){
                currentFrame = 0;
                return 0;
            }
            currentFrame++;
            return currentFrame;
        }

        void SetRotation(float angle){
            if (angle == 0){
                rotated = false;
                return;
            }
            float rad = angle * (float)M_PI / 180.0f;
            cosA = cosf(-rad); 
            sinA = sinf(-rad);
            rotated = true;
        }

        bool SetFrameId(int id){
            if (id < 0 || id > frames.size()){
                return false;
            }
            currentFrame = id;
            return true;
        }
        int GetFrameId(){
            return currentFrame;
        }
        int GetFrameCount(){
            return frames.size();
        }

        int GetId(){
            return id;
        };
        uint16_t GetWidth(int ida){ 
            if (ida < 0 || ida > frames.size()){
                return 0;
            }
            return frames[ida]->width;
        };
        uint16_t GetHeight(int ida){
            if (ida < 0 || ida > frames.size()){
                return 0;
            } 
            return frames[ida]->height;
        };

        void SetPosition(uint16_t xa, uint16_t ya){
            x = xa;
            y = ya;
        };

        void SetVisibility(bool v){
            visibility = v;
        };

        void SetPixelColor(int id, uint16_t x, uint16_t y, uint16_t color);
        void SetTransparencyColor(uint16_t c, int idx){
            if (idx == -1){
                for (int i=0;i<frames.size();i++){
                    frames[i]->transparentColor = c;
                }
                return;
            }
            if (idx < 0 || idx > frames.size()){
                return;
            }
            frames[idx]->transparentColor = c;
        }

        void CropSprite(int srcX,  int srcY, int pw, int ph, bool fliph, bool flipv){
            w = pw;
            h = ph;
            if (view.count() == 0){
                view.addView(srcX,  srcY, 0, 0, pw, ph, fliph, flipv);
                return;
            }
            view.setView(0, srcX,  srcY, 0, 0, pw, ph, fliph, flipv);
        }

        void ClearCrop(){
            view.clear();
        }

        int CreateEmptyTexture(uint16_t sizeX, uint16_t sizeY);

        int LoadFromPng(std::string name);

        void Draw(FlipConfig flipSettings, ShaderType shader, float shaderStrenght);
};

