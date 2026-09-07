#include "drawing/sprite.hpp"
#include "tools/storage.hpp"
#include "tools/devices.hpp"
#include "drawing/animation.hpp"


BasicTexture::BasicTexture(uint16_t sizeX, uint16_t sizeY){
    width = sizeX;
    height = sizeY;
    transparentColor = 0;
    pixels = (uint16_t*)ps_malloc(sizeof(uint16_t) * width * height);
    if (!pixels){
        return;
    }
}

int Sprite::CreateEmptyTexture(uint16_t sizeX, uint16_t sizeY){
    BasicTexture* mem = (BasicTexture*)ps_malloc(sizeof(BasicTexture));
    if (!mem) {
        return -1;
    }
    #ifndef __INTELLISENSE__
    new (mem) BasicTexture(sizeX, sizeY);
    #endif

    if (mem->pixels == nullptr){
        heap_caps_free(mem);
        return -1;
    }
    int len = frames.size();
    frames.emplace_back(mem);

    if (w == 0){
        w = mem->width;
    }
    if (h == 0){
        h = mem->height;
    }
    return len;
}


Sprite *Sprite::Clone(){
    Sprite* sp = (Sprite*)ps_malloc(sizeof(Sprite));
    if (!sp) {
        return nullptr;
    }
    #ifndef __INTELLISENSE__
    new (sp) Sprite();
    #endif
    (*sp) = (*this);
    g_animation.IncludeSpriteInPool(sp);
    return sp;
}


int Sprite::LoadFromPng(std::string name){
    int rcError;
    size_t width_p, height_p;
    uint16_t *pixels = Storage::DecodePNG(name.c_str(), rcError, width_p, height_p);
    if (rcError != 0){
        return -1;
    }

    BasicTexture* mem = (BasicTexture*)ps_malloc(sizeof(BasicTexture));
    if (!mem) {
        heap_caps_free(mem);
        return -1;
    }
    #ifndef __INTELLISENSE__
    new (mem) BasicTexture();
    #endif

    mem->width = width_p;
    mem->height = height_p;
    mem->pixels = pixels;

    if (w == 0){
        w = mem->width;
    }
    if (h == 0){
        h = mem->height;
    }

    int len = frames.size();
    frames.emplace_back(mem);
    return len;
}

void Sprite::SetPixelColor(int idx, uint16_t x, uint16_t y, uint16_t color){
    if (idx < 0 || idx > frames.size()){
        return;
    }
    BasicTexture *tx = frames[idx];
    if (x >= tx->width){
        return;
    }
    if (y >= tx->height){
        return;
    }
    tx->pixels[x + y*tx->width] = color;
}

void Sprite::Draw(FlipConfig flipSettings, ShaderType shader_p, float shaderStrenght_p){
    if (currentFrame < 0 || currentFrame > frames.size()){
        return;
    }
    if (!visibility){
        return;
    }
    if (usingShader){
        shaderStrenght_p = shaderStrenght;
        shader_p = shader;
    }
    int targetW = w;
    int targetH = h;

    
    BasicTexture *tx = frames[currentFrame];
    const int txW = tx->width;
    const int txH = tx->height;
    const uint16_t *txPixels = tx->pixels;
    const uint16_t transColor = tx->transparentColor;

    int cropW, cropH;
    if (view.getSize(0, cropW, cropH)){
        targetW = cropW;
        targetH = cropH;
    }
    
    if (targetW > txW){
        targetW = txW;
    }
    if (targetH > txH){
        targetH = txH;
    }

    float cx = targetW * 0.5f;
    float cy = targetH * 0.5f;
    
    for (int dy=0;dy<targetH;dy++){
        
        for (int dx=0;dx<targetW;dx++){
            int xIn;
            int yIn;
            if (!view.getPosition(dx, dy, xIn, yIn)){
                continue;
            }
            if (xIn < 0 || xIn >= txW || yIn >= txH || yIn < 0){
                continue;
            }
            uint16_t color = txPixels[yIn * txW + xIn];
            if (color == transColor){
                continue;
            }

            int outDx = dx;
            int outDy = dy;

            if (rotated) {
                float fx = dx - cx;
                float fy = dy - cy;
                float rx = fx * cosA - fy * sinA;
                float ry = fx * sinA + fy * cosA;
                outDx = (int)lroundf(rx + cx);
                outDy = (int)lroundf(ry + cy);
            }
            int16_t finalX = x+outDx;
            int16_t finalY = y+outDy;

            uint8_t r;
            uint8_t g;
            uint8_t b;
            BaseDisplay::color565to888(color, r,g,b);

            ShaderProcessor::UpdateColorByShader(finalX, finalY, r, g, b, shader_p, shaderStrenght_p);

            Devices::Display->setPixelWithFlip(finalX, finalY, r,g,b, flipSettings);
        }
    }
}